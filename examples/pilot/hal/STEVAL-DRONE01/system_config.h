// System configuration for STM32F401 (STEVAL-DRONE01)
//
// Peripheral clock enables and timing helpers.
// Note: Clock initialization is handled by ST HAL in the main pilot build.
//       This file provides system_enable_*() functions and timing if needed.

#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

// ----------------------------------------------------------------------------
// Clock Configuration
// ----------------------------------------------------------------------------
//
// The STEVAL-FCU001V1 runs at 16MHz HSI by default.
// ST HAL can configure PLL for 84MHz if needed.
// These values are used for baud rate calculations and timing.

// System clock frequency (HSI default, HAL may reconfigure)
#define SYSCLK_FREQ 16000000U // 16 MHz

// AHB clock (HCLK) - feeds Cortex-M4 core, memory, DMA
#define HCLK_FREQ 16000000U // 16 MHz

// APB1 clock (PCLK1) - low-speed peripherals (I2C, UART2, TIM2-5)
#define PCLK1_FREQ 16000000U // 16 MHz

// APB2 clock (PCLK2) - high-speed peripherals (SPI1, UART1, TIM1)
#define PCLK2_FREQ 16000000U // 16 MHz

// PLL configuration (for standalone use if not using ST HAL)
#define PLL_M 16
#define PLL_N 336
#define PLL_P 4
#define PLL_Q 7

// ----------------------------------------------------------------------------
// Timing
// ----------------------------------------------------------------------------

// SysTick configuration (1ms tick)
#define SYSTICK_FREQ 1000U // 1kHz (1ms period)

// Get current tick count (milliseconds since boot)
uint32_t system_get_tick(void);

// Delay for specified milliseconds
void system_delay_ms(uint32_t ms);

// Delay for specified microseconds (busy-wait)
void system_delay_us(uint32_t us);

// Get microsecond timestamp (from DWT cycle counter)
uint32_t system_get_us(void);

// ----------------------------------------------------------------------------
// System Initialization
// ----------------------------------------------------------------------------

// Initialize system clocks (HSE + PLL for 84MHz)
// Returns true on success, false if HSE fails to start
bool system_clock_init(void);

// Initialize SysTick for 1ms interrupt
void system_tick_init(void);

// Initialize DWT cycle counter for microsecond timing
void system_dwt_init(void);

// Full system initialization (clocks + tick + DWT)
bool system_init(void);

// ----------------------------------------------------------------------------
// Peripheral Clock Control
// ----------------------------------------------------------------------------

// Enable/disable peripheral clocks
void system_enable_gpio(char port); // 'A' to 'H'
void system_enable_spi1(void);
void system_enable_spi2(void);
void system_enable_i2c1(void);
void system_enable_i2c2(void);
void system_enable_tim2(void);
void system_enable_tim4(void);
void system_enable_usart1(void);
void system_enable_usart2(void);
void system_enable_dma1(void);
void system_enable_dma2(void);

// ----------------------------------------------------------------------------
// Error Handling
// ----------------------------------------------------------------------------

// System error handler (called on clock failure, etc.)
// Default implementation loops forever; override as needed.
void system_error_handler(void);

#endif // SYSTEM_CONFIG_H
