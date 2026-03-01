# ToyOS: A Dual-Target (i386 & WebAssembly) OS Kernel

**ToyOS** is an experimental, dual-target operating system kernel designed to run natively on both **i386 architecture** and entirely within a web browser via **WebAssembly (WASM)**.

### The Inspiration
This project was initially inspired by [Linux Kernel to Wasm](https://github.com/joelseverin/linux-wasm), which sparked my interest in the possibility of running an OS kernel in a browser sandbox. Drawing architectural inspiration from the clean codebase of [SerenityOS](https://github.com/SerenityOS/serenity), I set out to build a kernel from scratch. The primary goal was to implement core operating system concepts—such as process context switching, scheduling, and User Mode execution—in both a traditional hardware environment and a WASM runtime.

### The Architectural Pivot: Web Workers vs. Asyncify
In the browser environment, my initial thought was to use Web Workers to simulate hardware threads (similar to linux-wasm). However, I quickly found Web Workers to be too "heavy" for the lightweight, deterministic context switching I wanted to achieve.

Instead, I pivoted to a purely synchronous-looking approach using Binaryen's Asyncify. By running the code through wasm-opt, I injected complete Asyncify capabilities into the binary, utilizing hooks like: asyncify_start_unwind/asyncify_stop_unwind and 
asyncify_start_rewind/asyncify_stop_rewind.

### The Hard Problem: Dual-WASM State Machine Synchronization
The true engineering challenge emerged when implementing System Calls (e.g., getch or yield) from User Mode.

Unlike a standard WASM application, ToyOS involves two isolated WASM instances interacting with each other: the kernel.wasm and the userland.wasm. When a userland process initiates a blocking syscall, the stack must be safely unwound back to the JavaScript host. The JavaScript layer acts as the "hardware," orchestrating the state transitions. Both the kernel WASM and the userland WASM must maintain and synchronize their own correct state machines during the Asyncify pause/resume cycles.

### First-Principles Debugging vs. GenAI: A Dialectical Experience
Before discovering the dual-state machine solution, the system suffered from catastrophic crashes with incomprehensible call stacks. I fed these crash dumps to state-of-the-art LLMs (Gemini and Claude). Gemini misdiagnosed the issue as "memory corruption between user and kernel space," while Claude confidently blamed "incorrect standard library implementations."

Interestingly, even when presented with solid counter-evidence from my debugging logs, both models remained stubbornly anchored to their incorrect hypotheses. It took me an entire day of isolated, deep thinking—reverting strictly to First-Principles and meticulously tracing the execution flow across the JS/WASM boundary—to finally identify the root cause: the desynchronization of the Asyncify unwind/rewind states between the two isolated WASM instances. Only after I explicitly mapped out the correct architecture did the LLMs "agree" with the solution.

However, this debugging journey revealed a fascinating duality in modern AI tools. While they failed entirely at solving a highly non-standard, undocumented architectural bug, I found their semantic comprehension—particularly Gemini's—to be exceptionally valuable. Whenever I had a vague or fragmented architectural intuition, the LLM acted as an outstanding sounding board, perfectly grasping my intent and articulating it into clear, highly structured engineering concepts.

Ultimately, this project highlights the true paradigm of AI-assisted systems programming: **GenAI is an unparalleled tool for clarifying thoughts and accelerating articulation, but solving deep, zero-to-one domain challenges still absolutely dictates human architectural insight and first-principles thinking**.
