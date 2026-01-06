#include "rt_runtime.h"
#include "rt_actor.h"
#include "rt_scheduler.h"
#include "rt_link.h"
#include "rt_log.h"
#include "rt_static_config.h"
#include <stdio.h>
#include <stdlib.h>

// Forward declarations for subsystems
extern rt_status rt_ipc_init(void);
extern rt_status rt_file_init(void);
extern void rt_file_cleanup(void);
extern rt_status rt_net_init(void);
extern void rt_net_cleanup(void);
extern rt_status rt_timer_init(void);
extern void rt_timer_cleanup(void);
extern rt_status rt_bus_init(void);
extern void rt_bus_cleanup(void);
extern rt_status rt_link_init(void);
extern void rt_link_cleanup(void);

rt_status rt_init(void) {
    // Initialize actor subsystem
    rt_status status = rt_actor_init();
    if (RT_FAILED(status)) {
        return status;
    }

    // Initialize scheduler
    status = rt_scheduler_init();
    if (RT_FAILED(status)) {
        rt_actor_cleanup();
        return status;
    }

    // Initialize IPC pools
    status = rt_ipc_init();
    if (RT_FAILED(status)) {
        rt_scheduler_cleanup();
        rt_actor_cleanup();
        return status;
    }

    // Initialize link subsystem
    status = rt_link_init();
    if (RT_FAILED(status)) {
        rt_scheduler_cleanup();
        rt_actor_cleanup();
        return status;
    }

    // Initialize file I/O subsystem
    status = rt_file_init();
    if (RT_FAILED(status)) {
        rt_link_cleanup();
        rt_scheduler_cleanup();
        rt_actor_cleanup();
        return status;
    }

    // Initialize network I/O subsystem
    status = rt_net_init();
    if (RT_FAILED(status)) {
        rt_file_cleanup();
        rt_link_cleanup();
        rt_scheduler_cleanup();
        rt_actor_cleanup();
        return status;
    }

    // Initialize timer subsystem
    status = rt_timer_init();
    if (RT_FAILED(status)) {
        rt_net_cleanup();
        rt_file_cleanup();
        rt_link_cleanup();
        rt_scheduler_cleanup();
        rt_actor_cleanup();
        return status;
    }

    // Initialize bus subsystem
    status = rt_bus_init();
    if (RT_FAILED(status)) {
        rt_timer_cleanup();
        rt_net_cleanup();
        rt_file_cleanup();
        rt_link_cleanup();
        rt_scheduler_cleanup();
        rt_actor_cleanup();
        return status;
    }

    return RT_SUCCESS;
}

void rt_run(void) {
    rt_scheduler_run();
}

void rt_shutdown(void) {
    rt_scheduler_shutdown();
}

void rt_cleanup(void) {
    rt_bus_cleanup();
    rt_timer_cleanup();
    rt_net_cleanup();
    rt_file_cleanup();
    rt_link_cleanup();
    rt_scheduler_cleanup();
    rt_actor_cleanup();
}

actor_id rt_spawn(actor_fn fn, void *arg) {
    actor_config cfg = RT_ACTOR_CONFIG_DEFAULT;
    cfg.stack_size = RT_DEFAULT_STACK_SIZE;
    return rt_spawn_ex(fn, arg, &cfg);
}

actor_id rt_spawn_ex(actor_fn fn, void *arg, const actor_config *cfg) {
    if (!fn) {
        return ACTOR_ID_INVALID;
    }

    // Copy config field by field instead of struct copy to avoid alignment issues
    // (struct copy may use SIMD instructions requiring 16-byte alignment)
    actor_config actual_cfg;
    actual_cfg.stack_size = cfg->stack_size;
    actual_cfg.priority = cfg->priority;
    actual_cfg.name = cfg->name;
    actual_cfg.malloc_stack = cfg->malloc_stack;
    if (actual_cfg.stack_size == 0) {
        actual_cfg.stack_size = RT_DEFAULT_STACK_SIZE;
    }

    actor *a = rt_actor_alloc(fn, arg, &actual_cfg);
    if (!a) {
        return ACTOR_ID_INVALID;
    }

    return a->id;
}

_Noreturn void rt_exit(void) {
    actor *current = rt_actor_current();
    if (current) {
        RT_LOG_DEBUG("Actor %u (%s) exiting", current->id,
                     current->name ? current->name : "unnamed");

        // Mark exit reason and actor state
        // Scheduler will clean up resources - don't free stack here!
        current->exit_reason = RT_EXIT_NORMAL;
        current->state = ACTOR_STATE_DEAD;
    }

    // Yield back to scheduler and never return
    rt_scheduler_yield();

    // Should never reach here
    RT_LOG_ERROR("rt_exit: returned from scheduler yield");
    abort();
}

_Noreturn void rt_exit_crash(void) {
    actor *current = rt_actor_current();
    if (current) {
        RT_LOG_ERROR("Actor %u (%s) returned without calling rt_exit()",
                     current->id, current->name ? current->name : "unnamed");

        // Mark as crashed - linked/monitoring actors will be notified
        current->exit_reason = RT_EXIT_CRASH;
        current->state = ACTOR_STATE_DEAD;
    }

    // Yield back to scheduler and never return
    rt_scheduler_yield();

    // Should never reach here
    RT_LOG_ERROR("rt_exit_crash: returned from scheduler yield");
    abort();
}

actor_id rt_self(void) {
    actor *current = rt_actor_current();
    return current ? current->id : ACTOR_ID_INVALID;
}

void rt_yield(void) {
    rt_scheduler_yield();
}

bool rt_actor_alive(actor_id id) {
    actor *a = rt_actor_get(id);
    return a != NULL && a->state != ACTOR_STATE_DEAD;
}
