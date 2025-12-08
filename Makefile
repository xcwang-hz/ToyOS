SRC_COMMON := common
ARCH_I386  := arch/i386
ARCH_WASM  := arch/wasm
BUILD_DIR  := build
DIST_I386  := $(BUILD_DIR)/i386
DIST_WASM  := $(BUILD_DIR)/wasm

CXX_I386      := clang++
LD_I386       := ld.lld
AS_I386       := clang

CXXFLAGS_I386 := --target=i386-pc-none-elf -march=i386 \
                 -ffreestanding -O2 -Wall -Wextra -std=c++20 \
                 -fno-exceptions -fno-rtti -fno-threadsafe-statics \
                 -nostdinc \
				 -fno-pie \
                 -I. -IAK -I$(SRC_COMMON) -I$(ARCH_I386)

# Linker must be LLD (part of LLVM) for best compatibility
LDFLAGS_I386  := -m elf_i386 \
				 -T $(ARCH_I386)/linker.ld \
				 -z max-page-size=0x1000 \
				 --no-pie

# --- Wasm Toolchain (Clang) ---
CXX_WASM      := clang
CXXFLAGS_WASM := --target=wasm32 \
                 -ffreestanding -O2 -Wall -Wextra -std=c++20 \
                 -fno-exceptions -fno-rtti \
                 -nostdinc \
                 -I. -IAK -I$(SRC_COMMON) -I$(ARCH_WASM)

LDFLAGS_WASM  := --target=wasm32 \
				 -nostdlib \
				 -Wl,--no-entry \
				 -Wl,--export=kernel_entry \
				 -Wl,--allow-undefined \
				 -fuse-ld=lld

# --- Output Files ---
KERNEL_BIN  := $(DIST_I386)/toyos.bin
ISO_IMAGE   := $(DIST_I386)/toyos.iso
KERNEL_WASM := $(DIST_WASM)/toyos.wasm
OBJS_I386   := $(DIST_I386)/boot.o $(DIST_I386)/hal_i386.o $(DIST_I386)/kernel.o
OBJS_WASM   := $(DIST_WASM)/kernel.o

# ==============================================================================
# Rules
# ==============================================================================

.PHONY: all clean runs runq server directories iso

all: directories $(ISO_IMAGE) $(KERNEL_WASM)

directories:
	@mkdir -p $(DIST_I386)
	@mkdir -p $(DIST_WASM)

# --- i386 Build ---

$(ISO_IMAGE): $(KERNEL_BIN) $(ARCH_I386)/grub.cfg
	@echo "[ISO]  Generating $@"
	@mkdir -p $(BUILD_DIR)/isodir/boot/grub
	@cp $(KERNEL_BIN) $(BUILD_DIR)/isodir/boot/toyos.bin
	@cp $(ARCH_I386)/grub.cfg $(BUILD_DIR)/isodir/boot/grub/grub.cfg
	@grub-mkrescue -o $(ISO_IMAGE) $(BUILD_DIR)/isodir > /dev/null 2>&1
	@rm -rf $(BUILD_DIR)/isodir

$(KERNEL_BIN): $(OBJS_I386) $(ARCH_I386)/linker.ld
	@echo "[LINK] i386 Kernel (LLD)"
	@$(LD_I386) -o $@ $(OBJS_I386) $(LDFLAGS_I386)

$(DIST_I386)/kernel.o: $(SRC_COMMON)/kernel.cpp
	@echo "[CXX]  i386 Common"
	@$(CXX_I386) $(CXXFLAGS_I386) -c $< -o $@

$(DIST_I386)/hal_i386.o: $(ARCH_I386)/hal_i386.cpp
	@echo "[CXX]  i386 HAL"
	@$(CXX_I386) $(CXXFLAGS_I386) -c $< -o $@

$(DIST_I386)/boot.o: $(ARCH_I386)/boot.S
	@echo "[AS]   i386 Boot"
	@$(CXX_I386) $(CXXFLAGS_I386) -c $< -o $@

# --- Wasm Build ---

$(KERNEL_WASM): $(OBJS_WASM)
	@echo "[LINK] Wasm Module (LLD)"
	@$(CXX_WASM) -o $@ $(OBJS_WASM) $(LDFLAGS_WASM)
	@cp $(ARCH_WASM)/index.html $(DIST_WASM)/ 2>/dev/null || :
	@cp $(ARCH_WASM)/server.py $(DIST_WASM)/ 2>/dev/null || :
	@cp $(ARCH_WASM)/xterm.js $(DIST_WASM)/ 2>/dev/null || :
	@cp $(ARCH_WASM)/xterm.css $(DIST_WASM)/ 2>/dev/null || :

$(DIST_WASM)/kernel.o: $(SRC_COMMON)/kernel.cpp
	@echo "[CXX]  Wasm Common"
	@$(CXX_WASM) $(CXXFLAGS_WASM) -c $< -o $@

# --- Utility ---

clean:
	rm -rf $(BUILD_DIR)

runq: $(KERNEL_BIN)
	qemu-system-i386 -cdrom $(ISO_IMAGE) -vga std

runs: $(KERNEL_WASM)
	cd $(DIST_WASM) && python3 server.py
