// LSM6DSL IMU driver for STEVAL-DRONE01
//
// 6-axis accelerometer + gyroscope via SPI1.
// Provides raw sensor data for the estimator actor.

#ifndef LSM6DSL_H
#define LSM6DSL_H

#include <stdint.h>
#include <stdbool.h>

// Accelerometer full-scale selection
typedef enum {
    LSM6DSL_ACCEL_FS_2G  = 0,
    LSM6DSL_ACCEL_FS_4G  = 1,
    LSM6DSL_ACCEL_FS_8G  = 2,
    LSM6DSL_ACCEL_FS_16G = 3
} lsm6dsl_accel_fs_t;

// Gyroscope full-scale selection
typedef enum {
    LSM6DSL_GYRO_FS_250DPS  = 0,
    LSM6DSL_GYRO_FS_500DPS  = 1,
    LSM6DSL_GYRO_FS_1000DPS = 2,
    LSM6DSL_GYRO_FS_2000DPS = 3
} lsm6dsl_gyro_fs_t;

// Output data rate (both accel and gyro)
typedef enum {
    LSM6DSL_ODR_OFF    = 0,
    LSM6DSL_ODR_12_5HZ = 1,
    LSM6DSL_ODR_26HZ   = 2,
    LSM6DSL_ODR_52HZ   = 3,
    LSM6DSL_ODR_104HZ  = 4,
    LSM6DSL_ODR_208HZ  = 5,
    LSM6DSL_ODR_416HZ  = 6,
    LSM6DSL_ODR_833HZ  = 7,
    LSM6DSL_ODR_1666HZ = 8
} lsm6dsl_odr_t;

// Raw sensor data (signed 16-bit)
typedef struct {
    int16_t x, y, z;
} lsm6dsl_raw_t;

// Scaled sensor data (floats)
typedef struct {
    float x, y, z;
} lsm6dsl_data_t;

// Configuration
typedef struct {
    lsm6dsl_accel_fs_t accel_fs;
    lsm6dsl_gyro_fs_t gyro_fs;
    lsm6dsl_odr_t odr;
} lsm6dsl_config_t;

// Default configuration: ±4g, ±500dps, 416Hz
#define LSM6DSL_CONFIG_DEFAULT { \
    .accel_fs = LSM6DSL_ACCEL_FS_4G, \
    .gyro_fs = LSM6DSL_GYRO_FS_500DPS, \
    .odr = LSM6DSL_ODR_416HZ \
}

// ----------------------------------------------------------------------------
// API
// ----------------------------------------------------------------------------

// Initialize the LSM6DSL sensor.
// Returns true on success, false if sensor not found or init failed.
bool lsm6dsl_init(const lsm6dsl_config_t *config);

// Check if sensor is ready (WHO_AM_I register check).
bool lsm6dsl_is_ready(void);

// Read raw accelerometer data (signed 16-bit).
void lsm6dsl_read_accel_raw(lsm6dsl_raw_t *data);

// Read raw gyroscope data (signed 16-bit).
void lsm6dsl_read_gyro_raw(lsm6dsl_raw_t *data);

// Read scaled accelerometer data (m/s²).
void lsm6dsl_read_accel(lsm6dsl_data_t *data);

// Read scaled gyroscope data (rad/s).
void lsm6dsl_read_gyro(lsm6dsl_data_t *data);

// Read both accel and gyro in one SPI burst (more efficient).
void lsm6dsl_read_all(lsm6dsl_data_t *accel, lsm6dsl_data_t *gyro);

// Read temperature (°C).
float lsm6dsl_read_temp(void);

#endif // LSM6DSL_H
