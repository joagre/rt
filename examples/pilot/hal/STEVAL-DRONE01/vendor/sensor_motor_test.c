/**
 * Motor Configuration Diagnostic for STEVAL-FCU001V1
 *
 * Comprehensive test to identify motor wiring and configuration:
 *   1. Identifies which TIM4 channel connects to which motor connector (P1/P2/P4/P5)
 *   2. Detects motor rotation direction (CW/CCW) using gyroscope
 *   3. Helps determine correct motor-to-frame-position mapping
 *
 * Usage:
 *   1. REMOVE PROPELLERS or use a test rig!
 *   2. Build: make -f Makefile.STEVAL-DRONE01 TEST=sensor_motor_test
 *   3. Flash: make -f Makefile.STEVAL-DRONE01 TEST=sensor_motor_test flash
 *   4. Connect serial at 115200 baud (P7 header)
 *   5. Follow on-screen instructions
 *   6. Record the output to configure motors.h and hal_stm32.c
 *
 * Expected motor layout for X-quad (looking down):
 *
 *            Front
 *          M2    M3
 *            \  /
 *             \/
 *             /\
 *            /  \
 *          M1    M4
 *            Rear
 *
 *   M1 (rear-left):   CCW rotation
 *   M2 (front-left):  CW rotation
 *   M3 (front-right): CCW rotation
 *   M4 (rear-right):  CW rotation
 */

#include "stm32f4xx_hal.h"
#include "steval_fcu001_v1.h"
#include "steval_fcu001_v1_accelero.h"
#include "steval_fcu001_v1_gyro.h"
#include "usart1.h"

#include <stdbool.h>
#include <stdlib.h>

// ============================================================================
// TIM4 PWM Configuration (PB6, PB7, PB8, PB9)
// ============================================================================

#define TIM4_BASE           0x40000800UL
#define TIM4_CR1            (*(volatile uint32_t *)(TIM4_BASE + 0x00))
#define TIM4_CCMR1          (*(volatile uint32_t *)(TIM4_BASE + 0x18))
#define TIM4_CCMR2          (*(volatile uint32_t *)(TIM4_BASE + 0x1C))
#define TIM4_CCER           (*(volatile uint32_t *)(TIM4_BASE + 0x20))
#define TIM4_PSC            (*(volatile uint32_t *)(TIM4_BASE + 0x28))
#define TIM4_ARR            (*(volatile uint32_t *)(TIM4_BASE + 0x2C))
#define TIM4_CCR1           (*(volatile uint32_t *)(TIM4_BASE + 0x34))
#define TIM4_CCR2           (*(volatile uint32_t *)(TIM4_BASE + 0x38))
#define TIM4_CCR3           (*(volatile uint32_t *)(TIM4_BASE + 0x3C))
#define TIM4_CCR4           (*(volatile uint32_t *)(TIM4_BASE + 0x40))

#define RCC_BASE            0x40023800UL
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_APB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x40))

#define GPIOB_BASE          0x40020400UL
#define GPIOB_MODER         (*(volatile uint32_t *)(GPIOB_BASE + 0x00))
#define GPIOB_OSPEEDR       (*(volatile uint32_t *)(GPIOB_BASE + 0x08))
#define GPIOB_AFRL          (*(volatile uint32_t *)(GPIOB_BASE + 0x20))
#define GPIOB_AFRH          (*(volatile uint32_t *)(GPIOB_BASE + 0x24))

// PWM: 16MHz / 16 / 1000 = 1kHz
#define PWM_PRESCALER       15
#define PWM_PERIOD          999

// Test parameters
#define TEST_SPEED          150     // 15% duty cycle - enough to spin but safe
#define SPIN_DURATION_MS    2000    // How long to spin each motor
#define GYRO_SAMPLES        50      // Gyro samples during spin

// ============================================================================
// Channel info structure
// ============================================================================

typedef struct {
    int channel;            // 0-3 (TIM4 CH1-CH4)
    const char *pin_name;   // GPIO pin name
    const char *connector;  // Observed connector (filled by user)
    int rotation;           // Detected: +1=CW, -1=CCW, 0=unknown
    int gyro_z_sum;         // Accumulated gyro Z reading
} channel_info_t;

