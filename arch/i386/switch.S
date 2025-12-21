.section .text
.global context_switch_asm

# void context_switch_asm(uint32_t* prev_save_addr, uint32_t next_stack_val);
#
# Stack Layout (after pushf + pusha):
# 44(%esp) = next_stack_val (Param 2: Value of the new stack top)
# 40(%esp) = prev_save_addr (Param 1: Address to save the old stack top)
# 36(%esp) = Return Address (Caller's return address)
# 32(%esp) = EFLAGS
# 0(%esp)  = PUSHA (8 registers * 4 bytes = 32 bytes)

context_switch_asm:
    # 1. Save current context
    pushf                   # Push EFLAGS
    pusha                   # Push General Purpose Registers

    # --- Handle Param 1 (Save old stack) ---
    # Get prev_save_addr (Param 1) into EAX
    # Equivalent to Intel: mov eax, [esp + 40]
    mov 40(%esp), %eax

    # Save current ESP to the memory location pointed to by EAX
    # Equivalent to Intel: mov [eax], esp
    mov %esp, (%eax)

    # --- Handle Param 2 (Switch to new stack) ---
    # Get next_stack_val (Param 2) into EAX
    # Equivalent to Intel: mov eax, [esp + 44]
    mov 44(%esp), %eax

    # Load EAX into ESP (The actual stack switch happens here!)
    # Equivalent to Intel: mov esp, eax
    mov %eax, %esp

    # 3. Restore new context
    # The new stack is now active, so these pop from the NEW process's stack
    popa                    # Restore General Purpose Registers
    popf                    # Restore EFLAGS

    # 4. Jump to the new task's code
    ret