#ifndef ACRT_TIMER_H
#define ACRT_TIMER_H

#include "acrt_types.h"
#include <stdint.h>

// Timer ID type
typedef uint32_t timer_id;

#define TIMER_ID_INVALID ((timer_id)0)

// Timer operations
// All timers are owned by the calling actor and are automatically cancelled when the actor dies

// One-shot: wake current actor after delay
// Timer message: class=ACRT_MSG_TIMER, tag=timer_id, no payload
// Use acrt_msg_is_timer() to check, acrt_msg_decode() to get timer_id from tag
acrt_status acrt_timer_after(uint32_t delay_us, timer_id *out);

// Periodic: wake current actor every interval
// Timer message: class=ACRT_MSG_TIMER, tag=timer_id, no payload
// Use acrt_msg_is_timer() to check, acrt_msg_decode() to get timer_id from tag
acrt_status acrt_timer_every(uint32_t interval_us, timer_id *out);

// Cancel timer
acrt_status acrt_timer_cancel(timer_id id);

#endif // ACRT_TIMER_H
