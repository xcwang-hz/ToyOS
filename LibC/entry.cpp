#include <Kernel/Syscall.h>
#include <AK/StringImpl.h>

extern "C" int main(int, char**);

int errno;
char** environ;

// extern "C" void __malloc_init();
// extern "C" void __stdio_init();

#if WASM
extern "C" volatile int32_t wasm_syscall_params[5] = {0, 1, 0, 0, 0}; // ( ready, arg1, arg2, arg3, retval）
struct AsyncifyContext {
    // Defines the start and end of the buffer where Asyncify saves data.
    // Binaryen expects { void* start; void* end; } structure usually.
    // Or simpler: just a raw byte array.
    void* stack_start;
    void* stack_end;
    
    // Reserve enough space! 1KB is usually enough for simple kernels,
    // but if you have deep recursion, increase this.    
    uint8_t buffer[1024];
};    

extern "C" {
    AsyncifyContext asyncify_ctx;
    void* wasm_user_asyncify_ctx() { 
        return &asyncify_ctx;
    }
}
#endif

extern "C" int _start()
{
    errno = 0;

    // __stdio_init();
    // __malloc_init();

    // StringImpl::initialize_globals();

#if WASM
    asyncify_ctx.stack_start = asyncify_ctx.buffer;
    asyncify_ctx.stack_end = asyncify_ctx.buffer + sizeof(asyncify_ctx.buffer);
#endif

    int status = 254;
    int argc;
    char** argv;
    int rc = syscall(SC_get_arguments, &argc, &argv);
    if (rc < 0)
        goto epilogue;
    //rc = syscall(SC_get_environment, &environ);
    // if (rc < 0)
    //     goto epilogue;
    status = main(argc, argv);

    // fflush(stdout);
    // fflush(stderr);

epilogue:
    syscall(SC_exit, status);

    // Birger's birthday <3
    return 20150614;
}