static channel_info_t g_channels[4] = {
    {0, "PB6 (TIM4_CH1)", "?", 0, 0},
    {1, "PB7 (TIM4_CH2)", "?", 0, 0},
    {2, "PB8 (TIM4_CH3)", "?", 0, 0},
    {3, "PB9 (TIM4_CH4)", "?", 0, 0},
};

// Gyroscope handle
static void *g_gyro_handle = NULL;

// ============================================================================
// Helper Functions
// ============================================================================

static void blink_n(int n, int on_ms, int off_ms) {
    for (int i = 0; i < n; i++) {
        BSP_LED_On(LED1);
        HAL_Delay(on_ms);
        BSP_LED_Off(LED1);
        HAL_Delay(off_ms);
    }
    HAL_Delay(300);
}

static bool motors_init(void) {
    // Enable clocks
    RCC_AHB1ENR |= (1 << 1);  // GPIOBEN
    RCC_APB1ENR |= (1 << 2);  // TIM4EN
    for (volatile int i = 0; i < 100; i++);

    // Configure PB6-PB9 as AF2 (TIM4)
    GPIOB_MODER &= ~((3 << 12) | (3 << 14) | (3 << 16) | (3 << 18));
    GPIOB_MODER |=  ((2 << 12) | (2 << 14) | (2 << 16) | (2 << 18));
    GPIOB_OSPEEDR |= ((3 << 12) | (3 << 14) | (3 << 16) | (3 << 18));
    GPIOB_AFRL &= ~((0xF << 24) | (0xF << 28));
    GPIOB_AFRL |=  ((2 << 24) | (2 << 28));
    GPIOB_AFRH &= ~((0xF << 0) | (0xF << 4));
    GPIOB_AFRH |=  ((2 << 0) | (2 << 4));

    // Configure TIM4 PWM
    TIM4_PSC = PWM_PRESCALER;
    TIM4_ARR = PWM_PERIOD;
    TIM4_CCMR1 = (6 << 4) | (1 << 3) | (6 << 12) | (1 << 11);
    TIM4_CCMR2 = (6 << 4) | (1 << 3) | (6 << 12) | (1 << 11);
    TIM4_CCER = (1 << 0) | (1 << 4) | (1 << 8) | (1 << 12);
    TIM4_CCR1 = 0;
    TIM4_CCR2 = 0;
    TIM4_CCR3 = 0;
    TIM4_CCR4 = 0;
    TIM4_CR1 = 1;

    return true;
}

static void motor_set(int channel, uint16_t speed) {
    if (speed > PWM_PERIOD) speed = PWM_PERIOD;
    switch (channel) {
        case 0: TIM4_CCR1 = speed; break;
        case 1: TIM4_CCR2 = speed; break;
        case 2: TIM4_CCR3 = speed; break;
        case 3: TIM4_CCR4 = speed; break;
    }
}

static void motors_stop_all(void) {
    TIM4_CCR1 = 0;
    TIM4_CCR2 = 0;
    TIM4_CCR3 = 0;
    TIM4_CCR4 = 0;
}

static bool gyro_init(void) {
    if (Sensor_IO_SPI_Init() != COMPONENT_OK) return false;
    Sensor_IO_SPI_CS_Init_All();
    if (BSP_GYRO_Init(LSM6DSL_G_0, &g_gyro_handle) != COMPONENT_OK) return false;
    BSP_GYRO_Sensor_Enable(g_gyro_handle);
    return true;
}

static int read_gyro_z(void) {
    SensorAxes_t gyro;
    if (BSP_GYRO_Get_Axes(g_gyro_handle, &gyro) == COMPONENT_OK) {
        return (int)gyro.AXIS_Z;  // mdps (milli-degrees per second)
    }
    return 0;
}

// ============================================================================
// Test Functions
// ============================================================================

