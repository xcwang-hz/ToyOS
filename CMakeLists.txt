cmake_minimum_required(VERSION 3.20)

project(ToyOS LANGUAGES C CXX ASM)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

# Variable to hold flags for wasm-opt (Binaryen)
set(WASM_OPT_FLAGS "")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0 -DDEBUG)
    # For wasm: keep debug symbols in the final binary
    set(WASM_OPT_FLAGS "-g") 
else()
    add_compile_options(-O2 -DNDEBUG)
    # For wasm: strip debug symbols to reduce size
    set(WASM_OPT_FLAGS "--strip-debug")
endif()

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
    AK/String.cpp
    AK/StringImpl.cpp
    Kernel/CpioFileSystem.cpp
    Kernel/IRQHandler.cpp
    Kernel/Keyboard.cpp
    Kernel/Process.cpp
    Kernel/Scheduler.cpp
    Kernel/StdLib.cpp
    Kernel/Syscall.cpp
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

set(I386_KERNEL_ONLY_SOURCES
    arch/i386/PIC.cpp
    arch/i386/boot.S
    arch/i386/entry.cpp
    arch/i386/i386.cpp
    arch/i386/i8253.cpp
)

set(LIBC_SOURCES
    LibC/entry.cpp
    LibC/stdio.cpp
    LibC/string.cpp
    LibC/unistd.cpp
)

# Output directories (matches build/i386 and build/wasm)
set(DIST_I386 ${CMAKE_BINARY_DIR}/i386)
set(DIST_WASM ${CMAKE_BINARY_DIR}/wasm)

# Create output directories automatically
file(MAKE_DIRECTORY ${DIST_I386})
file(MAKE_DIRECTORY ${DIST_WASM})

set(I386_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/i386
    ${COMMON_INCLUDES}
)

