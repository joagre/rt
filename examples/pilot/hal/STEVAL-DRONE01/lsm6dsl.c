// LSM6DSL IMU driver for STEVAL-DRONE01
//
// SPI interface to the LSM6DSL 6-axis accelerometer + gyroscope.

#include "lsm6dsl.h"

// TODO: Include STM32 HAL headers when integrating
// #include "stm32f4xx_hal.h"

// ----------------------------------------------------------------------------
// Register addresses
// ----------------------------------------------------------------------------

#define LSM6DSL_WHO_AM_I        0x0F
#define LSM6DSL_CTRL1_XL        0x10  // Accel control
#define LSM6DSL_CTRL2_G         0x11  // Gyro control
#define LSM6DSL_CTRL3_C         0x12  // Control register 3
#define LSM6DSL_CTRL4_C         0x13  // Control register 4
#define LSM6DSL_CTRL5_C         0x14  // Control register 5
#define LSM6DSL_CTRL6_C         0x15  // Control register 6
#define LSM6DSL_CTRL7_G         0x16  // Gyro control 7
#define LSM6DSL_CTRL8_XL        0x17  // Accel control 8
#define LSM6DSL_STATUS_REG      0x1E  // Status register
#define LSM6DSL_OUT_TEMP_L      0x20  // Temperature low byte
#define LSM6DSL_OUT_TEMP_H      0x21  // Temperature high byte
#define LSM6DSL_OUTX_L_G        0x22  // Gyro X low byte
#define LSM6DSL_OUTX_H_G        0x23  // Gyro X high byte
#define LSM6DSL_OUTY_L_G        0x24  // Gyro Y low byte
#define LSM6DSL_OUTY_H_G        0x25  // Gyro Y high byte
#define LSM6DSL_OUTZ_L_G        0x26  // Gyro Z low byte
#define LSM6DSL_OUTZ_H_G        0x27  // Gyro Z high byte
#define LSM6DSL_OUTX_L_XL       0x28  // Accel X low byte
#define LSM6DSL_OUTX_H_XL       0x29  // Accel X high byte
#define LSM6DSL_OUTY_L_XL       0x2A  // Accel Y low byte
#define LSM6DSL_OUTY_H_XL       0x2B  // Accel Y high byte
#define LSM6DSL_OUTZ_L_XL       0x2C  // Accel Z low byte
#define LSM6DSL_OUTZ_H_XL       0x2D  // Accel Z high byte

#define LSM6DSL_WHO_AM_I_VALUE  0x6A  // Expected WHO_AM_I response

// SPI read/write flags
#define LSM6DSL_SPI_READ        0x80
#define LSM6DSL_SPI_WRITE       0x00

// ----------------------------------------------------------------------------
// Conversion constants
// ----------------------------------------------------------------------------

// Accelerometer sensitivity (mg/LSB) for each full-scale setting
static const float accel_sensitivity[] = {
    0.061f,  // ±2g
    0.122f,  // ±4g
    0.244f,  // ±8g
    0.488f   // ±16g
};

// Gyroscope sensitivity (mdps/LSB) for each full-scale setting
static const float gyro_sensitivity[] = {
    8.75f,   // ±250 dps
    17.50f,  // ±500 dps
    35.00f,  // ±1000 dps
    70.00f   // ±2000 dps
};

#define MG_TO_MS2   0.00981f   // mg to m/s²
#define MDPS_TO_RAD 0.0000175f // mdps to rad/s

// ----------------------------------------------------------------------------
// Static state
// ----------------------------------------------------------------------------

static lsm6dsl_config_t s_config;
static float s_accel_scale;  // Conversion factor: raw -> m/s²
static float s_gyro_scale;   // Conversion factor: raw -> rad/s

// ----------------------------------------------------------------------------
// SPI low-level (TODO: implement with STM32 HAL)
// ----------------------------------------------------------------------------

static void spi_cs_low(void) {
    // TODO: HAL_GPIO_WritePin(LSM6DSL_CS_GPIO_Port, LSM6DSL_CS_Pin, GPIO_PIN_RESET);
}

static void spi_cs_high(void) {
    // TODO: HAL_GPIO_WritePin(LSM6DSL_CS_GPIO_Port, LSM6DSL_CS_Pin, GPIO_PIN_SET);
}

static uint8_t spi_transfer(uint8_t data) {
    // TODO: Implement SPI transfer using HAL_SPI_TransmitReceive
    // uint8_t rx;
    // HAL_SPI_TransmitReceive(&hspi1, &data, &rx, 1, HAL_MAX_DELAY);
    // return rx;
    (void)data;
    return 0;
}

static void spi_write_reg(uint8_t reg, uint8_t value) {
    spi_cs_low();
    spi_transfer(reg | LSM6DSL_SPI_WRITE);
    spi_transfer(value);
    spi_cs_high();
}

static uint8_t spi_read_reg(uint8_t reg) {
    spi_cs_low();
    spi_transfer(reg | LSM6DSL_SPI_READ);
    uint8_t value = spi_transfer(0x00);
    spi_cs_high();
    return value;
}

