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

// Gyro bias (rad/s) - determined during calibration
static float s_gyro_bias[3] = {0.0f, 0.0f, 0.0f};

// Barometer reference pressure (hPa)
static float s_ref_pressure = 0.0f;

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

    s_ref_pressure = pressure_sum / BARO_CALIBRATION_SAMPLES;
    lps22hd_set_reference(s_ref_pressure);

    s_calibrated = true;
    return 0;
}

void platform_read_sensors(sensor_data_t *sensors) {
    // Read raw IMU data
    lsm6dsl_data_t accel, gyro;
    lsm6dsl_read_all(&accel, &gyro);

    // Accelerometer (m/s², body frame)
    sensors->accel[0] = accel.x;
    sensors->accel[1] = accel.y;
    sensors->accel[2] = accel.z;

    // Gyroscope (rad/s, body frame) - bias corrected
    sensors->gyro[0] = gyro.x - s_gyro_bias[0];
    sensors->gyro[1] = gyro.y - s_gyro_bias[1];
    sensors->gyro[2] = gyro.z - s_gyro_bias[2];

    // Magnetometer (read at every call, sensor rate handled internally)
    lis2mdl_data_t mag;
    lis2mdl_read(&mag);
    sensors->mag[0] = mag.x;
    sensors->mag[1] = mag.y;
    sensors->mag[2] = mag.z;
    sensors->mag_valid = true;

    // Barometer
    sensors->pressure_hpa = lps22hd_read_pressure();
    sensors->baro_temp_c = lps22hd_read_temp();
    sensors->baro_valid = true;

    // No GPS on this platform
    sensors->gps_x = 0.0f;
    sensors->gps_y = 0.0f;
    sensors->gps_z = 0.0f;
    sensors->gps_valid = false;
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

void platform_emergency_stop(void) {
    motors_emergency_stop();
    s_armed = false;
}
