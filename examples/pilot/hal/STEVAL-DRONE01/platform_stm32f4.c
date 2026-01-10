// STM32F4 Platform Layer for Pilot Example
//
// Implements the platform interface using STEVAL-DRONE01 HAL drivers.

#include "platform_stm32f4.h"
#include "system_config.h"
#include "gpio_config.h"
#include "spi1.h"
#include "i2c1.h"
#include "tim4.h"
#include "usart1.h"
#include "lsm6dsl.h"
#include "lis2mdl.h"
#include "lps22hd.h"
#include "motors.h"
#include "attitude.h"

#include <stdarg.h>
#include <stdbool.h>

// ----------------------------------------------------------------------------
// Configuration
// ----------------------------------------------------------------------------

#define CALIBRATION_SAMPLES     500     // Gyro calibration samples
#define BARO_CALIBRATION_SAMPLES 50     // Barometer calibration samples

// ----------------------------------------------------------------------------
// Static State
// ----------------------------------------------------------------------------

static bool s_initialized = false;
static bool s_calibrated = false;
static bool s_armed = false;

// Gyro bias (rad/s)
static float s_gyro_bias[3] = {0.0f, 0.0f, 0.0f};

// Current sensor data
static lsm6dsl_data_t s_accel;
static lsm6dsl_data_t s_gyro;
static lis2mdl_data_t s_mag;
static float s_altitude;
static float s_pressure;

// Attitude estimate
static attitude_t s_attitude;
static attitude_rates_t s_rates;

// ----------------------------------------------------------------------------
// Platform Interface
// ----------------------------------------------------------------------------

int platform_init(void) {
    // Initialize system clocks (84MHz), SysTick, DWT
    if (!system_init()) {
        return -1;
    }

    // Initialize I2C1 for magnetometer and barometer
    i2c1_init(I2C1_SPEED_400KHZ);

    // Initialize LSM6DSL (IMU) - SPI1 initialized internally
    if (!lsm6dsl_init(NULL)) {
        return -1;
    }

    // Initialize LIS2MDL (magnetometer)
    if (!lis2mdl_init(NULL)) {
        return -1;
    }

    // Initialize LPS22HD (barometer)
    if (!lps22hd_init(NULL)) {
        return -1;
    }

    // Initialize motors (TIM4 PWM)
    if (!motors_init(NULL)) {
        return -1;
    }

    // Initialize attitude filter
    attitude_config_t att_config = ATTITUDE_CONFIG_DEFAULT;
    att_config.use_mag = true;
    attitude_init(&att_config, NULL);

    s_initialized = true;
    s_calibrated = false;
    s_armed = false;

    return 0;
}

int platform_calibrate(void) {
    if (!s_initialized) {
        return -1;
    }

    // -------------------------------------------------------------------------
    // Gyro bias calibration - average readings while stationary
    // -------------------------------------------------------------------------
    float gyro_sum[3] = {0.0f, 0.0f, 0.0f};
    lsm6dsl_data_t accel, gyro;

    for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
        lsm6dsl_read_all(&accel, &gyro);
        gyro_sum[0] += gyro.x;
        gyro_sum[1] += gyro.y;
        gyro_sum[2] += gyro.z;
        system_delay_us(2500);  // ~400Hz
    }

    s_gyro_bias[0] = gyro_sum[0] / CALIBRATION_SAMPLES;
    s_gyro_bias[1] = gyro_sum[1] / CALIBRATION_SAMPLES;
    s_gyro_bias[2] = gyro_sum[2] / CALIBRATION_SAMPLES;

    // -------------------------------------------------------------------------
    // Barometer reference calibration
    // -------------------------------------------------------------------------
    float pressure_sum = 0.0f;

    for (int i = 0; i < BARO_CALIBRATION_SAMPLES; i++) {
        pressure_sum += lps22hd_read_pressure();
        system_delay_ms(20);
    }

    float ref_pressure = pressure_sum / BARO_CALIBRATION_SAMPLES;
    lps22hd_set_reference(ref_pressure);

    // -------------------------------------------------------------------------
    // Initialize attitude from accelerometer
    // -------------------------------------------------------------------------
    lsm6dsl_read_all(&accel, &gyro);
    float accel_arr[3] = {accel.x, accel.y, accel.z};

    attitude_t initial = {
        .roll = attitude_accel_roll(accel_arr),
        .pitch = attitude_accel_pitch(accel_arr),
        .yaw = 0.0f
    };
    attitude_reset(&initial);

    s_calibrated = true;
    return 0;
}

