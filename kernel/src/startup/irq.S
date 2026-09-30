.syntax unified
.cpu cortex-a9
.arm

.global minemu_irq_trampoline
.type minemu_irq_trampoline, %function
.extern minemu_irq_dispatch

minemu_irq_trampoline:
    // Preserve 88 bytes of stack space
    sub sp, sp, #88

    // Save values of registers 0-12
    stmia sp, {r0-r12}

    // Save IRQ lr
    str lr, [sp, #52]

    // Save SPSR register
    mrs r1, spsr
    str r1, [sp, #56]

    // Figure out where the interruption came from
    ldr r1, =0x10000000
    ldr r1, [r1, #8]
    str r1, [sp, #60]

    // Save interruption instructuon address
    sub r1, lr, #4
    str r1, [sp, #64]

    // Zero fields that don't matter to IRQs
    mov r1, #0
    str r1, [sp, #68]
    str r1, [sp, #72]

    // Save interrupted mode sp and lr
    add r1, sp, #76
    stmia r1, {sp, lr}^

    // Zero reserved field
    mov r1, #0
    str r1, [sp, #84]

    // Pass trap frame to C
    mov r0, sp
    bl minemu_irq_dispatch
    mov sp, r0

    // Restore IRQ return lr
    ldr lr, [sp, #52]

    // Restore SPSR
    ldr r1, [sp, #56]
    msr spsr_cxsf, r1

    // Restore interrupted mode sp and lr
    add r1, sp, #76
    ldmia r1, {sp, lr}^

    // Restore registers 0-12
    ldmia sp, {r0-r12}

    // Destroy trap frame, and return from IRQ
    add sp, sp, #88
    subs pc, lr, #4