static void test_single_motor(int channel) {
    channel_info_t *info = &g_channels[channel];

    usart1_puts("\r\n");
    usart1_puts("============================================================\r\n");
    usart1_printf("  TESTING CHANNEL %d: %s\r\n", channel + 1, info->pin_name);
    usart1_puts("============================================================\r\n");
    usart1_puts("\r\n");
    usart1_puts("  >>> OBSERVE: Which connector's motor is spinning? <<<\r\n");
    usart1_puts("  >>> OBSERVE: Is it spinning CW or CCW (looking down)? <<<\r\n");
    usart1_puts("\r\n");

    // Blink to indicate which motor (1-4 blinks)
    blink_n(channel + 1, 150, 150);

    usart1_printf("  Starting motor on CH%d for %d seconds...\r\n",
                  channel + 1, SPIN_DURATION_MS / 1000);

    // Clear gyro accumulator
    info->gyro_z_sum = 0;

    // Spin up motor
    motor_set(channel, TEST_SPEED);

    // Wait a bit for motor to reach speed
    HAL_Delay(200);

    // Sample gyro while spinning
    int sample_interval = (SPIN_DURATION_MS - 400) / GYRO_SAMPLES;
    for (int i = 0; i < GYRO_SAMPLES; i++) {
        info->gyro_z_sum += read_gyro_z();
        BSP_LED_Toggle(LED1);
        HAL_Delay(sample_interval);
    }
    BSP_LED_Off(LED1);

    // Stop motor
    motor_set(channel, 0);
    HAL_Delay(500);  // Let it spin down

    // Analyze rotation direction from gyro
    // Positive gyro Z = CCW rotation (right-hand rule, Z up)
    // Negative gyro Z = CW rotation
    int avg_gyro = info->gyro_z_sum / GYRO_SAMPLES;

    usart1_printf("  Gyro Z average: %d mdps\r\n", avg_gyro);

    // Threshold for detection (motor should produce significant rotation)
    if (avg_gyro > 5000) {
        info->rotation = -1;  // CCW (positive gyro = drone rotating CCW)
        usart1_puts("  Detected rotation: CCW (counter-clockwise)\r\n");
    } else if (avg_gyro < -5000) {
        info->rotation = 1;   // CW
        usart1_puts("  Detected rotation: CW (clockwise)\r\n");
    } else {
        info->rotation = 0;
        usart1_puts("  Detected rotation: UNCLEAR (gyro reading too low)\r\n");
        usart1_puts("  -> Motor may not be connected or spinning too slow\r\n");
    }

    usart1_puts("\r\n");
}

static void run_diagonal_test(void) {
    usart1_puts("\r\n");
    usart1_puts("============================================================\r\n");
    usart1_puts("  DIAGONAL PAIR TEST\r\n");
    usart1_puts("============================================================\r\n");
    usart1_puts("\r\n");
    usart1_puts("  Testing diagonal pairs to identify frame positions.\r\n");
    usart1_puts("  Diagonal motors should spin in SAME direction.\r\n");
    usart1_puts("\r\n");

    usart1_puts("--- Test 1: CH1 + CH4 (should be diagonals if wired correctly) ---\r\n");
    blink_n(2, 100, 100);
    motor_set(0, TEST_SPEED);  // CH1
    motor_set(3, TEST_SPEED);  // CH4
    HAL_Delay(2000);
    motors_stop_all();
    HAL_Delay(500);

    usart1_puts("--- Test 2: CH2 + CH3 (should be diagonals if wired correctly) ---\r\n");
    blink_n(3, 100, 100);
    motor_set(1, TEST_SPEED);  // CH2
    motor_set(2, TEST_SPEED);  // CH3
    HAL_Delay(2000);
    motors_stop_all();
    HAL_Delay(500);
}