void platform_read_imu(imu_data_t *imu) {
    // Get current attitude estimate
    imu->roll = s_attitude.roll;
    imu->pitch = s_attitude.pitch;
    imu->yaw = s_attitude.yaw;

    // Gyro rates (bias-corrected)
    imu->gyro_x = s_rates.roll_rate;
    imu->gyro_y = s_rates.pitch_rate;
    imu->gyro_z = s_rates.yaw_rate;

    // Position - not available without external tracking
    imu->x = 0.0f;
    imu->y = 0.0f;

    // Altitude from barometer
    imu->altitude = s_altitude;
}

void platform_write_motors(const motor_cmd_t *cmd) {
    if (!s_armed) {
        return;
    }

    motors_cmd_t motor_cmd;
    for (int i = 0; i < 4; i++) {
        motor_cmd.motor[i] = cmd->motor[i];
    }
    motors_set(&motor_cmd);
}

// ----------------------------------------------------------------------------
// Extended Platform Interface
// ----------------------------------------------------------------------------

void platform_arm(void) {
    if (s_initialized && s_calibrated) {
        motors_arm();
        s_armed = true;
    }
}

void platform_disarm(void) {
    motors_disarm();
    s_armed = false;
}

uint32_t platform_get_time_ms(void) {
    return system_get_tick();
}

uint32_t platform_get_time_us(void) {
    return system_get_us();
}

void platform_delay_ms(uint32_t ms) {
    system_delay_ms(ms);
}

void platform_delay_us(uint32_t us) {
    system_delay_us(us);
}

void platform_debug_init(void) {
    usart1_init(NULL);
}

void platform_debug_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    // Use usart1_printf which handles va_list internally
    // For now, just use the simple version
    usart1_printf("%s", fmt);  // Simplified - full implementation would need vprintf
    va_end(args);
}

void platform_update(void) {
    // This should be called at 400Hz to update sensor fusion

    // Read IMU
    lsm6dsl_read_all(&s_accel, &s_gyro);

    // Apply gyro bias correction
    float gyro[3] = {
        s_gyro.x - s_gyro_bias[0],
        s_gyro.y - s_gyro_bias[1],
        s_gyro.z - s_gyro_bias[2]
    };

    float accel[3] = {s_accel.x, s_accel.y, s_accel.z};

    // Update attitude filter (dt = 2.5ms for 400Hz)
    attitude_update(accel, gyro, 0.0025f);

    // Get current attitude and rates
    attitude_get(&s_attitude);
    attitude_get_rates(&s_rates);

    // Store corrected rates
    s_rates.roll_rate = gyro[0];
    s_rates.pitch_rate = gyro[1];
    s_rates.yaw_rate = gyro[2];

    // Read barometer at lower rate (handled by caller or counter)
    static int baro_counter = 0;
    if (++baro_counter >= 8) {  // 400/8 = 50Hz
        baro_counter = 0;
        s_pressure = lps22hd_read_pressure();
        s_altitude = lps22hd_altitude(s_pressure);
    }

    // Read magnetometer at lower rate
    static int mag_counter = 0;
    if (++mag_counter >= 8) {  // 50Hz
        mag_counter = 0;
        lis2mdl_read(&s_mag);
        float mag[3] = {s_mag.x, s_mag.y, s_mag.z};
        attitude_update_mag(mag);
    }
}

void platform_emergency_stop(void) {
    motors_emergency_stop();
    s_armed = false;
}
