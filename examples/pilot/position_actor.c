// Position actor - Horizontal position hold control
//
// Subscribes to state bus, runs simple PD position control,
// publishes angle setpoints for the angle actor to track.
//
// Internal convention (intuitive physics):
//   - Positive position error → positive command → accelerate toward target
//
// Webots/aerospace convention (attitude → acceleration):
//   - Positive pitch (nose up) → thrust tilts back → -X acceleration
//   - Positive roll (right wing down) → thrust tilts left → -Y acceleration
//
// We negate both roll and pitch when publishing to convert from our
// internal convention to Webots convention.

#include "position_actor.h"
#include "types.h"
#include "config.h"
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

    int count = 0;

    while (1) {
        state_estimate_t state;
        size_t len;

        if (hive_bus_read(s_state_bus, &state, sizeof(state), &len).code == HIVE_OK) {
            float pitch_cmd = 0.0f;
            float roll_cmd = 0.0f;

            // Only enable position control after reaching stable hover
            if (state.altitude > 0.8f) {
                // Simple PD controller - very gentle to avoid destabilizing
                float x_error = TARGET_X - state.x;
                float y_error = TARGET_Y - state.y;

                pitch_cmd = POS_KP * x_error - POS_KD * state.x_velocity;
                roll_cmd  = POS_KP * y_error - POS_KD * state.y_velocity;

                // Clamp to maximum tilt angle for safety
                pitch_cmd = CLAMPF(pitch_cmd, -MAX_TILT_ANGLE, MAX_TILT_ANGLE);
                roll_cmd  = CLAMPF(roll_cmd,  -MAX_TILT_ANGLE, MAX_TILT_ANGLE);
            }

            // Sign conversion for Webots Crazyflie (matching Bitcraze):
            // - Roll is negated: positive Y error → negative roll (left wing down) → +Y accel
            // - Pitch is NOT negated: positive X error → positive pitch → forward
            angle_setpoint_t setpoint = {
                .roll = -roll_cmd,
                .pitch = pitch_cmd,
                .yaw = 0.0f
            };
            hive_bus_publish(s_angle_setpoint_bus, &setpoint, sizeof(setpoint));

            if (++count % DEBUG_PRINT_INTERVAL == 0) {
                printf("[POS] x=%.2f y=%.2f vx=%.2f vy=%.2f pitch=%.1f roll=%.1f\n",
                       state.x, state.y, state.x_velocity, state.y_velocity,
                       pitch_cmd * RAD_TO_DEG, roll_cmd * RAD_TO_DEG);
            }
        }

        hive_yield();
    }
}
