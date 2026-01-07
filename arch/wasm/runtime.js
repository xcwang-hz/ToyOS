// -- Core System State --
// Moving these to top-level makes them accessible via 'window' or console.
let kernelInstance = null;
// 256 pages = 16MB
const wasmMemory = new WebAssembly.Memory({ initial: 256, maximum: 256 }); 

// -- Scheduler State --
let nextProcessStatePtr = 0; 
let prevProcessStatePtr = 0;   
const processStatePtrs = new Set(); 

// -- Userland Process Management --
// userProcId -> { instance, _start, name }
const userProcs = new Map();  
let nextUserProcId = 1;
let currentRunningUserProcId = 0;

// -- Graphics & Debug State --
let canvasCtx = null;
let sharedImgData = null;
let debugLineBuffer = "";

function log(text) {
    console.log(text);
}

function refreshScreen() {
    if (canvasCtx && sharedImgData) {
        canvasCtx.putImageData(sharedImgData, 0, 0);
    }
}

const PS2_SCANCODES = {
    "Escape": 0x01, "Digit1": 0x02, "Digit2": 0x03, "Digit3": 0x04, "Digit4": 0x05, 
    "Digit5": 0x06, "Digit6": 0x07, "Digit7": 0x08, "Digit8": 0x09, "Digit9": 0x0A, 
    "Digit0": 0x0B, "Minus": 0x0C, "Equal": 0x0D, "Backspace": 0x0E, "Tab": 0x0F, 
    "KeyQ": 0x10, "KeyW": 0x11, "KeyE": 0x12, "KeyR": 0x13, "KeyT": 0x14, "KeyY": 0x15, 
    "KeyU": 0x16, "KeyI": 0x17, "KeyO": 0x18, "KeyP": 0x19, "BracketLeft": 0x1A, 
    "BracketRight": 0x1B, "Enter": 0x1C, "ControlLeft": 0x1D, "KeyA": 0x1E, "KeyS": 0x1F, 
    "KeyD": 0x20, "KeyF": 0x21, "KeyG": 0x22, "KeyH": 0x23, "KeyJ": 0x24, "KeyK": 0x25, 
    "KeyL": 0x26, "Semicolon": 0x27, "Quote": 0x28, "Backquote": 0x29, "ShiftLeft": 0x2A, 
    "Backslash": 0x2B, "KeyZ": 0x2C, "KeyX": 0x2D, "KeyC": 0x2E, "KeyV": 0x2F, "KeyB": 0x30,
    "KeyN": 0x31, "KeyM": 0x32, "Comma": 0x33, "Period": 0x34, "Slash": 0x35, 
    "ShiftRight": 0x36, "NumpadMultiply": 0x37, "AltLeft": 0x38, "Space": 0x39
};
function handleKeyEvent(e, isDown) {
    if (!kernelInstance) return;

    const scancode = PS2_SCANCODES[e.code];
    if (scancode) {
        if (e.ctrlKey || e.altKey) e.preventDefault(); 

        let codeToSend = scancode;
        if (!isDown) codeToSend |= 0x80;

        // Write directly to Kernel Memory (Hardware Interrupt Simulation)
        const pendingKeyAddr = kernelInstance.exports.wasm_pending_key.value; 
        const mem32 = new Int32Array(wasmMemory.buffer);
        mem32[pendingKeyAddr / 4] = codeToSend;
    } else {
        console.warn(`ToyOS_JS: Unmapped key: ${e.code}`);
    }
}

/**
 * Creates the import object for Userland processes.
 * Allows Userland to call Kernel syscalls via JS bridge.
 */
function createUserImports(processName) {
    return {
        env: {
            memory: wasmMemory, // Shared memory model (Simplification)

            js_syscall_handle: (func, arg1, arg2, arg3) => {
                // Safety: If Kernel is unwinding, do not re-enter
                if (kernelInstance.exports.asyncify_get_state() === 1) return 0;

                // 1. Marshall arguments to Kernel
                const syscallParamsAddr = kernelInstance.exports.wasm_syscall_params.value;
                const mem32 = new Int32Array(wasmMemory.buffer);
                mem32[syscallParamsAddr / 4 + 0] = func;
                mem32[syscallParamsAddr / 4 + 1] = arg1;
                mem32[syscallParamsAddr / 4 + 2] = arg2;
                mem32[syscallParamsAddr / 4 + 3] = arg3;      
                
                // 2. Execute Kernel Syscall Handler
                const result = kernelInstance.exports.wasm_syscall_handle();

                // 3. Check for Asyncify Yield (Unwind)
                if (kernelInstance.exports.asyncify_get_state() === 1) {
                    // Critical: Kernel yielded, so Userland MUST also yield (Unwind)
                    const userInstance = userProcs.get(currentRunningUserProcId).instance;
                    const stackPtr = userInstance.exports.wasm_user_asyncify_buffer();
                    userInstance.exports.asyncify_start_unwind(stackPtr);
                    return 0; // Return value ignored during unwind
                }              

                const retval = mem32[syscallParamsAddr / 4 + 4];
                refreshScreen();
                return retval;  
            }
        }
    };
}

