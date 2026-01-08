#ifndef ACRT_SCHEDULER_H
#define ACRT_SCHEDULER_H

#include "acrt_types.h"
#include <stdbool.h>

// Initialize scheduler
acrt_status acrt_scheduler_init(void);

// Cleanup scheduler
void acrt_scheduler_cleanup(void);

// Run scheduler (blocks until all actors exit or shutdown requested)
void acrt_scheduler_run(void);

// Request shutdown
void acrt_scheduler_shutdown(void);

// Yield control back to scheduler (called by actors)
void acrt_scheduler_yield(void);

// Check if shutdown was requested
bool acrt_scheduler_should_stop(void);

// Get epoll file descriptor for event loop (for subsystems to register I/O)
int acrt_scheduler_get_epoll_fd(void);

#endif // ACRT_SCHEDULER_H
