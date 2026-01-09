// LPS22HD Barometer driver for STEVAL-DRONE01
//
// Pressure/altitude sensor via I2C1.
// Provides altitude data for altitude hold (relative altitude only).

#ifndef LPS22HD_H
#define LPS22HD_H

#include <stdint.h>
#include <stdbool.h>

// Output data rate selection
typedef enum {
    LPS22HD_ODR_ONE_SHOT = 0,  // Power down / one-shot mode
    LPS22HD_ODR_1HZ      = 1,
    LPS22HD_ODR_10HZ     = 2,
    LPS22HD_ODR_25HZ     = 3,
    LPS22HD_ODR_50HZ     = 4,
    LPS22HD_ODR_75HZ     = 5
} lps22hd_odr_t;

// Low-pass filter configuration
typedef enum {
    LPS22HD_LPF_DISABLED = 0,  // ODR/2 bandwidth
    LPS22HD_LPF_ODR_9    = 2,  // ODR/9 bandwidth
    LPS22HD_LPF_ODR_20   = 3   // ODR/20 bandwidth
} lps22hd_lpf_t;

// Raw sensor data
typedef struct {
    int32_t pressure;    // 24-bit signed (actually unsigned, but stored as int32)
    int16_t temperature; // 16-bit signed
} lps22hd_raw_t;

// Scaled sensor data
typedef struct {
    float pressure_hpa;  // Pressure in hectopascals (hPa)
    float temp_c;        // Temperature in Celsius
} lps22hd_data_t;

// Configuration
typedef struct {
    lps22hd_odr_t odr;
    lps22hd_lpf_t lpf;
    bool bdu;            // Block data update (recommended: true)
} lps22hd_config_t;

// Default configuration: 50Hz, LPF ODR/9, BDU enabled
#define LPS22HD_CONFIG_DEFAULT { \
    .odr = LPS22HD_ODR_50HZ, \
    .lpf = LPS22HD_LPF_ODR_9, \
    .bdu = true \
}

// ----------------------------------------------------------------------------
// API
// ----------------------------------------------------------------------------

// Initialize the LPS22HD sensor.
// Returns true on success, false if sensor not found or init failed.
bool lps22hd_init(const lps22hd_config_t *config);

// Check if sensor is ready (WHO_AM_I register check).
bool lps22hd_is_ready(void);

// Check if new pressure data is available.
bool lps22hd_pressure_ready(void);

// Check if new temperature data is available.
bool lps22hd_temp_ready(void);

// Read raw pressure (24-bit) and temperature (16-bit).
void lps22hd_read_raw(lps22hd_raw_t *data);

// Read scaled pressure (hPa) and temperature (°C).
void lps22hd_read(lps22hd_data_t *data);

// Read pressure only (hPa).
float lps22hd_read_pressure(void);

// Read temperature only (°C).
float lps22hd_read_temp(void);

// Set reference pressure for altitude calculations.
// Call this at ground level to establish baseline.
// pressure_hpa: Current pressure reading at ground level.
void lps22hd_set_reference(float pressure_hpa);

// Get reference pressure (hPa).
float lps22hd_get_reference(void);

// Calculate altitude relative to reference pressure.
// Uses barometric formula: altitude = 44330 * (1 - (P/P0)^0.1903)
// Returns altitude in meters above reference point.
float lps22hd_altitude(float pressure_hpa);

// Calculate altitude from current reading.
// Convenience function that reads pressure and calculates altitude.
float lps22hd_read_altitude(void);

// Trigger one-shot measurement (when ODR = ONE_SHOT).
void lps22hd_trigger_one_shot(void);

#endif // LPS22HD_H
