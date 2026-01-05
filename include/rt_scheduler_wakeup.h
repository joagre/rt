#ifndef RT_SCHEDULER_WAKEUP_H
#define RT_SCHEDULER_WAKEUP_H

#include "rt_types.h"

// Platform abstraction for scheduler wakeup mechanism
// Linux: eventfd, FreeRTOS: binary semaphore

rt_status rt_scheduler_wakeup_init(void);
void rt_scheduler_wakeup_cleanup(void);

// Signal the scheduler to wake up (called by I/O threads after posting completion)
void rt_scheduler_wakeup_signal(void);

// Wait for wakeup signal (called by scheduler when idle)
void rt_scheduler_wakeup_wait(void);

#endif // RT_SCHEDULER_WAKEUP_H
