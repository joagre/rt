// LIS2MDL Magnetometer driver for STEVAL-DRONE01
//
// 3-axis magnetometer via I2C1.
// Provides heading data for yaw estimation (with tilt compensation).

#ifndef LIS2MDL_H
#define LIS2MDL_H

#include <stdint.h>
#include <stdbool.h>

// Output data rate selection
typedef enum {
    LIS2MDL_ODR_10HZ  = 0,
    LIS2MDL_ODR_20HZ  = 1,
    LIS2MDL_ODR_50HZ  = 2,
    LIS2MDL_ODR_100HZ = 3
} lis2mdl_odr_t;

// Operating mode
typedef enum {
    LIS2MDL_MODE_CONTINUOUS = 0,  // Continuous measurement
    LIS2MDL_MODE_SINGLE     = 1,  // Single measurement
    LIS2MDL_MODE_IDLE       = 2   // Idle (power down)
} lis2mdl_mode_t;

// Raw sensor data (signed 16-bit)
typedef struct {
    int16_t x, y, z;
} lis2mdl_raw_t;

// Scaled sensor data (microtesla)
typedef struct {
    float x, y, z;
} lis2mdl_data_t;

// Hard-iron calibration offsets
typedef struct {
    float x, y, z;  // Offset in microtesla
} lis2mdl_offset_t;

// Configuration
typedef struct {
    lis2mdl_odr_t odr;
    lis2mdl_mode_t mode;
    bool temp_comp;           // Temperature compensation enable
    bool low_pass_filter;     // Low-pass filter enable
} lis2mdl_config_t;

// Default configuration: 50Hz, continuous, temp comp enabled
#define LIS2MDL_CONFIG_DEFAULT { \
    .odr = LIS2MDL_ODR_50HZ, \
    .mode = LIS2MDL_MODE_CONTINUOUS, \
    .temp_comp = true, \
    .low_pass_filter = true \
}

// ----------------------------------------------------------------------------
// API
// ----------------------------------------------------------------------------

// Initialize the LIS2MDL sensor.
// Returns true on success, false if sensor not found or init failed.
bool lis2mdl_init(const lis2mdl_config_t *config);

// Check if sensor is ready (WHO_AM_I register check).
bool lis2mdl_is_ready(void);

// Check if new data is available.
bool lis2mdl_data_ready(void);

// Read raw magnetometer data (signed 16-bit).
void lis2mdl_read_raw(lis2mdl_raw_t *data);

// Read scaled magnetometer data (microtesla).
void lis2mdl_read(lis2mdl_data_t *data);

// Read scaled data with hard-iron offset correction.
void lis2mdl_read_calibrated(lis2mdl_data_t *data, const lis2mdl_offset_t *offset);

// Read temperature (degrees Celsius).
float lis2mdl_read_temp(void);

// Set hard-iron offset registers (built-in offset cancellation).
// Note: These are 16-bit raw values, not scaled.
void lis2mdl_set_offset(int16_t x, int16_t y, int16_t z);

// Calculate heading from magnetometer data (no tilt compensation).
// Returns heading in radians, 0 = magnetic north, positive = clockwise.
// WARNING: Only accurate when sensor is level. For tilted orientations,
// use tilt-compensated heading with accelerometer data.
float lis2mdl_heading(const lis2mdl_data_t *mag);

// Calculate tilt-compensated heading.
// roll, pitch in radians (from accelerometer/IMU).
// Returns heading in radians, 0 = magnetic north.
float lis2mdl_heading_tilt_compensated(const lis2mdl_data_t *mag,
                                        float roll, float pitch);

#endif // LIS2MDL_H
