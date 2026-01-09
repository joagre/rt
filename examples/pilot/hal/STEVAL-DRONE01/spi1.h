// SPI1 driver for STM32F401 (STEVAL-DRONE01)
//
// Configured for LSM6DSL IMU communication.
// Mode: Master, full-duplex, 8-bit, MSB first
// Clock: CPOL=1, CPHA=1 (Mode 3) - required by LSM6DSL

#ifndef SPI1_H
#define SPI1_H

#include <stdint.h>
#include <stdbool.h>

// ----------------------------------------------------------------------------
// Configuration
// ----------------------------------------------------------------------------

// SPI clock speed options
// APB2 clock = 84MHz, SPI1 is on APB2
typedef enum {
    SPI1_SPEED_42MHZ    = 0,    // 84MHz / 2   = 42 MHz (max)
    SPI1_SPEED_21MHZ    = 1,    // 84MHz / 4   = 21 MHz
    SPI1_SPEED_10_5MHZ  = 2,    // 84MHz / 8   = 10.5 MHz
    SPI1_SPEED_5_25MHZ  = 3,    // 84MHz / 16  = 5.25 MHz
    SPI1_SPEED_2_625MHZ = 4,    // 84MHz / 32  = 2.625 MHz
    SPI1_SPEED_1_3MHZ   = 5,    // 84MHz / 64  = 1.3 MHz
    SPI1_SPEED_656KHZ   = 6,    // 84MHz / 128 = 656 kHz
    SPI1_SPEED_328KHZ   = 7     // 84MHz / 256 = 328 kHz
} spi1_speed_t;

// Default: 10.5 MHz (LSM6DSL max is 10 MHz)
#define SPI1_DEFAULT_SPEED  SPI1_SPEED_10_5MHZ

// ----------------------------------------------------------------------------
// API
// ----------------------------------------------------------------------------

// Initialize SPI1 peripheral
// Configures for LSM6DSL: Mode 3 (CPOL=1, CPHA=1), 8-bit, MSB first
void spi1_init(spi1_speed_t speed);

// Deinitialize SPI1
void spi1_deinit(void);

// Set SPI clock speed
void spi1_set_speed(spi1_speed_t speed);

// Transfer single byte (full-duplex)
// Sends tx_data, returns received byte
uint8_t spi1_transfer(uint8_t tx_data);

// Transfer multiple bytes (full-duplex)
// tx_buf and rx_buf can be the same buffer
// If tx_buf is NULL, sends 0x00 for each byte
// If rx_buf is NULL, discards received bytes
void spi1_transfer_buf(const uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len);

// Write single byte (ignore received data)
void spi1_write(uint8_t data);

// Write multiple bytes (ignore received data)
void spi1_write_buf(const uint8_t *buf, uint16_t len);

// Read single byte (sends 0x00)
uint8_t spi1_read(void);

// Read multiple bytes (sends 0x00 for each)
void spi1_read_buf(uint8_t *buf, uint16_t len);

// Check if SPI is busy
bool spi1_is_busy(void);

// Wait for SPI transfer to complete
void spi1_wait(void);

#endif // SPI1_H
