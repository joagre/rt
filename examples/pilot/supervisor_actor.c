// Supervisor actor - Flight authority and safety monitoring
//
// Controls flight lifecycle:
// 1. Startup delay (real hardware only)
// 2. Send START to waypoint actor
// 3. Flight duration timer
// 4. Send LANDING to altitude actor
// 5. Wait for LANDED, then send STOP to motor actor

#include "supervisor_actor.h"
#include "notifications.h"
#include "config.h"
#include "hive_runtime.h"
#include "hive_ipc.h"
#include "hive_timer.h"
#include "hive_log.h"

// Flight duration per profile (supervisor decides when to land)
#if FLIGHT_PROFILE == FLIGHT_PROFILE_FIRST_TEST
#define FLIGHT_DURATION_US  (10 * 1000000)  // 10 seconds
#elif FLIGHT_PROFILE == FLIGHT_PROFILE_ALTITUDE
#define FLIGHT_DURATION_US  (40 * 1000000)  // 40 seconds
#elif FLIGHT_PROFILE == FLIGHT_PROFILE_FULL_3D
#define FLIGHT_DURATION_US  (30 * 1000000)  // 30 seconds
#else
#define FLIGHT_DURATION_US  (20 * 1000000)  // Default: 20 seconds
#endif

static actor_id s_waypoint_actor;
static actor_id s_altitude_actor;
static actor_id s_motor_actor;

void supervisor_actor_init(actor_id waypoint_actor, actor_id altitude_actor, actor_id motor_actor) {
    s_waypoint_actor = waypoint_actor;
    s_altitude_actor = altitude_actor;
    s_motor_actor = motor_actor;
}

void supervisor_actor(void *arg) {
    (void)arg;

#ifndef SIMULATED_TIME
    // Real hardware: wait for startup delay before allowing flight
    HIVE_LOG_INFO("[SUP] Startup delay: 60 seconds");

    // Sleep in 10-second intervals with progress logging
    for (int i = 6; i > 0; i--) {
        hive_sleep(10 * 1000000);  // 10 seconds
        if (i > 1) {
            HIVE_LOG_INFO("[SUP] Startup delay: %d seconds remaining", (i - 1) * 10);
        }
    }

    HIVE_LOG_INFO("[SUP] Startup delay complete - sending START");
#else
    // Simulation: no delay needed
    HIVE_LOG_INFO("[SUP] Simulation mode - sending START immediately");
#endif

    // Notify waypoint actor to begin flight sequence
    // Tag carries the notification type, no payload needed
    hive_ipc_notify(s_waypoint_actor, NOTIFY_FLIGHT_START, NULL, 0);
    HIVE_LOG_INFO("[SUP] Flight authorized");

#ifndef SIMULATED_TIME
    // Real hardware: flight window with hard cutoff
    // Wait for either LANDED notification or timeout (whichever comes first)
    HIVE_LOG_INFO("[SUP] Flight window: 12 seconds");

    timer_id timeout_timer;
    hive_timer_after(12 * 1000000, &timeout_timer);  // 12 seconds

    hive_message msg;
    hive_ipc_recv(&msg, -1);  // Wait for any message

    if (hive_msg_is_timer(&msg)) {
        HIVE_LOG_INFO("[SUP] Flight window expired - stopping motors");
    } else if (msg.tag == NOTIFY_FLIGHT_LANDED) {
        HIVE_LOG_INFO("[SUP] Landing confirmed - stopping motors");
        hive_timer_cancel(timeout_timer);
    }
#else
    // Simulation: flight duration timer, then initiate landing
    HIVE_LOG_INFO("[SUP] Flight duration: %.0f seconds", FLIGHT_DURATION_US / 1000000.0f);

    timer_id flight_timer;
    hive_timer_after(FLIGHT_DURATION_US, &flight_timer);

    hive_message msg;
    hive_ipc_recv_match(HIVE_SENDER_ANY, HIVE_MSG_TIMER, flight_timer, &msg, -1);
    HIVE_LOG_INFO("[SUP] Flight duration complete - initiating landing");
    hive_ipc_notify(s_altitude_actor, NOTIFY_LANDING, NULL, 0);

    // Wait for LANDED notification
    hive_ipc_recv_match(HIVE_SENDER_ANY, HIVE_MSG_NOTIFY, NOTIFY_FLIGHT_LANDED, &msg, -1);
    HIVE_LOG_INFO("[SUP] Landing confirmed - stopping motors");
#endif

    // Send STOP to motor actor
    hive_ipc_notify(s_motor_actor, NOTIFY_FLIGHT_STOP, NULL, 0);

    hive_exit();
}