static void spi_read_burst(uint8_t reg, uint8_t *buf, uint8_t len) {
    spi_cs_low();
    spi_transfer(reg | LSM6DSL_SPI_READ);
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = spi_transfer(0x00);
    }
    spi_cs_high();
}

// ----------------------------------------------------------------------------
// Public API
// ----------------------------------------------------------------------------

bool lsm6dsl_init(const lsm6dsl_config_t *config) {
    // Use provided config or defaults
    if (config) {
        s_config = *config;
    } else {
        s_config = (lsm6dsl_config_t)LSM6DSL_CONFIG_DEFAULT;
    }

    // Calculate scale factors
    s_accel_scale = accel_sensitivity[s_config.accel_fs] * MG_TO_MS2;
    s_gyro_scale = gyro_sensitivity[s_config.gyro_fs] * MDPS_TO_RAD;

    // Check WHO_AM_I
    if (!lsm6dsl_is_ready()) {
        return false;
    }

    // Software reset
    spi_write_reg(LSM6DSL_CTRL3_C, 0x01);
    // TODO: HAL_Delay(10);

    // Configure accelerometer: ODR and full-scale
    uint8_t ctrl1 = (s_config.odr << 4) | (s_config.accel_fs << 2);
    spi_write_reg(LSM6DSL_CTRL1_XL, ctrl1);

    // Configure gyroscope: ODR and full-scale
    uint8_t ctrl2 = (s_config.odr << 4) | (s_config.gyro_fs << 2);
    spi_write_reg(LSM6DSL_CTRL2_G, ctrl2);

    // Enable block data update (BDU) to prevent reading during update
    spi_write_reg(LSM6DSL_CTRL3_C, 0x44);  // BDU=1, IF_INC=1

    return true;
}

bool lsm6dsl_is_ready(void) {
    uint8_t who = spi_read_reg(LSM6DSL_WHO_AM_I);
    return (who == LSM6DSL_WHO_AM_I_VALUE);
}

void lsm6dsl_read_accel_raw(lsm6dsl_raw_t *data) {
    uint8_t buf[6];
    spi_read_burst(LSM6DSL_OUTX_L_XL, buf, 6);
    data->x = (int16_t)(buf[1] << 8 | buf[0]);
    data->y = (int16_t)(buf[3] << 8 | buf[2]);
    data->z = (int16_t)(buf[5] << 8 | buf[4]);
}

void lsm6dsl_read_gyro_raw(lsm6dsl_raw_t *data) {
    uint8_t buf[6];
    spi_read_burst(LSM6DSL_OUTX_L_G, buf, 6);
    data->x = (int16_t)(buf[1] << 8 | buf[0]);
    data->y = (int16_t)(buf[3] << 8 | buf[2]);
    data->z = (int16_t)(buf[5] << 8 | buf[4]);
}

void lsm6dsl_read_accel(lsm6dsl_data_t *data) {
    lsm6dsl_raw_t raw;
    lsm6dsl_read_accel_raw(&raw);
    data->x = raw.x * s_accel_scale;
    data->y = raw.y * s_accel_scale;
    data->z = raw.z * s_accel_scale;
}

void lsm6dsl_read_gyro(lsm6dsl_data_t *data) {
    lsm6dsl_raw_t raw;
    lsm6dsl_read_gyro_raw(&raw);
    data->x = raw.x * s_gyro_scale;
    data->y = raw.y * s_gyro_scale;
    data->z = raw.z * s_gyro_scale;
}

void lsm6dsl_read_all(lsm6dsl_data_t *accel, lsm6dsl_data_t *gyro) {
    // Read all 12 bytes in one burst (gyro + accel are contiguous)
    uint8_t buf[12];
    spi_read_burst(LSM6DSL_OUTX_L_G, buf, 12);

    // Gyro (first 6 bytes)
    int16_t gx = (int16_t)(buf[1] << 8 | buf[0]);
    int16_t gy = (int16_t)(buf[3] << 8 | buf[2]);
    int16_t gz = (int16_t)(buf[5] << 8 | buf[4]);
    gyro->x = gx * s_gyro_scale;
    gyro->y = gy * s_gyro_scale;
    gyro->z = gz * s_gyro_scale;

    // Accel (next 6 bytes)
    int16_t ax = (int16_t)(buf[7] << 8 | buf[6]);
    int16_t ay = (int16_t)(buf[9] << 8 | buf[8]);
    int16_t az = (int16_t)(buf[11] << 8 | buf[10]);
    accel->x = ax * s_accel_scale;
    accel->y = ay * s_accel_scale;
    accel->z = az * s_accel_scale;
}

float lsm6dsl_read_temp(void) {
    uint8_t buf[2];
    spi_read_burst(LSM6DSL_OUT_TEMP_L, buf, 2);
    int16_t raw = (int16_t)(buf[1] << 8 | buf[0]);
    // Temperature sensitivity: 256 LSB/°C, offset at 25°C
    return 25.0f + (raw / 256.0f);
}
