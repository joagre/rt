// STM32F4 Platform Layer for Pilot Example
//
// Provides the same platform interface as the Webots version,
// but using real STEVAL-DRONE01 hardware.
//
// Usage: In pilot.c, replace Webots platform functions with these.
// Or compile this file instead of the Webots-specific code.

#ifndef PLATFORM_STM32F4_H
#define PLATFORM_STM32F4_H

#include "../../types.h"
#include <stdint.h>

// ----------------------------------------------------------------------------
// Platform Interface (matches Webots version in pilot.c)
// ----------------------------------------------------------------------------

// Initialize all hardware: clocks, GPIO, sensors, motors.
// Returns 0 on success, -1 on error.
int platform_init(void);

// Read raw sensor data from sensors.
// Populates accel/gyro from LSM6DSL (gyro is bias-corrected).
// Populates mag from LIS2MDL.
// Populates pressure from LPS22HD barometer.
// Note: GPS not available on this platform.
void platform_read_sensors(sensor_data_t *sensors);

// Write motor commands to TIM4 PWM.
// Values in cmd->motor[0..3] are normalized 0.0 to 1.0.
void platform_write_motors(const motor_cmd_t *cmd);

// ----------------------------------------------------------------------------
// Extended Platform Interface (STM32-specific)
// ----------------------------------------------------------------------------

// Calibrate sensors (gyro bias, barometer reference).
// Call after platform_init(), keep drone still and level.
// Returns 0 on success, -1 on error.
int platform_calibrate(void);

// Arm/disarm motors.
void platform_arm(void);
void platform_disarm(void);

// Get timing information.
uint32_t platform_get_time_ms(void);
uint32_t platform_get_time_us(void);

// Delay functions.
void platform_delay_ms(uint32_t ms);
void platform_delay_us(uint32_t us);

// Debug output (requires USART1 initialized).
void platform_debug_init(void);
void platform_debug_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

// Emergency stop - immediately stop all motors.
void platform_emergency_stop(void);

#endif // PLATFORM_STM32F4_H
