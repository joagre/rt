/**
 * Minimal LED blink test - bare metal, no HAL_Delay
 * LED1 = PB5
 */

#include "stm32f4xx_hal.h"

#define RCC_AHB1ENR  (*(volatile uint32_t *)0x40023830)
#define GPIOB_MODER  (*(volatile uint32_t *)0x40020400)
#define GPIOB_ODR    (*(volatile uint32_t *)0x40020414)

static void delay(volatile uint32_t count) {
    while (count--) __asm__("nop");
}

int main(void) {
    // Enable GPIOB clock
    RCC_AHB1ENR |= (1 << 1);

    // Small delay for clock to stabilize
    delay(100);

    // Set PB5 as output (MODER bits 10:11 = 01)
    GPIOB_MODER &= ~(3 << 10);
    GPIOB_MODER |= (1 << 10);

    while (1) {
        GPIOB_ODR |= (1 << 5);   // LED on
        delay(500000);
        GPIOB_ODR &= ~(1 << 5);  // LED off
        delay(500000);
    }
}

void HAL_MspInit(void) {}
