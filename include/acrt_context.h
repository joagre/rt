#ifndef ACRT_CONTEXT_H
#define ACRT_CONTEXT_H

#include <stddef.h>

// Context for x86-64
// Stores callee-saved registers: rbx, rbp, r12-r15, and rsp
typedef struct {
    void *rsp;  // Stack pointer
    void *rbx;
    void *rbp;
    void *r12;
    void *r13;
    void *r14;
    void *r15;
} acrt_context;

// Switch from 'from' context to 'to' context
// Saves current context in 'from' and restores 'to'
void acrt_context_switch(acrt_context *from, acrt_context *to);

// Initialize a new context
// stack: pointer to top of stack (stack grows down)
// stack_size: size of stack in bytes
// fn: function to execute
// arg: argument to pass to function
void acrt_context_init(acrt_context *ctx, void *stack, size_t stack_size,
                     void (*fn)(void *), void *arg);

#endif // ACRT_CONTEXT_H