static void run_all_motors_test(void) {
    usart1_puts("\r\n");
    usart1_puts("============================================================\r\n");
    usart1_puts("  ALL MOTORS TEST\r\n");
    usart1_puts("============================================================\r\n");
    usart1_puts("\r\n");
    usart1_puts("  All 4 motors spinning together.\r\n");
    usart1_puts("  If wired correctly, drone should NOT rotate (yaw balanced).\r\n");
    usart1_puts("\r\n");

    blink_n(4, 100, 100);

    int gyro_sum = 0;
    motor_set(0, TEST_SPEED);
    motor_set(1, TEST_SPEED);
    motor_set(2, TEST_SPEED);
    motor_set(3, TEST_SPEED);

    HAL_Delay(300);

    for (int i = 0; i < 30; i++) {
        gyro_sum += read_gyro_z();
        BSP_LED_Toggle(LED1);
        HAL_Delay(50);
    }
    BSP_LED_Off(LED1);

    motors_stop_all();

    int avg = gyro_sum / 30;
    usart1_printf("  Gyro Z average (all motors): %d mdps\r\n", avg);

    if (abs(avg) < 3000) {
        usart1_puts("  Result: BALANCED - Yaw torques cancel out correctly!\r\n");
    } else if (avg > 0) {
        usart1_puts("  Result: ROTATING CCW - CW motors too weak or CCW too strong\r\n");
    } else {
        usart1_puts("  Result: ROTATING CW - CCW motors too weak or CW too strong\r\n");
    }
}

static void print_summary(void) {
    usart1_puts("\r\n");
    usart1_puts("============================================================\r\n");
    usart1_puts("  MOTOR CONFIGURATION SUMMARY\r\n");
    usart1_puts("============================================================\r\n");
    usart1_puts("\r\n");
    usart1_puts("  TIM4 Channel -> Detected Rotation\r\n");
    usart1_puts("  ---------------------------------\r\n");

    for (int i = 0; i < 4; i++) {
        const char *rot_str = "???";
        if (g_channels[i].rotation == 1) rot_str = "CW";
        else if (g_channels[i].rotation == -1) rot_str = "CCW";

        usart1_printf("  CH%d (%s): %s\r\n",
                      i + 1, g_channels[i].pin_name, rot_str);
    }

    usart1_puts("\r\n");
    usart1_puts("  NEXT STEPS:\r\n");
    usart1_puts("  -----------\r\n");
    usart1_puts("  1. Note which physical connector (P1/P2/P4/P5) each channel drives\r\n");
    usart1_puts("  2. Note the position on frame (front-left, rear-right, etc.)\r\n");
    usart1_puts("  3. Update motors.h with correct mapping\r\n");
    usart1_puts("\r\n");
    usart1_puts("  EXPECTED X-QUAD CONFIGURATION:\r\n");
    usart1_puts("  ------------------------------\r\n");
    usart1_puts("              Front\r\n");
    usart1_puts("           M2(CW)  M3(CCW)\r\n");
    usart1_puts("               \\  /\r\n");
    usart1_puts("                \\/\r\n");
    usart1_puts("                /\\\r\n");
    usart1_puts("               /  \\\r\n");
    usart1_puts("           M1(CCW) M4(CW)\r\n");
    usart1_puts("              Rear\r\n");
    usart1_puts("\r\n");
    usart1_puts("  Diagonal pairs MUST have same rotation:\r\n");
    usart1_puts("    - M1 + M3 = both CCW\r\n");
    usart1_puts("    - M2 + M4 = both CW\r\n");
    usart1_puts("\r\n");

    // Check if detected rotations match expected pattern
    int ch1_rot = g_channels[0].rotation;
    int ch2_rot = g_channels[1].rotation;
    int ch3_rot = g_channels[2].rotation;
    int ch4_rot = g_channels[3].rotation;

    if (ch1_rot != 0 && ch2_rot != 0 && ch3_rot != 0 && ch4_rot != 0) {
        // Check diagonal pairing
        if (ch1_rot == ch4_rot && ch2_rot == ch3_rot && ch1_rot != ch2_rot) {
            usart1_puts("  DIAGONAL CHECK: PASS - CH1+CH4 same, CH2+CH3 same, opposite pairs\r\n");

            if (ch1_rot == -1) {  // CH1=CCW, CH4=CCW, CH2=CW, CH3=CW
                usart1_puts("  MAPPING: CH1->M1, CH2->M2, CH3->M3, CH4->M4 (or rotated)\r\n");
            } else {  // CH1=CW, CH4=CW, CH2=CCW, CH3=CCW
                usart1_puts("  MAPPING: CH1->M2, CH2->M1, CH3->M4, CH4->M3 (or rotated)\r\n");
            }
        } else if (ch1_rot == ch3_rot && ch2_rot == ch4_rot && ch1_rot != ch2_rot) {
            usart1_puts("  DIAGONAL CHECK: ALTERNATE - CH1+CH3 same, CH2+CH4 same\r\n");
            usart1_puts("  This suggests CH1/CH3 are diagonal, CH2/CH4 are diagonal\r\n");
        } else {
            usart1_puts("  DIAGONAL CHECK: FAIL - Rotation pattern doesn't match X-quad!\r\n");
            usart1_puts("  Check motor wiring or propeller direction.\r\n");
        }
    } else {
        usart1_puts("  DIAGONAL CHECK: INCOMPLETE - Some rotations not detected\r\n");
    }

    usart1_puts("\r\n");
}

