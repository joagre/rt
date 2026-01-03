#include "rt_context.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

// Forward declaration of assembly function
extern void rt_context_switch_asm(rt_context *from, rt_context *to);

// Helper structure to pass data through the initial context
typedef struct {
    void (*fn)(void *);
    void *arg;
} context_start_data;

// Wrapper function that calls the actor function and handles return
static void context_entry(void) {
    // When we first enter, r12 and r13 contain our function and argument
    // We need to extract them via inline assembly
    void (*fn)(void *);
    void *arg;

    __asm__ volatile (
        "movq %%r12, %0\n"
        "movq %%r13, %1\n"
        : "=r"(fn), "=r"(arg)
        :
        : "r12", "r13"
    );

    fn(arg);

    // If actor function returns, we should exit
    // For now, just hang (in full implementation, would call rt_exit)
    while (1) {
        __asm__ volatile("pause");
    }
}

void rt_context_init(rt_context *ctx, void *stack, size_t stack_size,
                     void (*fn)(void *), void *arg) {
    // Zero out context
    memset(ctx, 0, sizeof(rt_context));

    // Stack grows down on x86-64
    // Calculate stack top (align to 16 bytes as required by x86-64 ABI)
    uintptr_t stack_top = (uintptr_t)stack + stack_size;
    stack_top &= ~((uintptr_t)15);  // Align to 16 bytes

    // x86-64 ABI requires RSP to be aligned such that (RSP+8) % 16 == 0 at function entry
    // Since we'll be using 'ret' to jump to context_entry, and 'ret' pops 8 bytes,
    // we need RSP to be 8 bytes off 16-byte alignment before the ret
    // After ret, RSP will be 16-byte aligned as required

    // Store function and argument in callee-saved registers
    // These will be preserved across the context switch
    // Use a union to safely convert function pointer to void pointer
    union {
        void (*fn_ptr)(void *);
        void *obj_ptr;
    } fn_conv;
    fn_conv.fn_ptr = fn;

    union {
        void (*fn_ptr)(void);
        void *obj_ptr;
    } entry_conv;
    entry_conv.fn_ptr = context_entry;

    ctx->r12 = fn_conv.obj_ptr;
    ctx->r13 = arg;

    // Set instruction pointer to context_entry
    // We do this by pushing the return address onto the stack
    // When the context switch does 'ret', it will pop this address and jump to it
    stack_top -= sizeof(void *);
    *(void **)stack_top = entry_conv.obj_ptr;

    ctx->rsp = (void *)stack_top;
}

void rt_context_switch(rt_context *from, rt_context *to) {
    rt_context_switch_asm(from, to);
}