/**
 * Creates the import object for the Kernel.
 * Connects Kernel to JS Hardware (Canvas, Console, Scheduler Control).
 */
function createKernelImports() {
    return {
        env: {
            memory: wasmMemory,

            // --- Graphics ---
            js_canvas_init: (ptr, width, height) => {
                const bufferSize = width * height * 4;
                const dataArray = new Uint8ClampedArray(wasmMemory.buffer, ptr, bufferSize);
                sharedImgData = new ImageData(dataArray, width, height);
            },

            // --- Debugging ---
            js_console_error: (ptr) => {
                const mem = new Uint8Array(wasmMemory.buffer);
                let str = "";
                while (mem[ptr] !== 0) str += String.fromCharCode(mem[ptr++]);
                console.error(`[ToyOS: RAW] ${str}`);
            },

            js_debug_char: (ch) => {
                if (ch === 10) { 
                    console.log(`%c[ToyOS] ${debugLineBuffer}`, "color: #00AAFF");
                    debugLineBuffer = "";
                } else if (ch !== 13) {
                    debugLineBuffer += String.fromCharCode(ch);
                }                
            },

            // --- Process Management (Asyncify Orchestration) ---
            js_context_switch: (prevCtxPtr, nextCtxPtr) => {
                // Kernel requests a yield (Unwind Phase)
                if (kernelInstance.exports.asyncify_get_state() == 0) {
                    console.log(`[JS] Context Switch: Save to ${prevCtxPtr}, Next is ${nextCtxPtr}`);
                    kernelInstance.exports.asyncify_start_unwind(prevCtxPtr);
                    processStatePtrs.add(prevCtxPtr);
                    nextProcessStatePtr = nextCtxPtr;
                    prevProcessStatePtr = prevCtxPtr;
                } else {
                    // Kernel is resuming (Rewind Stop Phase)
                    console.log(`[JS] Process Resumed.`);
                    kernelInstance.exports.asyncify_stop_rewind();
                }
            },

            // --- Userland Loader ---
            js_load_user_process: (ptr, size) => {
                console.log(`[JS] Loading user process from 0x${ptr.toString(16)}, size ${size}`);
                
                // Copy Wasm bytes from Kernel Heap to a temporary buffer
                const kernelMem = new Uint8Array(wasmMemory.buffer);
                // Must copy! WebAssembly.instantiate requires a detached/independent buffer or specific alignment
                const wasmBytes = kernelMem.slice(ptr, ptr + size); 

                // Validate Magic Number
                if (wasmBytes[0] !== 0x00 || wasmBytes[1] !== 0x61 || wasmBytes[2] !== 0x73 || wasmBytes[3] !== 0x6d) {
                    console.error("[JS] Invalid WASM magic number!");
                    return -1;
                }

                const userProcId = nextUserProcId++;
                const processName = `UserProc-${userProcId}`;
                try {
                    const module = new WebAssembly.Module(wasmBytes);
                    const instance = new WebAssembly.Instance(module, createUserImports(processName));
                    
                    if (!instance.exports._start) throw new Error("No _start export");

                    const stackPtr = instance.exports.wasm_user_asyncify_buffer();
                    console.log(`[JS] [Debug] User Stack State Ptr: 0x${stackPtr.toString(16)}`);                
                    
                    userProcs.set(userProcId, {
                        instance: instance,
                        _start: instance.exports._start,
                        name: processName
                    });                  
                    console.log(`[JS] ${processName} loaded successfully`);
                } catch (e) {
                    console.error("[JS] User process compilation failed:", e);
                    return 0;
                }
                return userProcId;
            },

            js_start_user_process: (userProcId) => {
                const userProc = userProcs.get(userProcId);
                if (!userProc) return false;
                
                currentRunningUserProcId = userProcId;
                console.log(`[JS] Starting ${userProc.name}`);

                // Check if we need to Rewind Userland (if returning from a syscall yield)
                const kernelState = kernelInstance.exports.asyncify_get_state();
                if (kernelState === 2) { 
                    console.log(`[JS] Rewinding Userland ${userProc.name}...`);
                    const stackPtr = userProc.instance.exports.wasm_user_asyncify_buffer();
                    userProc.instance.exports.asyncify_start_rewind(stackPtr);
                }              

                try {
                    userProc._start();
                } catch (e) {
                    console.error(`[JS] ${userProc.name} crashed:`, e);
                    return false;
                }       
                return true;
            },
        }
    };
}