// ============================================================================
// Main
// ============================================================================

int main(void) {
    SystemCoreClock = 16000000;
    HAL_Init();

    BSP_LED_Init(LED1);
    BSP_LED_Off(LED1);

    usart1_init(NULL);

    usart1_puts("\r\n");
    usart1_puts("************************************************************\r\n");
    usart1_puts("*                                                          *\r\n");
    usart1_puts("*    STEVAL-FCU001V1 MOTOR CONFIGURATION DIAGNOSTIC        *\r\n");
    usart1_puts("*                                                          *\r\n");
    usart1_puts("************************************************************\r\n");
    usart1_puts("\r\n");
    usart1_puts("  This test helps identify motor wiring configuration:\r\n");
    usart1_puts("    - Which TIM4 channel drives which connector (P1/P2/P4/P5)\r\n");
    usart1_puts("    - Motor rotation direction (CW/CCW)\r\n");
    usart1_puts("    - Correct mixer configuration for your wiring\r\n");
    usart1_puts("\r\n");
    usart1_puts("  !!! WARNING: REMOVE ALL PROPELLERS BEFORE CONTINUING !!!\r\n");
    usart1_puts("\r\n");
    usart1_puts("  Press RESET to restart test at any time.\r\n");
    usart1_puts("\r\n");

    // Initialize gyroscope
    usart1_puts("Initializing gyroscope...\r\n");
    if (!gyro_init()) {
        usart1_puts("FATAL: Gyroscope init failed!\r\n");
        while (1) { BSP_LED_Toggle(LED1); HAL_Delay(1000); }
    }
    usart1_puts("  Gyroscope OK\r\n\r\n");

    // Initialize motors
    usart1_puts("Initializing motor PWM (TIM4 CH1-CH4)...\r\n");
    if (!motors_init()) {
        usart1_puts("FATAL: Motor init failed!\r\n");
        while (1) { BSP_LED_Toggle(LED1); HAL_Delay(1000); }
    }
    usart1_puts("  Motors OK\r\n");

    // Countdown before starting
    usart1_puts("\r\n");
    usart1_puts("Starting motor tests in 3 seconds...\r\n");
    usart1_puts("  >>> WATCH THE MOTORS AND NOTE WHICH CONNECTOR SPINS! <<<\r\n");
    usart1_puts("\r\n");
    for (int i = 3; i > 0; i--) {
        usart1_printf("  %d...\r\n", i);
        blink_n(1, 200, 800);
    }

    // Test each motor individually
    usart1_puts("\r\n");
    usart1_puts("========== PHASE 1: INDIVIDUAL MOTOR TESTS ==========\r\n");

    for (int ch = 0; ch < 4; ch++) {
        test_single_motor(ch);
    }

    // Test diagonal pairs
    usart1_puts("\r\n");
    usart1_puts("========== PHASE 2: DIAGONAL PAIR TESTS ==========\r\n");
    run_diagonal_test();

    // Test all motors
    usart1_puts("\r\n");
    usart1_puts("========== PHASE 3: ALL MOTORS TEST ==========\r\n");
    run_all_motors_test();

    // Print summary
    print_summary();

    usart1_puts("\r\n");
    usart1_puts("************************************************************\r\n");
    usart1_puts("*  TEST COMPLETE - Record results and update motor config  *\r\n");
    usart1_puts("************************************************************\r\n");
    usart1_puts("\r\n");

    // Done - fast blink
    while (1) {
        BSP_LED_Toggle(LED1);
        HAL_Delay(100);
    }
}

void HAL_MspInit(void) {
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
}
