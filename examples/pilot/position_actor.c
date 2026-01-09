// Position actor - Horizontal position hold control
//
// Subscribes to state bus, runs position PI with velocity damping,
// publishes angle setpoints for the angle actor to track.
//
// Uses the same PI + velocity damping pattern as altitude control.
// Coordinate system:
//   - Positive pitch → accelerate in -X direction (nose down)
//   - Positive roll → accelerate in +Y direction (right wing down)

#include "position_actor.h"
#include "types.h"
#include "config.h"
#include "pid.h"
#include "hive_runtime.h"
#include "hive_bus.h"
#include <stdio.h>

static bus_id s_state_bus;
static bus_id s_angle_setpoint_bus;

void position_actor_init(bus_id state_bus, bus_id angle_setpoint_bus) {
    s_state_bus = state_bus;
    s_angle_setpoint_bus = angle_setpoint_bus;
}

void position_actor(void *arg) {
    (void)arg;

    hive_bus_subscribe(s_state_bus);

    pid_state_t x_pid, y_pid;
    pid_init_full(&x_pid, POS_PID_KP, POS_PID_KI, POS_PID_KD, 0.5f, POS_PID_OMAX);
    pid_init_full(&y_pid, POS_PID_KP, POS_PID_KI, POS_PID_KD, 0.5f, POS_PID_OMAX);

    int count = 0;

    while (1) {
        state_estimate_t state;
        size_t len;

        if (hive_bus_read(s_state_bus, &state, sizeof(state), &len).code == HIVE_OK) {
            // Position control (PI): position error → velocity command
            float vel_cmd_x = pid_update(&x_pid, TARGET_X, state.x, TIME_STEP_S);
            float vel_cmd_y = pid_update(&y_pid, TARGET_Y, state.y, TIME_STEP_S);

            // Velocity damping: oppose difference between commanded and actual velocity
            // pitch controls X motion, roll controls Y motion
            float pitch_cmd = -HVEL_DAMPING_GAIN * (vel_cmd_x - state.x_velocity);
            float roll_cmd  =  HVEL_DAMPING_GAIN * (vel_cmd_y - state.y_velocity);

            // Clamp to maximum tilt angle for safety
            pitch_cmd = CLAMPF(pitch_cmd, -MAX_TILT_ANGLE, MAX_TILT_ANGLE);
            roll_cmd  = CLAMPF(roll_cmd,  -MAX_TILT_ANGLE, MAX_TILT_ANGLE);

            angle_setpoint_t setpoint = {
                .roll = roll_cmd,
                .pitch = pitch_cmd,
                .yaw = 0.0f
            };
            hive_bus_publish(s_angle_setpoint_bus, &setpoint, sizeof(setpoint));

            if (++count % DEBUG_PRINT_INTERVAL == 0) {
                printf("[POS] x=%.2f y=%.2f roll_sp=%.2f pitch_sp=%.2f\n",
                       state.x, state.y, roll_cmd * RAD_TO_DEG, pitch_cmd * RAD_TO_DEG);
            }
        }

        hive_yield();
    }
}
