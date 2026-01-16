// System configuration for STM32F401 (STEVAL-DRONE01)
//
// Clock configuration for 84MHz operation using HSE + PLL.

#include "system_config.h"

// HAL tick increment (defined in stm32f4xx_hal.c)
extern void HAL_IncTick(void);

// Hive runtime timer tick (defined in hive_timer_stm32.c)
extern void hive_timer_tick_isr(void);

// ----------------------------------------------------------------------------
// STM32F4 Register Definitions
// ----------------------------------------------------------------------------

// Base addresses
#define RCC_BASE 0x40023800U
#define FLASH_BASE 0x40023C00U
#define PWR_BASE 0x40007000U
#define SCB_BASE 0xE000ED00U
#define SYSTICK_BASE 0xE000E010U
#define DWT_BASE 0xE0001000U
#define COREDEBUG_BASE 0xE000EDF0U

// RCC registers
#define RCC_CR (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_PLLCFGR (*(volatile uint32_t *)(RCC_BASE + 0x04))
#define RCC_CFGR (*(volatile uint32_t *)(RCC_BASE + 0x08))
#define RCC_AHB1ENR (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_AHB2ENR (*(volatile uint32_t *)(RCC_BASE + 0x34))
#define RCC_APB1ENR (*(volatile uint32_t *)(RCC_BASE + 0x40))
#define RCC_APB2ENR (*(volatile uint32_t *)(RCC_BASE + 0x44))

// RCC_CR bits
#define RCC_CR_HSION (1U << 0)
#define RCC_CR_HSIRDY (1U << 1)
#define RCC_CR_HSEON (1U << 16)
#define RCC_CR_HSERDY (1U << 17)
#define RCC_CR_PLLON (1U << 24)
#define RCC_CR_PLLRDY (1U << 25)

// RCC_CFGR bits
#define RCC_CFGR_SW_HSI 0U
#define RCC_CFGR_SW_HSE 1U
#define RCC_CFGR_SW_PLL 2U
#define RCC_CFGR_SWS_HSI (0U << 2)
#define RCC_CFGR_SWS_HSE (1U << 2)
#define RCC_CFGR_SWS_PLL (2U << 2)
#define RCC_CFGR_SWS_MASK (3U << 2)

// RCC_PLLCFGR bits
#define RCC_PLLCFGR_PLLSRC_HSE (1U << 22)

// RCC_AHB1ENR bits
#define RCC_AHB1ENR_GPIOAEN (1U << 0)
#define RCC_AHB1ENR_GPIOBEN (1U << 1)
#define RCC_AHB1ENR_GPIOCEN (1U << 2)
#define RCC_AHB1ENR_GPIODEN (1U << 3)
#define RCC_AHB1ENR_GPIOEEN (1U << 4)
#define RCC_AHB1ENR_GPIOHEN (1U << 7)
#define RCC_AHB1ENR_DMA1EN (1U << 21)
#define RCC_AHB1ENR_DMA2EN (1U << 22)

// RCC_APB1ENR bits
#define RCC_APB1ENR_TIM2EN (1U << 0)
#define RCC_APB1ENR_TIM3EN (1U << 1)
#define RCC_APB1ENR_TIM4EN (1U << 2)
#define RCC_APB1ENR_TIM5EN (1U << 3)
#define RCC_APB1ENR_SPI2EN (1U << 14)
#define RCC_APB1ENR_SPI3EN (1U << 15)
#define RCC_APB1ENR_USART2EN (1U << 17)
#define RCC_APB1ENR_I2C1EN (1U << 21)
#define RCC_APB1ENR_I2C2EN (1U << 22)
#define RCC_APB1ENR_I2C3EN (1U << 23)
#define RCC_APB1ENR_PWREN (1U << 28)

// RCC_APB2ENR bits
#define RCC_APB2ENR_TIM1EN (1U << 0)
#define RCC_APB2ENR_USART1EN (1U << 4)
#define RCC_APB2ENR_USART6EN (1U << 5)
#define RCC_APB2ENR_SPI1EN (1U << 12)
#define RCC_APB2ENR_SPI4EN (1U << 13)
#define RCC_APB2ENR_SYSCFGEN (1U << 14)

// Flash registers
#define FLASH_ACR (*(volatile uint32_t *)(FLASH_BASE + 0x00))
#define FLASH_ACR_LATENCY_MASK 0x0FU
#define FLASH_ACR_PRFTEN (1U << 8)
#define FLASH_ACR_ICEN (1U << 9)
#define FLASH_ACR_DCEN (1U << 10)

// PWR registers
#define PWR_CR (*(volatile uint32_t *)(PWR_BASE + 0x00))
#define PWR_CR_VOS_SCALE2 (2U << 14) // Scale 2 mode (default)

// SysTick registers
#define SYSTICK_CTRL (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYSTICK_LOAD (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))
#define SYSTICK_VAL (*(volatile uint32_t *)(SYSTICK_BASE + 0x08))

