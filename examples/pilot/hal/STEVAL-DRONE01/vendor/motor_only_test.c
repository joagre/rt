/**
 * Motor-only test - no gyro, no SPI, no HAL_Delay
 * Just spins each motor in sequence so you can observe which is which
 *
 * LED blinks N times to indicate motor N, then spins that motor
 *
 * Expected motor configuration (X-quad):
 *
 *          Front
 *      M2(CW)  M3(CCW)
 *       P2  \  /  P4
 *            \/
 *            /\
 *       P1  /  \  P5
 *      M1(CCW) M4(CW)
 *          Rear
 *
 * Channel mapping:
 *   1 blink → CH1/P1 → M1 (rear-left, CCW)
 *   2 blinks → CH2/P2 → M2 (front-left, CW)
 *   3 blinks → CH3/P4 → M3 (front-right, CCW)
 *   4 blinks → CH4/P5 → M4 (rear-right, CW)
 *
 * Note: Board connectors are labeled P1, P2, P4, P5 (no P3).
 * To reverse motor direction: flip the 2-wire connector.
 */

#include <stdint.h>

// Register definitions
#define RCC_AHB1ENR     (*(volatile uint32_t *)0x40023830)
#define RCC_APB1ENR     (*(volatile uint32_t *)0x40023840)

#define GPIOB_MODER     (*(volatile uint32_t *)0x40020400)
#define GPIOB_OSPEEDR   (*(volatile uint32_t *)0x40020408)
#define GPIOB_ODR       (*(volatile uint32_t *)0x40020414)
#define GPIOB_AFRL      (*(volatile uint32_t *)0x40020420)
#define GPIOB_AFRH      (*(volatile uint32_t *)0x40020424)

#define TIM4_CR1        (*(volatile uint32_t *)0x40000800)
#define TIM4_CCMR1      (*(volatile uint32_t *)0x40000818)
#define TIM4_CCMR2      (*(volatile uint32_t *)0x4000081C)
#define TIM4_CCER       (*(volatile uint32_t *)0x40000820)
#define TIM4_PSC        (*(volatile uint32_t *)0x40000828)
#define TIM4_ARR        (*(volatile uint32_t *)0x4000082C)
#define TIM4_CCR1       (*(volatile uint32_t *)0x40000834)
#define TIM4_CCR2       (*(volatile uint32_t *)0x40000838)
#define TIM4_CCR3       (*(volatile uint32_t *)0x4000083C)
#define TIM4_CCR4       (*(volatile uint32_t *)0x40000840)

// PWM config: 16MHz / 16 / 1000 = 1kHz
#define PWM_PRESCALER   15
#define PWM_PERIOD      999
#define TEST_SPEED      80   // 8% - slow enough to see direction

static void delay(volatile uint32_t count) {
    while (count--) __asm__("nop");
}

static void delay_ms(uint32_t ms) {
    // Rough approximation at 16MHz
    delay(ms * 2000);
}

static void led_on(void) {
    GPIOB_ODR |= (1 << 5);
}

static void led_off(void) {
    GPIOB_ODR &= ~(1 << 5);
}

static void led_blink(int n) {
    for (int i = 0; i < n; i++) {
        led_on();
        delay_ms(150);
        led_off();
        delay_ms(150);
    }
    delay_ms(300);
}

static void motors_init(void) {
    // Enable clocks: GPIOB and TIM4
    RCC_AHB1ENR |= (1 << 1);   // GPIOBEN
    RCC_APB1ENR |= (1 << 2);   // TIM4EN
    delay(1000);

    // Configure PB6-PB9 as AF2 (TIM4 CH1-CH4)
    // MODER: 10 = alternate function
    GPIOB_MODER &= ~((3 << 12) | (3 << 14) | (3 << 16) | (3 << 18));
    GPIOB_MODER |=  ((2 << 12) | (2 << 14) | (2 << 16) | (2 << 18));

    // High speed
    GPIOB_OSPEEDR |= ((3 << 12) | (3 << 14) | (3 << 16) | (3 << 18));

    // AF2 for PB6, PB7 (AFRL)
    GPIOB_AFRL &= ~((0xF << 24) | (0xF << 28));
    GPIOB_AFRL |=  ((2 << 24) | (2 << 28));

    // AF2 for PB8, PB9 (AFRH)
    GPIOB_AFRH &= ~((0xF << 0) | (0xF << 4));
    GPIOB_AFRH |=  ((2 << 0) | (2 << 4));

    // Configure TIM4 for PWM
    TIM4_PSC = PWM_PRESCALER;
    TIM4_ARR = PWM_PERIOD;

    // PWM mode 1 on all channels, preload enable
    TIM4_CCMR1 = (6 << 4) | (1 << 3) | (6 << 12) | (1 << 11);
    TIM4_CCMR2 = (6 << 4) | (1 << 3) | (6 << 12) | (1 << 11);

    // Enable all 4 channels
    TIM4_CCER = (1 << 0) | (1 << 4) | (1 << 8) | (1 << 12);

    // All motors off
    TIM4_CCR1 = 0;
    TIM4_CCR2 = 0;
    TIM4_CCR3 = 0;
    TIM4_CCR4 = 0;

    // Enable timer
    TIM4_CR1 = 1;
}

static void motor_set(int channel, uint16_t speed) {
    switch (channel) {
        case 1: TIM4_CCR1 = speed; break;
        case 2: TIM4_CCR2 = speed; break;
        case 3: TIM4_CCR3 = speed; break;
        case 4: TIM4_CCR4 = speed; break;
    }
}

static void motors_stop(void) {
    TIM4_CCR1 = 0;
    TIM4_CCR2 = 0;
    TIM4_CCR3 = 0;
    TIM4_CCR4 = 0;
}

int main(void) {
    // Enable GPIOB for LED
    RCC_AHB1ENR |= (1 << 1);
    delay(100);

    // PB5 as output (LED)
    GPIOB_MODER &= ~(3 << 10);
    GPIOB_MODER |= (1 << 10);

    // Startup indication - fast blinks
    for (int i = 0; i < 5; i++) {
        led_on(); delay_ms(50);
        led_off(); delay_ms(50);
    }
    delay_ms(1000);

    // Init motors
    motors_init();

    // Test each motor (once)
        // Motor 1 (CH1 = PB6)
        led_blink(1);
        motor_set(1, TEST_SPEED);
        delay_ms(2000);
        motors_stop();
        delay_ms(5000);

        // Motor 2 (CH2 = PB7)
        led_blink(2);
        motor_set(2, TEST_SPEED);
        delay_ms(2000);
        motors_stop();
        delay_ms(5000);

        // Motor 3 (CH3 = PB8)
        led_blink(3);
        motor_set(3, TEST_SPEED);
        delay_ms(2000);
        motors_stop();
        delay_ms(5000);

        // Motor 4 (CH4 = PB9)
        led_blink(4);
        motor_set(4, TEST_SPEED);
        delay_ms(2000);
        motors_stop();
        delay_ms(5000);

        // All motors together
        led_blink(5);
        motor_set(1, TEST_SPEED);
        motor_set(2, TEST_SPEED);
        motor_set(3, TEST_SPEED);
        motor_set(4, TEST_SPEED);
        delay_ms(3000);
        motors_stop();
        delay_ms(1000);

    // Done - slow blink
    while (1) {
        led_on(); delay_ms(1000);
        led_off(); delay_ms(1000);
    }
}
