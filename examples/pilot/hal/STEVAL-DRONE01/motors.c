// Motor PWM driver for STEVAL-DRONE01
//
// TIM4 PWM output for 4 brushed DC motors.

#include "motors.h"

// TODO: Include STM32 HAL headers when integrating
// #include "stm32f4xx_hal.h"

// ----------------------------------------------------------------------------
// Configuration
// ----------------------------------------------------------------------------

// TIM4 channel mapping
#define MOTOR_TIM           TIM4
#define MOTOR_TIM_CHANNEL_1 TIM_CHANNEL_1
#define MOTOR_TIM_CHANNEL_2 TIM_CHANNEL_2
#define MOTOR_TIM_CHANNEL_3 TIM_CHANNEL_3
#define MOTOR_TIM_CHANNEL_4 TIM_CHANNEL_4

// ----------------------------------------------------------------------------
// Static state
// ----------------------------------------------------------------------------

static motors_config_t s_config;
static bool s_armed = false;
static uint16_t s_pwm[MOTORS_COUNT] = {0, 0, 0, 0};

// TODO: Timer handle from STM32 HAL
// extern TIM_HandleTypeDef htim4;

// ----------------------------------------------------------------------------
// Helper functions
// ----------------------------------------------------------------------------

static inline float clampf(float x, float lo, float hi) {
    return (x < lo) ? lo : ((x > hi) ? hi : x);
}

static inline uint16_t float_to_pwm(float value) {
    value = clampf(value, 0.0f, 1.0f);
    uint16_t range = s_config.max_pulse - s_config.min_pulse;
    return s_config.min_pulse + (uint16_t)(value * range);
}

// ----------------------------------------------------------------------------
// PWM low-level (TODO: implement with STM32 HAL)
// ----------------------------------------------------------------------------

static void pwm_set_channel(uint8_t channel, uint16_t value) {
    // TODO: Implement with STM32 HAL
    // switch (channel) {
    // case 0: __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, value); break;
    // case 1: __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, value); break;
    // case 2: __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, value); break;
    // case 3: __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, value); break;
    // }
    (void)channel;
    (void)value;
}

static void pwm_start_all(void) {
    // TODO: Start PWM on all channels
    // HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
    // HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);
    // HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
    // HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
}

static void pwm_stop_all(void) {
    // TODO: Stop PWM on all channels
    // HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1);
    // HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_2);
    // HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_3);
    // HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_4);
}

static void pwm_init_timer(void) {
    // TODO: Initialize TIM4 for PWM
    // This is typically done in CubeMX-generated code (MX_TIM4_Init)
    // Timer should be configured for:
    // - PWM mode 1
    // - Desired frequency (s_config.frequency_hz)
    // - Auto-reload value = max_pulse
}

// ----------------------------------------------------------------------------
// Public API
// ----------------------------------------------------------------------------

bool motors_init(const motors_config_t *config) {
    // Use provided config or defaults
    if (config) {
        s_config = *config;
    } else {
        s_config = (motors_config_t)MOTORS_CONFIG_DEFAULT;
    }

    s_armed = false;

    // Initialize all PWM values to zero
    for (int i = 0; i < MOTORS_COUNT; i++) {
        s_pwm[i] = 0;
    }

    // Initialize timer peripheral
    pwm_init_timer();

    // Set all channels to zero
    for (int i = 0; i < MOTORS_COUNT; i++) {
        pwm_set_channel(i, 0);
    }

    return true;
}

void motors_arm(void) {
    if (!s_armed) {
        // Ensure motors are at zero before arming
        motors_stop();
        pwm_start_all();
        s_armed = true;
    }
}

void motors_disarm(void) {
    if (s_armed) {
        motors_stop();
        pwm_stop_all();
        s_armed = false;
    }
}

bool motors_is_armed(void) {
    return s_armed;
}

void motors_set(const motors_cmd_t *cmd) {
    if (!s_armed) {
        return;  // Ignore commands when disarmed
    }

    for (int i = 0; i < MOTORS_COUNT; i++) {
        s_pwm[i] = float_to_pwm(cmd->motor[i]);
        pwm_set_channel(i, s_pwm[i]);
    }
}

void motors_set_single(uint8_t motor, float value) {
    if (!s_armed || motor >= MOTORS_COUNT) {
        return;
    }

    s_pwm[motor] = float_to_pwm(value);
    pwm_set_channel(motor, s_pwm[motor]);
}

void motors_stop(void) {
    for (int i = 0; i < MOTORS_COUNT; i++) {
        s_pwm[i] = 0;
        pwm_set_channel(i, 0);
    }
}

void motors_emergency_stop(void) {
    // Immediately stop and disarm
    for (int i = 0; i < MOTORS_COUNT; i++) {
        s_pwm[i] = 0;
        pwm_set_channel(i, 0);
    }
    pwm_stop_all();
    s_armed = false;
}

void motors_get_pwm(uint16_t pwm[MOTORS_COUNT]) {
    for (int i = 0; i < MOTORS_COUNT; i++) {
        pwm[i] = s_pwm[i];
    }
}
