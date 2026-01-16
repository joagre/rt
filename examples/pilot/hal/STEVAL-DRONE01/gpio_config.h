// GPIO configuration for STEVAL-DRONE01
//
// Pin definitions and low-level GPIO helpers for motors, debug serial, LED,
// button. Note: Sensor GPIO (SPI2, chip selects) handled by ST BSP drivers in
// vendor/.

#ifndef GPIO_CONFIG_H
#define GPIO_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

// ----------------------------------------------------------------------------
// Pin Definitions
// ----------------------------------------------------------------------------

// TIM4 - Motor PWM (channels 1-4)
#define TIM4_CH1_PORT 'B'
#define TIM4_CH1_PIN 6
#define TIM4_CH2_PORT 'B'
#define TIM4_CH2_PIN 7
#define TIM4_CH3_PORT 'B'
#define TIM4_CH3_PIN 8
#define TIM4_CH4_PORT 'B'
#define TIM4_CH4_PIN 9
#define TIM4_AF 2 // Alternate function 2

// USART1 - Debug serial
#define USART1_TX_PORT 'A'
#define USART1_TX_PIN 9
#define USART1_RX_PORT 'A'
#define USART1_RX_PIN 10
#define USART1_AF 7 // Alternate function 7

// LED (PB5 = LD1 on STEVAL-FCU001V1)
#define LED_PORT 'B'
#define LED_PIN 5

// User button (optional, board-dependent)
#define BTN_PORT 'A'
#define BTN_PIN 0

// ----------------------------------------------------------------------------
// GPIO Mode and Configuration Types
// ----------------------------------------------------------------------------

typedef enum {
    GPIO_MODE_INPUT = 0,
    GPIO_MODE_OUTPUT = 1,
    GPIO_MODE_AF = 2, // Alternate function
    GPIO_MODE_ANALOG = 3
} gpio_mode_t;

typedef enum { GPIO_OTYPE_PUSHPULL = 0, GPIO_OTYPE_OPENDRAIN = 1 } gpio_otype_t;

typedef enum {
    GPIO_SPEED_LOW = 0,
    GPIO_SPEED_MEDIUM = 1,
    GPIO_SPEED_HIGH = 2,
    GPIO_SPEED_VERYHIGH = 3
} gpio_speed_t;

typedef enum {
    GPIO_PUPD_NONE = 0,
    GPIO_PUPD_PULLUP = 1,
    GPIO_PUPD_PULLDOWN = 2
} gpio_pupd_t;

// ----------------------------------------------------------------------------
// API
// ----------------------------------------------------------------------------

// Initialize all GPIO for peripherals (motors, debug serial, LED, button)
void gpio_init_all(void);

// Individual peripheral GPIO initialization
void gpio_init_tim4_pwm(void);
void gpio_init_usart1(void);
void gpio_init_led(void);
void gpio_init_button(void);

// LED control
void gpio_led_on(void);
void gpio_led_off(void);
void gpio_led_toggle(void);

// Button read
bool gpio_button_read(void);

// Low-level GPIO configuration
void gpio_set_mode(char port, uint8_t pin, gpio_mode_t mode);
void gpio_set_otype(char port, uint8_t pin, gpio_otype_t otype);
void gpio_set_speed(char port, uint8_t pin, gpio_speed_t speed);
void gpio_set_pupd(char port, uint8_t pin, gpio_pupd_t pupd);
void gpio_set_af(char port, uint8_t pin, uint8_t af);

// GPIO read/write
void gpio_write(char port, uint8_t pin, bool value);
bool gpio_read(char port, uint8_t pin);
void gpio_toggle(char port, uint8_t pin);

#endif // GPIO_CONFIG_H
