cmake_minimum_required(VERSION 3.20)

project(ToyOS LANGUAGES C CXX ASM)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ------------------------------------------------------------------------------
# Global Configuration
# ------------------------------------------------------------------------------

# Ensure Clang is used
if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(WARNING "ToyOS requires Clang/LLVM. Current compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

# Common include directories (matches -I. -IAK -I$(SRC_KERNEL))
set(COMMON_INCLUDES 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/AK 
    ${CMAKE_CURRENT_SOURCE_DIR}/Kernel
    ${CMAKE_CURRENT_SOURCE_DIR}/Kernel/SharedGraphics
    ${CMAKE_CURRENT_SOURCE_DIR}/Kernel/Terminal
    ${CMAKE_CURRENT_SOURCE_DIR}/LibC
    ${CMAKE_CURRENT_SOURCE_DIR}/LibC/sys
)

set(KERNEL_COMMON_SOURCES
    Kernel/StdLib.cpp
    Kernel/entry.cpp
    Kernel/kmalloc.cpp
    Kernel/kprintf.cpp
    Kernel/SharedGraphics/CharacterBitmap.cpp
    Kernel/SharedGraphics/Color.cpp
    Kernel/SharedGraphics/Font.cpp
    Kernel/SharedGraphics/GraphicsBitmap.cpp
    Kernel/SharedGraphics/Painter.cpp
    Kernel/SharedGraphics/Rect.cpp
    Kernel/Terminal/Terminal.cpp
)

# Output directories (matches build/i386 and build/wasm)
set(DIST_I386 ${CMAKE_BINARY_DIR}/i386)
set(DIST_WASM ${CMAKE_BINARY_DIR}/wasm)

# Create output directories automatically
file(MAKE_DIRECTORY ${DIST_I386})
file(MAKE_DIRECTORY ${DIST_WASM})

set(I386_SOURCES
    arch/i386/boot.S
    ${KERNEL_COMMON_SOURCES}
)

add_executable(toyos_i386 ${I386_SOURCES})

# 3. Set Properties
# OUTPUT_NAME: Forces the output file to be "toyos.bin"
# SUFFIX: Removes default extensions (like .exe)
# LINK_DEPENDS: Re-link if the linker script changes
set_target_properties(toyos_i386 PROPERTIES 
    OUTPUT_NAME "toyos.bin"
    SUFFIX "" 
    RUNTIME_OUTPUT_DIRECTORY ${DIST_I386}
    LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/arch/i386/linker.ld
)

# 4. Compile Options (Matches CXXFLAGS_I386)
target_include_directories(toyos_i386 PRIVATE ${COMMON_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/arch/i386)

target_compile_options(toyos_i386 PRIVATE
    --target=i386-pc-none-elf -march=i386
    -ffreestanding -O2 -Wall -Wextra -std=c++20
    -fno-exceptions -fno-rtti -fno-threadsafe-statics
    -nostdinc -fno-pie
    -DI386 -DSERENITY -DKERNEL
)

# 5. Link Options (Matches LDFLAGS_I386)
# Note: Since we use add_executable, these flags are passed to the Clang driver.
# We use -Wl,<arg> to pass arguments to the underlying linker (LLD).
target_link_options(toyos_i386 PRIVATE
    --target=i386-pc-none-elf
    -march=i386
    -nostdlib                      # Do not link standard C libraries
    -fuse-ld=lld                   # Force use of LLD
    -Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/arch/i386/linker.ld
    -Wl,-m,elf_i386
    -Wl,-z,max-page-size=0x1000    # Critical for Multiboot compatibility
    -Wl,--no-pie
    -Wl,--build-id=none
)

# 6. Verification Step (Matches 'verify' target)
# Runs immediately after the binary is built
add_custom_command(TARGET toyos_i386 POST_BUILD
    COMMAND grub-file --is-x86-multiboot $<TARGET_FILE:toyos_i386>
    COMMAND ${CMAKE_COMMAND} -E echo "[CHECK] Multiboot Header found. Binary is valid."
    COMMENT "Verifying Multiboot header compatibility..."
)

# ------------------------------------------------------------------------------
# ISO Generation (toyos.iso)
# ------------------------------------------------------------------------------

set(ISO_IMAGE ${DIST_I386}/toyos.iso)
set(GRUB_CFG ${CMAKE_CURRENT_SOURCE_DIR}/arch/i386/grub.cfg)
set(ISO_STAGING ${CMAKE_BINARY_DIR}/isodir)

add_custom_command(
    OUTPUT ${ISO_IMAGE}
    # Create staging directory structure
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ISO_STAGING}/boot/grub
    # Copy kernel binary
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:toyos_i386> ${ISO_STAGING}/boot/toyos.bin
    # Copy GRUB config
    COMMAND ${CMAKE_COMMAND} -E copy ${GRUB_CFG} ${ISO_STAGING}/boot/grub/grub.cfg
    # Generate ISO using grub-mkrescue
    COMMAND grub-mkrescue -o ${ISO_IMAGE} ${ISO_STAGING}
    # Clean up staging directory
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${ISO_STAGING}
    DEPENDS toyos_i386 ${GRUB_CFG}
    COMMENT "[ISO] Generating bootable ISO: ${ISO_IMAGE}"
)

# Top-level target to build the ISO
add_custom_target(toyos_iso ALL DEPENDS ${ISO_IMAGE})

# ------------------------------------------------------------------------------
# Target 2: Wasm Kernel (toyos.wasm)
# ------------------------------------------------------------------------------

# 1. Define Sources
set(WASM_SOURCES ${KERNEL_COMMON_SOURCES})

# 2. Create Executable Target
add_executable(toyos_wasm ${WASM_SOURCES})

# 3. Set Properties
set_target_properties(toyos_wasm PROPERTIES 
    OUTPUT_NAME "toyos"
    SUFFIX ".wasm"
    RUNTIME_OUTPUT_DIRECTORY ${DIST_WASM}
)

# 4. Compile Options (Matches CXXFLAGS_WASM)
target_include_directories(toyos_wasm PRIVATE ${COMMON_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/arch/wasm)

target_compile_options(toyos_wasm PRIVATE
    --target=wasm32
    -ffreestanding -O2 -Wall -Wextra -std=c++20
    -fno-exceptions -fno-rtti 
    -nostdinc
    -DWASM -DSERENITY -DKERNEL
)

# 5. Link Options (Matches LDFLAGS_WASM)
target_link_options(toyos_wasm PRIVATE
    --target=wasm32 -nostdlib
    -Wl,--no-entry
    -Wl,--export=kernel_entry
    -Wl,--allow-undefined
    [CRITICAL FIX] Tell the linker: Import memory from JS, do not create it yourself!
    -Wl,--import-memory
    -fuse-ld=lld
)

# 6. Post-Build Assets Copy
# Copies index.html and server.py to the build directory
add_custom_command(TARGET toyos_wasm POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/arch/wasm/index.html ${DIST_WASM}/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/arch/wasm/server.py ${DIST_WASM}/
    COMMENT "Deploying Wasm assets to ${DIST_WASM}..."
)

# ------------------------------------------------------------------------------
# Run Targets (Helper Scripts)
# ------------------------------------------------------------------------------

# Target: make runq (Run QEMU)
add_custom_target(runq 
    COMMAND qemu-system-i386 -cdrom ${ISO_IMAGE} -vga std -debugcon file:debug.log
    DEPENDS toyos_iso
    USES_TERMINAL
    COMMENT "Starting QEMU..."
)

# Target: make runs (Run Python Server)
add_custom_target(runs
    COMMAND python3 server.py
    WORKING_DIRECTORY ${DIST_WASM}
    DEPENDS toyos_wasm
    USES_TERMINAL
    COMMENT "Starting Web Server at http://localhost:8000 ..."
)