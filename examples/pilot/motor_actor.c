// Motor actor - Mixer and safety layer
//
// Subscribes to torque bus, applies mixer, enforces limits, implements watchdog,
// writes to hardware.

#include "motor_actor.h"
#include "config.h"
#include "hive_runtime.h"
#include "hive_bus.h"
#include <stdio.h>
#include <stdbool.h>

static bus_id s_torque_bus;
static motor_write_fn s_write_fn;

// Motor mixer for Crazyflie "+" configuration.
//
//         Front
//           M1
//            |
//     M4 ----+---- M2
//            |
//           M3
//          Rear
static void mixer_apply(const torque_cmd_t *torque, motor_cmd_t *cmd) {
    cmd->motor[0] = torque->thrust - torque->pitch - torque->yaw;  // M1 (front)
    cmd->motor[1] = torque->thrust - torque->roll  + torque->yaw;  // M2 (right)
    cmd->motor[2] = torque->thrust + torque->pitch - torque->yaw;  // M3 (rear)
    cmd->motor[3] = torque->thrust + torque->roll  + torque->yaw;  // M4 (left)

    for (int i = 0; i < NUM_MOTORS; i++) {
        cmd->motor[i] = CLAMPF(cmd->motor[i], 0.0f, 1.0f);
    }
}

void motor_actor_init(bus_id torque_bus, motor_write_fn write_fn) {
    s_torque_bus = torque_bus;
    s_write_fn = write_fn;
}

void motor_actor(void *arg) {
    (void)arg;

    hive_bus_subscribe(s_torque_bus);

    int watchdog = 0;
    motor_cmd_t cmd = MOTOR_CMD_ZERO;
    bool armed = false;

    while (1) {
        torque_cmd_t torque;
        size_t len;

        if (hive_bus_read(s_torque_bus, &torque, sizeof(torque), &len).code == HIVE_OK) {
            watchdog = 0;

            mixer_apply(&torque, &cmd);
            armed = true;

            if (s_write_fn) {
                s_write_fn(&cmd);
            }
        } else {
            watchdog++;

            if (watchdog >= MOTOR_WATCHDOG_TIMEOUT && armed) {
                printf("MOTOR WATCHDOG: No commands for %dms - cutting motors!\n",
                       MOTOR_WATCHDOG_TIMEOUT * TIME_STEP_MS);
                cmd = (motor_cmd_t)MOTOR_CMD_ZERO;
                if (s_write_fn) {
                    s_write_fn(&cmd);
                }
                armed = false;
            }
        }

        hive_yield();
    }
}
