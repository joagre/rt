// Motor actor - Mixer and output layer
//
// Subscribes to torque bus, applies mixer, enforces limits, writes to hardware.

#include "motor_actor.h"
#include "config.h"
#include "hive_runtime.h"
#include "hive_bus.h"
#include "hive_log.h"
#include <assert.h>

static bus_id s_torque_bus;
static write_motors_fn s_write_motors;

// Motor mixer - X-configuration quadcopter
//
// Motor layout (both platforms):
//
//         Front
//       M2    M3
//         \  /
//          \/
//          /\.
//         /  \.
//       M1    M4
//         Rear
//
// In X-config, each motor affects BOTH roll AND pitch.
// Sign conventions differ between platforms due to motor rotation directions.

#ifdef PLATFORM_STEVAL_DRONE01
// STEVAL-DRONE01 mixer
// Motor rotation: M1(CCW), M2(CW), M3(CCW), M4(CW)
static void mixer_apply(const torque_cmd_t *torque, motor_cmd_t *cmd) {
    cmd->motor[0] = torque->thrust + torque->roll + torque->pitch - torque->yaw;  // M1 (rear-left, CCW)
    cmd->motor[1] = torque->thrust + torque->roll - torque->pitch + torque->yaw;  // M2 (front-left, CW)
    cmd->motor[2] = torque->thrust - torque->roll - torque->pitch - torque->yaw;  // M3 (front-right, CCW)
    cmd->motor[3] = torque->thrust - torque->roll + torque->pitch + torque->yaw;  // M4 (rear-right, CW)

    for (int i = 0; i < NUM_MOTORS; i++) {
        cmd->motor[i] = CLAMPF(cmd->motor[i], 0.0f, 1.0f);
    }
}
#else
// Crazyflie mixer (Webots simulation)
// Motor rotation: M1(CCW), M2(CW), M3(CCW), M4(CW)
static void mixer_apply(const torque_cmd_t *torque, motor_cmd_t *cmd) {
    cmd->motor[0] = torque->thrust - torque->roll + torque->pitch + torque->yaw;  // M1 (rear-left)
    cmd->motor[1] = torque->thrust - torque->roll - torque->pitch - torque->yaw;  // M2 (front-left)
    cmd->motor[2] = torque->thrust + torque->roll - torque->pitch + torque->yaw;  // M3 (front-right)
    cmd->motor[3] = torque->thrust + torque->roll + torque->pitch - torque->yaw;  // M4 (rear-right)

    for (int i = 0; i < NUM_MOTORS; i++) {
        cmd->motor[i] = CLAMPF(cmd->motor[i], 0.0f, 1.0f);
    }
}
#endif

void motor_actor_init(bus_id torque_bus, write_motors_fn write_motors) {
    assert(write_motors != NULL);
    s_torque_bus = torque_bus;
    s_write_motors = write_motors;
}

void motor_actor(void *arg) {
    (void)arg;

    assert(HIVE_SUCCEEDED(hive_bus_subscribe(s_torque_bus)));

    motor_cmd_t cmd = MOTOR_CMD_ZERO;

    while (1) {
        torque_cmd_t torque;

        // Block until torque command available
        BUS_READ_WAIT(s_torque_bus, &torque);

        mixer_apply(&torque, &cmd);
        s_write_motors(&cmd);
    }
}
