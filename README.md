# ToyOS

**ToyOS** is an experimental, dual-architecture operating system kernel written in C++20. It is designed to run on both **i386 bare metal** and **WebAssembly (Browser)** environments from a single codebase.

### 🎯 Motivation
The primary goal of ToyOS is to deconstruct and understand operating system fundamentals by bridging the gap between traditional hardware and the modern web runtime. It aims to explore how a monolithic kernel architecture can adapt to the constraints of the WebAssembly environment.

### 🏗️ Architecture & Technology Stack
ToyOS is a hybrid built upon two major pillars:

1.  **Kernel Logic & Subsystems (The "Brain"):**
    * **Derived from [SerenityOS](https://github.com/SerenityOS/serenity) (2019 era):** The core kernel architecture, including the `AK` (Algorithms Kit), `LibGfx` (Graphics), and PTY/TTY subsystems, is extracted from early versions of SerenityOS. This provides a clean, C++20 implementation of OS fundamentals without the complexity of modern x86_64 support.

2.  **Wasm Runtime & Toolchain (The "Body"):**
    * **Adapted from [Linux Kernel to Wasm](https://github.com/joelseverin/linux-to-webassembly):** The project leverages the build system strategies and runtime concepts pioneered by Joel Severin. Specifically, it adopts the Buildroot configurations and techniques for executing No-MMU userland applications (like **BusyBox**) within the WebAssembly linear memory model.

**Key Features:**
* **Dual-Arch Build System:** Seamlessly compiles to `.bin` (Multiboot i386) and `.wasm` (HTML5 host).
* **Hybrid Graphics:** Implements a custom HAL that bridges software-rendered framebuffers to WebGL via **EGL Texture Upload**, enabling hardware-accelerated presentation of software surfaces.
* **K-Console:** A graphical kernel console capable of handling ANSI escape codes.

### 📜 Credits
This project stands on the shoulders of giants:
* **SerenityOS** (Andreas Kling): For the elegant C++ kernel architecture and graphics stack.
* **Linux Kernel to WebAssembly** (Joel Severin): For the groundbreaking work on Wasm toolchains and userland execution environments.