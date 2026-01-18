// Motor actor - Output layer
//
// Subscribes to torque bus, writes to hardware via HAL.
// The HAL handles mixing (converting torque to individual motor commands).
// Uses hive_select() to wait on torque bus OR STOP notification simultaneously,
// ensuring immediate response to STOP commands (critical for safety).

#include "motor_actor.h"
#include "pilot_buses.h"
#include "notifications.h"
#include "types.h"
#include "config.h"
#include "hal/hal.h"
#include "hive_runtime.h"
#include "hive_bus.h"
#include "hive_ipc.h"
#include "hive_select.h"
#include <assert.h>
#include <string.h>

// Actor state - initialized by motor_actor_init
typedef struct {
    bus_id torque_bus;
} motor_state;

void *motor_actor_init(void *init_args) {
    const pilot_buses *buses = init_args;
    static motor_state state;
    state.torque_bus = buses->torque_bus;
    return &state;
}

void motor_actor(void *args, const hive_spawn_info *siblings,
                 size_t sibling_count) {
    (void)siblings;
    (void)sibling_count;

    motor_state *state = args;

    hive_status status = hive_bus_subscribe(state->torque_bus);
    assert(HIVE_SUCCEEDED(status));

    bool stopped = false;

    // Set up hive_select() sources: torque bus + STOP notification
    enum { SEL_TORQUE, SEL_STOP };
    hive_select_source sources[] = {
        [SEL_TORQUE] = {HIVE_SEL_BUS, .bus = state->torque_bus},
        [SEL_STOP] = {HIVE_SEL_IPC, .ipc = {HIVE_SENDER_ANY, HIVE_MSG_NOTIFY,
                                            NOTIFY_FLIGHT_STOP}},
    };

    while (1) {
        torque_cmd_t torque;

        // Wait for torque command OR STOP notification (unified event waiting)
        hive_select_result result;
        hive_select(sources, 2, &result, -1);

        if (result.index == SEL_STOP) {
            // STOP received - respond immediately
            stopped = true;
            torque = (torque_cmd_t)TORQUE_CMD_ZERO;
            hal_write_torque(&torque);
            continue; // Loop back to wait for next event
        }

        // SEL_TORQUE: Copy torque data from select result
        assert(result.bus.len == sizeof(torque));
        memcpy(&torque, result.bus.data, sizeof(torque));

        if (stopped) {
            torque = (torque_cmd_t)TORQUE_CMD_ZERO;
        }

        hal_write_torque(&torque);
    }
}
