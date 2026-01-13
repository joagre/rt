/**
 * Interrupt handlers for HAL sensor test
 *
 * Note: SysTick_Handler is defined in system_config.c with hive_timer_tick_isr().
 */
#include "stm32f4xx_hal.h"

void NMI_Handler(void) {}
void HardFault_Handler(void) { while(1); }
void MemManage_Handler(void) { while(1); }
void BusFault_Handler(void) { while(1); }
void UsageFault_Handler(void) { while(1); }
void SVC_Handler(void) {}
void DebugMon_Handler(void) {}
void PendSV_Handler(void) {}
