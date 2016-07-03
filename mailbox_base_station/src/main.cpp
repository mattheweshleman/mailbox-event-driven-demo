#include "mbed.h"
#include "myDebug.h"
#include "BaseStationManager.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "stm32l1xx.h"
#include "stm32l1xx_hal_cortex.h"
#include "core_cmFunc.h"

static Serial m_debug_uart(USBTX, USBRX); //Use default TX and RX. Available via USB Com port when using PGM-NUCLEO programmer

/**
 *
 */
int main()
{
   m_debug_uart.baud(115200);

   debug_if(1, "\nStarting Base Station\n\n\r");

   BaseStationManager::Init();

   //mbed has all interrupts at highest priority,
   //but FreeRTOS requires at least 1 level lower
   //than max
   NVIC_SetPriority(EXTI0_IRQn, 5);
   NVIC_SetPriority(EXTI15_10_IRQn, 5);
   NVIC_SetPriority(EXTI1_IRQn, 5);
   NVIC_SetPriority(EXTI2_IRQn, 5);
   NVIC_SetPriority(EXTI3_IRQn, 5);
   NVIC_SetPriority(EXTI4_IRQn, 5);
   NVIC_SetPriority(EXTI9_5_IRQn, 5);
   NVIC_SetPriority(TIM5_IRQn, 5);     //mbed ticker

   vTaskStartScheduler();

   while (1)
   {
      debug_if(1, "Should not get here!\n");
      wait_ms(1000);
   }
}

/**
 *
 */
extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask,
      signed char *pcTaskName)
{
   debug_if(1, "stack overflow hook hit!\n");
}