/**
 * Main Scheduler Loop.
 * Controls the Asyncify Rewind/Unwind cycle for process switching.
 */
async function runSchedulerLoop() {
    while (true) {
        if (nextProcessStatePtr === 0) break;

        // Yield to browser UI thread to keep page responsive
        await new Promise(r => setTimeout(r, 0));
        
        // Decide: Rewind old process OR Start new process
        if (processStatePtrs.has(nextProcessStatePtr)) {
            console.log(`[JS] Rewinding existing process: ${nextProcessStatePtr}`);
            kernelInstance.exports.asyncify_start_rewind(nextProcessStatePtr);
        } else {
            console.log(`[JS] Starting new process: ${nextProcessStatePtr}`);
            // Reset Asyncify state if needed (Edge case handling)
            if (kernelInstance.exports.asyncify_get_state() !== 0) {
                if (kernelInstance.exports.asyncify_stop_unwind)
                    kernelInstance.exports.asyncify_stop_unwind();
                else
                    kernelInstance.exports.asyncify_stop_rewind();
            }
        }
        
        try {
            // Re-enter Kernel
            let state = kernelInstance.exports.asyncify_get_state();
            console.log(`[JS] Calling wasm_loop state=${state}`);
            
            kernelInstance.exports.wasm_loop();
            refreshScreen();

            // Safety Check: wasm_loop should ONLY return via Unwind (State=1)
            state = kernelInstance.exports.asyncify_get_state();                
            if (state === 0) {
                console.error(`[JS] CRITICAL: Kernel exited main loop unexpectedly!`);
                break; 
            }
        } catch (e) {
            console.error("Kernel Error (Loop):", e);
            break;
        }
    }
}

async function initOS() 
{
    if (!window.crossOriginIsolated) {
        console.warn("Error: SharedArrayBuffer requires Cross-Origin-Isolated headers.");
    }

    // 1. Setup Graphics
    const canvas = document.getElementById('screen');
    canvasCtx = canvas.getContext('2d');

    // 2. Setup Input
    window.addEventListener('keydown', (e) => handleKeyEvent(e, true));
    window.addEventListener('keyup', (e) => handleKeyEvent(e, false));      

    try {
        // 3. Load Assets
        const [wasmRes, cpioRes] = await Promise.all([
            fetch('toyos.wasm'),
            fetch('initramfs.cpio')
        ]);
        const wasmBuffer = await wasmRes.arrayBuffer();
        const cpioBuffer = await cpioRes.arrayBuffer();
        
        // 4. Initramfs Setup (at 10MB)
        const CPIO_BASE_ADDR = 0xA00000; 
        const memView = new Uint8Array(wasmMemory.buffer);
        memView.set(new Uint8Array(cpioBuffer), CPIO_BASE_ADDR);     
        console.log(`[JS] Loaded Initramfs to 0x${CPIO_BASE_ADDR.toString(16)}`);   
        
        // 5. Instantiate Wasm with Kernel Imports
        const obj = await WebAssembly.instantiate(wasmBuffer, createKernelImports());
        console.log("[JS] Wasm Instantiated. Starting Kernel...");
        
        kernelInstance = obj.instance;
        
        // Expose to window for Debugging in Console!
        window.kernelInstance = kernelInstance; 
        window.userProcs = userProcs;

        // 6. Boot Kernel (Entry)
        kernelInstance.exports.wasm_entry(CPIO_BASE_ADDR, cpioBuffer.byteLength);
        kernelInstance.exports.wasm_loop(); // First run to initialize Scheduler
        refreshScreen();

        // 7. Start Scheduler Loop
        await runSchedulerLoop();

    } catch (error) {
        console.error("OS Boot Failed:", error);
    }
}

// Entry Point
document.addEventListener("DOMContentLoaded", initOS, false);