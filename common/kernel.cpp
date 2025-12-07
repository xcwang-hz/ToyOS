extern "C" {
    void host_console_log(const char* ptr);
}

extern "C" void kernel_entry() {
    host_console_log("ToyOS");
}