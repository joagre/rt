#ifndef RT_RUNTIME_H
#define RT_RUNTIME_H

#include "rt_types.h"

// Runtime configuration
typedef struct {
    size_t default_stack_size;    // default actor stack, bytes
    size_t max_actors;            // maximum concurrent actors
    size_t completion_queue_size; // entries per I/O completion queue
    size_t max_buses;             // maximum concurrent buses
} rt_config;

#define RT_CONFIG_DEFAULT { \
    .default_stack_size = 65536, \
    .max_actors = 64, \
    .completion_queue_size = 64, \
    .max_buses = 32 \
}

// Initialize runtime (call once from main)
rt_status rt_init(const rt_config *cfg);

// Run scheduler (blocks until all actors exit or rt_shutdown called)
void rt_run(void);

// Request graceful shutdown
void rt_shutdown(void);

// Cleanup runtime
void rt_cleanup(void);

// Actor lifecycle API

// Spawn a new actor with default configuration
actor_id rt_spawn(actor_fn fn, void *arg);

// Spawn with explicit configuration
actor_id rt_spawn_ex(actor_fn fn, void *arg, const actor_config *cfg);

// Terminate current actor
_Noreturn void rt_exit(void);

// Get current actor's ID
actor_id rt_self(void);

// Yield to scheduler
void rt_yield(void);

// Check if actor is alive
bool rt_actor_alive(actor_id id);

#endif // RT_RUNTIME_H
