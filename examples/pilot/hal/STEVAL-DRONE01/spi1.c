// SPI1 driver for STM32F401 (STEVAL-DRONE01)
//
// Direct register access implementation for LSM6DSL communication.

#include "spi1.h"
#include "system_config.h"
#include "gpio_config.h"

// ----------------------------------------------------------------------------
// SPI1 Register Definitions
// ----------------------------------------------------------------------------

#define SPI1_BASE           0x40013000U

#define SPI1_CR1            (*(volatile uint32_t *)(SPI1_BASE + 0x00))
#define SPI1_CR2            (*(volatile uint32_t *)(SPI1_BASE + 0x04))
#define SPI1_SR             (*(volatile uint32_t *)(SPI1_BASE + 0x08))
#define SPI1_DR             (*(volatile uint32_t *)(SPI1_BASE + 0x0C))

// SPI_CR1 bits
#define SPI_CR1_CPHA        (1U << 0)   // Clock phase
#define SPI_CR1_CPOL        (1U << 1)   // Clock polarity
#define SPI_CR1_MSTR        (1U << 2)   // Master mode
#define SPI_CR1_BR_MASK     (7U << 3)   // Baud rate prescaler
#define SPI_CR1_BR_SHIFT    3
#define SPI_CR1_SPE         (1U << 6)   // SPI enable
#define SPI_CR1_LSBFIRST    (1U << 7)   // LSB first (0 = MSB first)
#define SPI_CR1_SSI         (1U << 8)   // Internal slave select
#define SPI_CR1_SSM         (1U << 9)   // Software slave management
#define SPI_CR1_RXONLY      (1U << 10)  // Receive only mode
#define SPI_CR1_DFF         (1U << 11)  // Data frame format (0 = 8-bit)
#define SPI_CR1_CRCNEXT     (1U << 12)  // CRC next
#define SPI_CR1_CRCEN       (1U << 13)  // CRC enable
#define SPI_CR1_BIDIOE      (1U << 14)  // Bidirectional output enable
#define SPI_CR1_BIDIMODE    (1U << 15)  // Bidirectional mode

// SPI_CR2 bits
#define SPI_CR2_RXDMAEN     (1U << 0)   // RX DMA enable
#define SPI_CR2_TXDMAEN     (1U << 1)   // TX DMA enable
#define SPI_CR2_SSOE        (1U << 2)   // SS output enable
#define SPI_CR2_FRF         (1U << 4)   // Frame format (0 = Motorola)
#define SPI_CR2_ERRIE       (1U << 5)   // Error interrupt enable
#define SPI_CR2_RXNEIE      (1U << 6)   // RX not empty interrupt enable
#define SPI_CR2_TXEIE       (1U << 7)   // TX empty interrupt enable

// SPI_SR bits
#define SPI_SR_RXNE         (1U << 0)   // RX buffer not empty
#define SPI_SR_TXE          (1U << 1)   // TX buffer empty
#define SPI_SR_CHSIDE       (1U << 2)   // Channel side
#define SPI_SR_UDR          (1U << 3)   // Underrun flag
#define SPI_SR_CRCERR       (1U << 4)   // CRC error flag
#define SPI_SR_MODF         (1U << 5)   // Mode fault
#define SPI_SR_OVR          (1U << 6)   // Overrun flag
#define SPI_SR_BSY          (1U << 7)   // Busy flag
#define SPI_SR_FRE          (1U << 8)   // Frame error

// ----------------------------------------------------------------------------
// Implementation
// ----------------------------------------------------------------------------

void spi1_init(spi1_speed_t speed) {
    // Enable SPI1 clock
    system_enable_spi1();

    // Initialize GPIO pins
    gpio_init_spi1();

    // Disable SPI before configuration
    SPI1_CR1 &= ~SPI_CR1_SPE;

    // Configure SPI1:
    // - Master mode
    // - Full duplex (BIDIMODE=0, RXONLY=0)
    // - 8-bit data frame (DFF=0)
    // - MSB first (LSBFIRST=0)
    // - Software slave management (SSM=1, SSI=1)
    // - Clock polarity high (CPOL=1) - Mode 3
    // - Clock phase 2nd edge (CPHA=1) - Mode 3
    uint32_t cr1 = 0;
    cr1 |= SPI_CR1_MSTR;        // Master mode
    cr1 |= SPI_CR1_SSM;         // Software slave management
    cr1 |= SPI_CR1_SSI;         // Internal slave select high
    cr1 |= SPI_CR1_CPOL;        // Clock polarity high (idle high)
    cr1 |= SPI_CR1_CPHA;        // Clock phase: sample on 2nd edge
    cr1 |= ((uint32_t)speed << SPI_CR1_BR_SHIFT);   // Baud rate

    SPI1_CR1 = cr1;

    // CR2: No DMA, no interrupts, Motorola frame format
    SPI1_CR2 = 0;

    // Enable SPI
    SPI1_CR1 |= SPI_CR1_SPE;
}

void spi1_deinit(void) {
    // Wait for any pending transfer
    spi1_wait();

    // Disable SPI
    SPI1_CR1 &= ~SPI_CR1_SPE;

    // Note: GPIO pins and clock are not disabled here
    // to allow reinitialization
}

void spi1_set_speed(spi1_speed_t speed) {
    // Disable SPI
    SPI1_CR1 &= ~SPI_CR1_SPE;

    // Update baud rate
    SPI1_CR1 = (SPI1_CR1 & ~SPI_CR1_BR_MASK) |
               ((uint32_t)speed << SPI_CR1_BR_SHIFT);

    // Re-enable SPI
    SPI1_CR1 |= SPI_CR1_SPE;
}

uint8_t spi1_transfer(uint8_t tx_data) {
    // Wait until TX buffer is empty
    while (!(SPI1_SR & SPI_SR_TXE));

    // Send data
    SPI1_DR = tx_data;

    // Wait until RX buffer is not empty
    while (!(SPI1_SR & SPI_SR_RXNE));

    // Read received data
    return (uint8_t)SPI1_DR;
}

void spi1_transfer_buf(const uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        uint8_t tx = tx_buf ? tx_buf[i] : 0x00;
        uint8_t rx = spi1_transfer(tx);
        if (rx_buf) {
            rx_buf[i] = rx;
        }
    }
}

void spi1_write(uint8_t data) {
    (void)spi1_transfer(data);
}

void spi1_write_buf(const uint8_t *buf, uint16_t len) {
    spi1_transfer_buf(buf, NULL, len);
}

uint8_t spi1_read(void) {
    return spi1_transfer(0x00);
}

void spi1_read_buf(uint8_t *buf, uint16_t len) {
    spi1_transfer_buf(NULL, buf, len);
}

bool spi1_is_busy(void) {
    return (SPI1_SR & SPI_SR_BSY) != 0;
}

void spi1_wait(void) {
    // Wait until not busy
    while (SPI1_SR & SPI_SR_BSY);
}