set(I386_SOURCES
    ${I386_KERNEL_ONLY_SOURCES}
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
target_include_directories(toyos_i386 PRIVATE ${I386_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/arch/i386)

target_compile_options(toyos_i386 PRIVATE
    --target=i386-pc-none-elf -march=i386
    -ffreestanding -Wall -Wextra -std=c++20
    -fno-exceptions -fno-rtti -fno-threadsafe-statics
    -nostdinc -fno-pie
    -DI386 -DSERENITY
)

set_source_files_properties(${KERNEL_COMMON_SOURCES} ${I386_KERNEL_ONLY_SOURCES}
    PROPERTIES COMPILE_DEFINITIONS "KERNEL"
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

set(DIST_USERLAND_I386 ${DIST_I386}/bin)
file(MAKE_DIRECTORY ${DIST_USERLAND_I386})

add_library(LibC_i386 STATIC ${LIBC_SOURCES})

# Apply compilation flags for LibC (similar to Kernel but strictly 32-bit ELF)
target_include_directories(LibC_i386 PRIVATE ${COMMON_INCLUDES})
target_compile_options(LibC_i386 PRIVATE
    --target=i386-pc-none-elf -march=i386
    -ffreestanding -Wall -Wextra -std=c++20
    -fno-exceptions -fno-rtti -fno-threadsafe-statics
    -nostdinc -fno-pie
    -DSERENITY -DI386 -DUSERLAND
)

set_target_properties(LibC_i386 PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${DIST_I386}
    OUTPUT_NAME "LibC"
    PREFIX ""
)

macro(add_i386_userland_prog NAME SOURCE)
    # The executable consists of the source file AND the assembly entry point (crt0.s)
    add_executable(${NAME}_i386 ${SOURCE})

    # Link against our custom LibC
    target_link_libraries(${NAME}_i386 LibC_i386)

    # Set output location
    set_target_properties(${NAME}_i386 PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY ${DIST_USERLAND_I386}
        OUTPUT_NAME "${NAME}" # No extension for ELF files usually
        SUFFIX ""
    )

    # Compilation flags
    target_include_directories(${NAME}_i386 PRIVATE ${COMMON_INCLUDES})
    target_compile_options(${NAME}_i386 PRIVATE
        --target=i386-pc-none-elf -march=i386
        -ffreestanding -Wall -Wextra -std=c++20
        -fno-exceptions -fno-rtti
        -nostdinc -fno-pie
        -DUSERLAND
    )

    # Linker flags (Critical: Use custom linker script and prevent standard libs)
    target_link_options(${NAME}_i386 PRIVATE
        --target=i386-pc-none-elf -march=i386
        -nostdlib
        -fuse-ld=lld
        -Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/Userland/linker.ld
        -Wl,--no-pie
        -Wl,--entry=_start
    )
endmacro()
add_i386_userland_prog(clear Userland/clear.cpp)
add_i386_userland_prog(help Userland/help.cpp)
add_i386_userland_prog(sh Userland/sh.cpp)

set(INITRAMFS_I386 ${DIST_I386}/initramfs.cpio)
set(ROOTFS_I386 ${DIST_I386}/rootfs)

add_custom_command(
    OUTPUT ${INITRAMFS_I386}
    # Create directory structure
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ROOTFS_I386}/bin
    
    # Copy i386 executables (strip them to reduce size, keeping them as ELF)
    COMMAND llvm-strip ${DIST_USERLAND_I386}/clear -o ${ROOTFS_I386}/bin/clear
    COMMAND llvm-strip ${DIST_USERLAND_I386}/help -o ${ROOTFS_I386}/bin/help
    COMMAND llvm-strip ${DIST_USERLAND_I386}/sh -o ${ROOTFS_I386}/bin/sh
    
    # Pack into CPIO (New ASCII Format)
    # We cd into the dir so paths inside CPIO are relative (e.g., "bin/clear")
    COMMAND sh -c "cd ${ROOTFS_I386} && find . | cpio -o -H newc > ${INITRAMFS_I386}"
    
    DEPENDS clear_i386 help_i386 sh_i386
    COMMENT "Generating i386 Initramfs (ELF)..."
    VERBATIM
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
    # Copy CPIO to ISO
    COMMAND ${CMAKE_COMMAND} -E copy ${INITRAMFS_I386} ${ISO_STAGING}/boot/initramfs.cpio
    # Generate ISO using grub-mkrescue
    COMMAND grub-mkrescue -o ${ISO_IMAGE} ${ISO_STAGING}
    # Clean up staging directory
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${ISO_STAGING}
    DEPENDS toyos_i386 ${GRUB_CFG} ${INITRAMFS_I386}
    COMMENT "[ISO] Generating bootable ISO with Initramfs..."
)

# Top-level target to build the ISO
add_custom_target(toyos_iso ALL DEPENDS ${ISO_IMAGE})

# ------------------------------------------------------------------------------
# Target 2: wasm Kernel (toyos.wasm)
# ------------------------------------------------------------------------------

set(WASM_KERNEL_ONLY_SOURCES
    arch/wasm/entry.cpp
)

set(WASM_SOURCES
    ${WASM_KERNEL_ONLY_SOURCES}
    ${KERNEL_COMMON_SOURCES}
)

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
    -ffreestanding -Wall -Wextra -std=c++20
    -fno-exceptions -fno-rtti 
    -nostdinc
    -DWASM -DSERENITY
)

set_source_files_properties(${KERNEL_COMMON_SOURCES} ${WASM_KERNEL_ONLY_SOURCES}
    PROPERTIES COMPILE_DEFINITIONS "KERNEL"
)

# 5. Link Options (Matches LDFLAGS_WASM)
target_link_options(toyos_wasm PRIVATE
    --target=wasm32 -nostdlib
    -Wl,--no-entry
    -Wl,--export=wasm_entry
    -Wl,--export=wasm_handle
    -Wl,--export=wasm_pending_key
    -Wl,--allow-undefined
    # [CRITICAL FIX] Tell the linker: Import memory from JS, do not create it yourself!
    -Wl,--import-memory
    -fuse-ld=lld
)

# This command runs 'wasm-opt' to rewrite the binary for stack switching.
add_custom_command(TARGET toyos_wasm POST_BUILD
    COMMAND wasm-opt $<TARGET_FILE:toyos_wasm>
            -o $<TARGET_FILE_DIR:toyos_wasm>/toyos_wasm.async.wasm
            --asyncify
            # [CRITICAL] Tell Asyncify: "When C++ calls this import, start unwinding."
            --pass-arg=asyncify-imports@env.js_context_switch
            # Prevent other imports (like kmalloc) from triggering unwinding
            --pass-arg=asyncify-ignore-imports
            ${WASM_OPT_FLAGS}
    
    COMMAND mv $<TARGET_FILE_DIR:toyos_wasm>/toyos_wasm.async.wasm $<TARGET_FILE:toyos_wasm>
    COMMENT "Running Binaryen Asyncify pass..."
    VERBATIM
)

set(DIST_USERLAND_WASM ${DIST_WASM}/bin)
file(MAKE_DIRECTORY ${DIST_USERLAND_WASM})

add_library(LibC_wasm STATIC ${LIBC_SOURCES})

target_include_directories(LibC_wasm PRIVATE ${COMMON_INCLUDES})
target_compile_options(LibC_wasm PRIVATE
    --target=wasm32
    -ffreestanding -Wall -Wextra -std=c++20
    -fno-exceptions -fno-rtti -fno-threadsafe-statics
    -nostdinc 
    -DSERENITY -DWASM -DUSERLAND
)

set_target_properties(LibC_wasm PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${DIST_WASM}
    OUTPUT_NAME "LibC"
    PREFIX ""
)

# 2. Macro to add wasm user programs
macro(add_wasm_userland_prog NAME SOURCE)
    add_executable(${NAME}_wasm ${SOURCE})

    # Link against wasm version of LibC
    target_link_libraries(${NAME}_wasm LibC_wasm)

    set_target_properties(${NAME}_wasm PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY ${DIST_USERLAND_WASM}
        OUTPUT_NAME "${NAME}"
        SUFFIX ".wasm"
    )

    target_include_directories(${NAME}_wasm PRIVATE ${COMMON_INCLUDES})
    target_compile_options(${NAME}_wasm PRIVATE
        --target=wasm32
        -ffreestanding -Wall -Wextra -std=c++20
        -fno-exceptions -fno-rtti
        -nostdinc
        -DWASM -DUSERLAND
    )

    # wasm Linker Options
    # We explicitly tell the linker to use _start (from entry.cpp) as the entry point
    target_link_options(${NAME}_wasm PRIVATE
        --target=wasm32
        -nostdlib
        -fuse-ld=lld
        -Wl,--entry=_start 
        -Wl,--allow-undefined 
    )
    
    # Optional: Run Asyncify on user programs if they use blocking syscalls that yield
    # add_custom_command(...)
endmacro()

# Instantiate wasm Userland Programs
add_wasm_userland_prog(clear Userland/clear.cpp)
add_wasm_userland_prog(help Userland/help.cpp)
add_wasm_userland_prog(sh Userland/sh.cpp)

set(INITRAMFS_WASM ${DIST_WASM}/initramfs.cpio)
set(ROOTFS_WASM ${DIST_WASM}/rootfs)

add_custom_command(
    OUTPUT ${INITRAMFS_WASM}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ROOTFS_WASM}/bin
    
    # Copy Wasm executables (Remove .wasm extension for "feeling" like native cmds)
    COMMAND ${CMAKE_COMMAND} -E copy ${DIST_USERLAND_WASM}/clear.wasm ${ROOTFS_WASM}/bin/clear
    COMMAND ${CMAKE_COMMAND} -E copy ${DIST_USERLAND_WASM}/help.wasm ${ROOTFS_WASM}/bin/help
    COMMAND ${CMAKE_COMMAND} -E copy ${DIST_USERLAND_WASM}/sh.wasm ${ROOTFS_WASM}/bin/sh
    
    # Pack into CPIO
    COMMAND sh -c "cd ${ROOTFS_WASM} && find . | cpio -o -H newc > ${INITRAMFS_WASM}"
    
    DEPENDS clear_wasm help_wasm sh_wasm
    COMMENT "Generating Wasm Initramfs (Wasm Modules)..."
    VERBATIM
)

add_custom_target(gen_initramfs_wasm_target DEPENDS ${INITRAMFS_WASM})
add_dependencies(toyos_wasm gen_initramfs_wasm_target)

# 6. Post-Build Assets Copy
# Copies index.html and server.py to the build directory
add_custom_command(TARGET toyos_wasm POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/arch/wasm/index.html ${DIST_WASM}/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/arch/wasm/server.py ${DIST_WASM}/
    COMMENT "Deploying wasm assets to ${DIST_WASM}..."
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

# Target: make debugq (Run QEMU)
add_custom_target(debugq 
    COMMAND qemu-system-i386 -cdrom ${ISO_IMAGE} -vga std -debugcon file:debug.log -s -S
    DEPENDS toyos_iso
    USES_TERMINAL
    COMMENT "Starting QEMU in Debug Mode (Waiting for GDB)..."
)

# Target: make runs (Run Python Server)
add_custom_target(runs
    COMMAND python3 server.py
    WORKING_DIRECTORY ${DIST_WASM}
    DEPENDS toyos_wasm
    USES_TERMINAL
    COMMENT "Starting Web Server at http://localhost:8000 ..."
)