#define SYSTICK_CTRL_ENABLE (1U << 0)
#define SYSTICK_CTRL_TICKINT (1U << 1)
#define SYSTICK_CTRL_CLKSOURCE (1U << 2) // 1 = processor clock

// DWT registers (Data Watchpoint and Trace)
#define DWT_CTRL (*(volatile uint32_t *)(DWT_BASE + 0x00))
#define DWT_CYCCNT (*(volatile uint32_t *)(DWT_BASE + 0x04))
#define DWT_CTRL_CYCCNTENA (1U << 0)

// CoreDebug registers
#define COREDEBUG_DEMCR (*(volatile uint32_t *)(COREDEBUG_BASE + 0x0C))
#define COREDEBUG_DEMCR_TRCENA (1U << 24)

// ----------------------------------------------------------------------------
// Static state
// ----------------------------------------------------------------------------

static volatile uint32_t s_tick_count = 0;

// ----------------------------------------------------------------------------
// Clock Configuration
// ----------------------------------------------------------------------------

bool system_clock_init(void) {
    // -------------------------------------------------------------------------
    // Step 1: Enable HSE (High-Speed External oscillator)
    // -------------------------------------------------------------------------

    RCC_CR |= RCC_CR_HSEON;

    // Wait for HSE to stabilize (with timeout)
    uint32_t timeout = 100000;
    while (!(RCC_CR & RCC_CR_HSERDY)) {
        if (--timeout == 0) {
            return false; // HSE failed to start
        }
    }

    // -------------------------------------------------------------------------
    // Step 2: Enable power controller and set voltage scaling
    // -------------------------------------------------------------------------

    RCC_APB1ENR |= RCC_APB1ENR_PWREN;
    PWR_CR |= PWR_CR_VOS_SCALE2; // Scale 2 mode for 84MHz

    // -------------------------------------------------------------------------
    // Step 3: Configure Flash latency for 84MHz
    // -------------------------------------------------------------------------
    // At 84MHz with 3.3V, we need 2 wait states (see reference manual)

    FLASH_ACR = (FLASH_ACR & ~FLASH_ACR_LATENCY_MASK) | 2U; // 2 wait states
    FLASH_ACR |= FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;

    // -------------------------------------------------------------------------
    // Step 4: Configure PLL
    // -------------------------------------------------------------------------
    // SYSCLK = HSE * PLLN / PLLM / PLLP
    // 84MHz = 16MHz * 336 / 16 / 4

    // Disable PLL before configuration
    RCC_CR &= ~RCC_CR_PLLON;
    while (RCC_CR & RCC_CR_PLLRDY)
        ;

    // Configure PLL
    // PLLM[5:0], PLLN[14:6], PLLP[17:16], PLLSRC[22], PLLQ[27:24]
    uint32_t pllcfgr = 0;
    pllcfgr |= PLL_M;                   // PLLM = 16
    pllcfgr |= (PLL_N << 6);            // PLLN = 336
    pllcfgr |= ((PLL_P / 2 - 1) << 16); // PLLP = 4 (register value = 1)
    pllcfgr |= RCC_PLLCFGR_PLLSRC_HSE;  // PLL source = HSE
    pllcfgr |= (PLL_Q << 24);           // PLLQ = 7 (for USB 48MHz)

    RCC_PLLCFGR = pllcfgr;

    // Enable PLL
    RCC_CR |= RCC_CR_PLLON;

    // Wait for PLL to lock
    timeout = 100000;
    while (!(RCC_CR & RCC_CR_PLLRDY)) {
        if (--timeout == 0) {
            return false; // PLL failed to lock
        }
    }

    // -------------------------------------------------------------------------
    // Step 5: Configure bus clocks (AHB, APB1, APB2)
    // -------------------------------------------------------------------------
    // HCLK = SYSCLK / 1 = 84MHz
    // PCLK1 = HCLK / 2 = 42MHz (APB1, max 42MHz)
    // PCLK2 = HCLK / 1 = 84MHz (APB2, max 84MHz)

    uint32_t cfgr = RCC_CFGR;
    cfgr &= ~(0xF0U); // Clear HPRE (AHB prescaler)
    cfgr |= (0x00U);  // AHB prescaler = 1

    cfgr &= ~(0x1C00U);    // Clear PPRE1 (APB1 prescaler)
    cfgr |= (0x04U << 10); // APB1 prescaler = 2 (HCLK/2)

    cfgr &= ~(0xE000U);    // Clear PPRE2 (APB2 prescaler)
    cfgr |= (0x00U << 13); // APB2 prescaler = 1

    RCC_CFGR = cfgr;

    // -------------------------------------------------------------------------
    // Step 6: Switch system clock to PLL
    // -------------------------------------------------------------------------

    cfgr = RCC_CFGR;
    cfgr &= ~3U;             // Clear SW bits
    cfgr |= RCC_CFGR_SW_PLL; // Select PLL as system clock
    RCC_CFGR = cfgr;

    // Wait for PLL to be used as system clock
    timeout = 100000;
    while ((RCC_CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_PLL) {
        if (--timeout == 0) {
            return false;
        }
    }

    return true;
}

// ----------------------------------------------------------------------------
// SysTick (1ms interrupt)
// ----------------------------------------------------------------------------

void system_tick_init(void) {
    // Configure SysTick for 1ms interrupt
    // Reload value = (SYSCLK / SYSTICK_FREQ) - 1
    uint32_t reload = (SYSCLK_FREQ / SYSTICK_FREQ) - 1;

    SYSTICK_LOAD = reload;
    SYSTICK_VAL = 0;
    SYSTICK_CTRL =
        SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_ENABLE;
}

uint32_t system_get_tick(void) {
    return s_tick_count;
}

void system_delay_ms(uint32_t ms) {
    uint32_t start = s_tick_count;
    while ((s_tick_count - start) < ms) {
        __asm__ volatile("wfi"); // Wait for interrupt (low power)
    }
}

// SysTick interrupt handler (called every 1ms)
void SysTick_Handler(void) {
    s_tick_count++;
    HAL_IncTick();         // Required for HAL_Delay() to work
    hive_timer_tick_isr(); // Required for hive_sleep() to work
}

// ----------------------------------------------------------------------------
// DWT Cycle Counter (microsecond timing)
// ----------------------------------------------------------------------------

void system_dwt_init(void) {
    // Enable DWT (Data Watchpoint and Trace unit)
    COREDEBUG_DEMCR |= COREDEBUG_DEMCR_TRCENA;

    // Reset and enable cycle counter
    DWT_CYCCNT = 0;
    DWT_CTRL |= DWT_CTRL_CYCCNTENA;
}

uint32_t system_get_us(void) {
    // Convert cycles to microseconds
    // cycles / (SYSCLK_FREQ / 1000000) = cycles / 84 for 84MHz
    return DWT_CYCCNT / (SYSCLK_FREQ / 1000000U);
}

void system_delay_us(uint32_t us) {
    uint32_t start = DWT_CYCCNT;
    uint32_t cycles = us * (SYSCLK_FREQ / 1000000U);

    while ((DWT_CYCCNT - start) < cycles)
        ;
}

// ----------------------------------------------------------------------------
// Full System Initialization
// ----------------------------------------------------------------------------

bool system_init(void) {
    // Initialize clocks (HSE + PLL for 84MHz)
    if (!system_clock_init()) {
        system_error_handler();
        return false;
    }

    // Initialize SysTick (1ms interrupt)
    system_tick_init();

    // Initialize DWT cycle counter (microsecond timing)
    system_dwt_init();

    return true;
}

// ----------------------------------------------------------------------------
// Peripheral Clock Control
// ----------------------------------------------------------------------------

void system_enable_gpio(char port) {
    switch (port) {
    case 'A':
    case 'a':
        RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
        break;
    case 'B':
    case 'b':
        RCC_AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
        break;
    case 'C':
    case 'c':
        RCC_AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
        break;
    case 'D':
    case 'd':
        RCC_AHB1ENR |= RCC_AHB1ENR_GPIODEN;
        break;
    case 'E':
    case 'e':
        RCC_AHB1ENR |= RCC_AHB1ENR_GPIOEEN;
        break;
    case 'H':
    case 'h':
        RCC_AHB1ENR |= RCC_AHB1ENR_GPIOHEN;
        break;
    }
}

void system_enable_spi1(void) {
    RCC_APB2ENR |= RCC_APB2ENR_SPI1EN;
}

void system_enable_spi2(void) {
    RCC_APB1ENR |= RCC_APB1ENR_SPI2EN;
}

void system_enable_i2c1(void) {
    RCC_APB1ENR |= RCC_APB1ENR_I2C1EN;
}

void system_enable_i2c2(void) {
    RCC_APB1ENR |= RCC_APB1ENR_I2C2EN;
}

void system_enable_tim2(void) {
    RCC_APB1ENR |= RCC_APB1ENR_TIM2EN;
}

void system_enable_tim4(void) {
    RCC_APB1ENR |= RCC_APB1ENR_TIM4EN;
}

void system_enable_usart1(void) {
    RCC_APB2ENR |= RCC_APB2ENR_USART1EN;
}

void system_enable_usart2(void) {
    RCC_APB1ENR |= RCC_APB1ENR_USART2EN;
}

void system_enable_dma1(void) {
    RCC_AHB1ENR |= RCC_AHB1ENR_DMA1EN;
}

void system_enable_dma2(void) {
    RCC_AHB1ENR |= RCC_AHB1ENR_DMA2EN;
}

// ----------------------------------------------------------------------------
// Error Handler
// ----------------------------------------------------------------------------

__attribute__((weak)) void system_error_handler(void) {
    // Disable interrupts
    __asm__ volatile("cpsid i");

    // Loop forever
    while (1) {
        __asm__ volatile("nop");
    }
}
