// Webots Crazyflie HAL Implementation
//
// Hardware abstraction for Webots simulation of Bitcraze Crazyflie.

#include "../hal.h"
#include <webots/robot.h>
#include <webots/motor.h>
#include <webots/gyro.h>
#include <webots/inertial_unit.h>
#include <webots/gps.h>

// ----------------------------------------------------------------------------
// Hardware handles
// ----------------------------------------------------------------------------

static WbDeviceTag g_motors[NUM_MOTORS];
static WbDeviceTag g_gyro;
static WbDeviceTag g_imu;
static WbDeviceTag g_gps;

// Motor direction signs (Crazyflie motor rotation directions)
static const float MOTOR_SIGNS[NUM_MOTORS] = {-1.0f, 1.0f, -1.0f, 1.0f};

// ----------------------------------------------------------------------------
// Platform Lifecycle
// ----------------------------------------------------------------------------

int hal_init(void) {
    wb_robot_init();

    // Initialize motors
    const char *motor_names[NUM_MOTORS] = {
        "m1_motor", "m2_motor", "m3_motor", "m4_motor"
    };

    for (int i = 0; i < NUM_MOTORS; i++) {
        g_motors[i] = wb_robot_get_device(motor_names[i]);
        if (g_motors[i] == 0) {
            return -1;
        }
        wb_motor_set_position(g_motors[i], INFINITY);
        wb_motor_set_velocity(g_motors[i], 0.0);
    }

    // Initialize sensors
    g_gyro = wb_robot_get_device("gyro");
    g_imu = wb_robot_get_device("inertial_unit");
    g_gps = wb_robot_get_device("gps");

    if (g_gyro == 0 || g_imu == 0 || g_gps == 0) {
        return -1;
    }

    wb_gyro_enable(g_gyro, TIME_STEP_MS);
    wb_inertial_unit_enable(g_imu, TIME_STEP_MS);
    wb_gps_enable(g_gps, TIME_STEP_MS);

    return 0;
}

void hal_cleanup(void) {
    wb_robot_cleanup();
}

void hal_calibrate(void) {
    // No-op: Webots sensors don't need calibration
}

void hal_arm(void) {
    // No-op: Webots motors are always ready
}

void hal_disarm(void) {
    // Stop all motors
    for (int i = 0; i < NUM_MOTORS; i++) {
        wb_motor_set_velocity(g_motors[i], 0.0);
    }
}

// ----------------------------------------------------------------------------
// Sensor Interface
// ----------------------------------------------------------------------------

void hal_read_imu(imu_data_t *imu) {
    const double *gyro = wb_gyro_get_values(g_gyro);
    const double *rpy = wb_inertial_unit_get_roll_pitch_yaw(g_imu);
    const double *pos = wb_gps_get_values(g_gps);

    // Attitude (Webots inertial_unit provides fused Euler angles)
    imu->roll = (float)rpy[0];
    imu->pitch = (float)rpy[1];
    imu->yaw = (float)rpy[2];

    // Angular rates (body frame)
    imu->gyro_x = (float)gyro[0];
    imu->gyro_y = (float)gyro[1];
    imu->gyro_z = (float)gyro[2];

    // Position (world frame)
    imu->x = (float)pos[0];
    imu->y = (float)pos[1];
    imu->altitude = (float)pos[2];
}

// ----------------------------------------------------------------------------
// Motor Interface
// ----------------------------------------------------------------------------

// Crazyflie X-configuration mixer
//
// Motor layout:
//         Front
//       M2    M3
//         \  /
//          \/
//          /\.
//         /  \.
//       M1    M4
//         Rear
//
// Motor rotation: M1(CCW), M2(CW), M3(CCW), M4(CW)

void hal_write_torque(const torque_cmd_t *cmd) {
    // Platform-specific adjustment: negate pitch for Crazyflie coordinate frame
    float pitch = -cmd->pitch;

    // Apply mixer: convert torque to individual motor commands
    float motors[NUM_MOTORS];
    motors[0] = cmd->thrust - cmd->roll + pitch + cmd->yaw;  // M1 (rear-left)
    motors[1] = cmd->thrust - cmd->roll - pitch - cmd->yaw;  // M2 (front-left)
    motors[2] = cmd->thrust + cmd->roll - pitch + cmd->yaw;  // M3 (front-right)
    motors[3] = cmd->thrust + cmd->roll + pitch - cmd->yaw;  // M4 (rear-right)

    // Clamp and output to Webots motors
    for (int i = 0; i < NUM_MOTORS; i++) {
        float clamped = CLAMPF(motors[i], 0.0f, 1.0f);
        wb_motor_set_velocity(g_motors[i],
            MOTOR_SIGNS[i] * clamped * MOTOR_MAX_VELOCITY);
    }
}

// ----------------------------------------------------------------------------
// Simulated Time Interface
// ----------------------------------------------------------------------------

bool hal_step(void) {
    return wb_robot_step(TIME_STEP_MS) != -1;
}
