// Motor actor - Output layer
//
// Subscribes to torque bus, writes to hardware via HAL.
// The HAL handles mixing (converting torque to individual motor commands).

#include "motor_actor.h"
#include "types.h"
#include "config.h"
#include "hal/hal.h"
#include "hive_runtime.h"
#include "hive_bus.h"
#include <assert.h>

static bus_id s_torque_bus;

void motor_actor_init(bus_id torque_bus) {
    s_torque_bus = torque_bus;
}

// Real hardware only: startup delay and hard cutoff for first flight testing
#ifndef SIMULATED_TIME
#define MOTOR_STARTUP_DELAY_TICKS  15000  // 60 seconds at 250Hz
#define MOTOR_CUTOFF_TICKS         1250   // 5 seconds at 250Hz
#endif

void motor_actor(void *arg) {
    (void)arg;

    BUS_SUBSCRIBE(s_torque_bus);

#ifndef SIMULATED_TIME
    int tick_count = 0;
#endif

    while (1) {
        torque_cmd_t torque;

        // Block until torque command available
        BUS_READ_WAIT(s_torque_bus, &torque);

#ifndef SIMULATED_TIME
        tick_count++;

        // Startup delay: keep motors off for 60 seconds
        if (tick_count <= MOTOR_STARTUP_DELAY_TICKS) {
            torque = (torque_cmd_t)TORQUE_CMD_ZERO;
        }
        // Hard cutoff 5 seconds after startup delay
        else if (tick_count > MOTOR_STARTUP_DELAY_TICKS + MOTOR_CUTOFF_TICKS) {
            torque = (torque_cmd_t)TORQUE_CMD_ZERO;
        }
#endif

        // HAL handles mixing and hardware output
        hal_write_torque(&torque);
    }
}
