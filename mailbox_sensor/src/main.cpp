#include "mbed.h"
#include "sx1276-inAir.h"
#include "myDebug.h"
#include "app_config_data_types.hpp"
#include "radio_settings.h"
#include "sensor_msg.h"
#include "core_cmFunc.h"
#include "SensorStatemachine.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "main_queue.h"
#include "rtc_api.h"
#include "rtc_time.h"

#define NZ32_ST1L_REV2

static SensorStatemachine m_statemachine;
static Serial m_debug_uart(USBTX, USBRX); //Use default TX and RX. Available via USB Com port when using PGM-NUCLEO programmer

/* Set this flag to '1' to display debug messages on the console */
#define DEBUG_MESSAGE   1

// GLOBAL VARIABLES ///////////////////////////////////////////////////////////
AppData appData;
QueueHandle_t m_main_queue = nullptr;

/* The Sensor Task simply reads events
 * from the main queue and feeds them
 * to the Sensor state machine. */
static void SensorTask(void *)
{
   m_main_queue = xQueueCreate(5, sizeof(SensorEvent));
   if (m_main_queue == nullptr)
   {
      debug_if(DEBUG_MESSAGE, "Fatal! Main queue create failed!\n");
   }

   //mbed has all interrupts at highest priority,
   //but FreeRTOS requires at least 1 level lower
   //than max.
   NVIC_SetPriority(EXTI0_IRQn, 5);
   NVIC_SetPriority(EXTI15_10_IRQn, 5);
   NVIC_SetPriority(EXTI1_IRQn, 5);
   NVIC_SetPriority(EXTI2_IRQn, 5);
   NVIC_SetPriority(EXTI3_IRQn, 5);
   NVIC_SetPriority(EXTI4_IRQn, 5);
   NVIC_SetPriority(EXTI9_5_IRQn, 5);
   NVIC_SetPriority(TIM5_IRQn, 5);     //mbed ticker

   SensorEvent event;
   m_statemachine.Init();

   while (1)
   {
      if (xQueueReceive(m_main_queue, &(event), (TickType_t ) portMAX_DELAY))
      {
         m_statemachine.ProcessEvent(event);
      }
   }
}

/** Main function */
int main()
{
   m_debug_uart.baud(115200);

   //Configure default appData
   memset(&appData, 0, sizeof(appData));

   appData.boardType = BOARD_INAIR9; //Use BOARD_INAIR9, it is a 14dBm output

   debug_if(DEBUG_MESSAGE, "\nStarted mailbox sensor app.\n");

   //Startup Delay (per original Modtronix example code)
   wait_ms(100);

   //keep power usage as low as possible, make certain RTC is off.
   rtc_free();

   xTaskCreate(SensorTask, "NAME", configMINIMAL_STACK_SIZE*2,
      nullptr, tskIDLE_PRIORITY, nullptr);

   vTaskStartScheduler();

   debug_if(DEBUG_MESSAGE, "Fatal! Should not get here!\n");

   while (1)
   {
      wait_ms(100);
   }
}

/**
 *
 */
void InjectEvent(SensorEvent event)
{
   BaseType_t rtn = xQueueSend(m_main_queue, &event, 100);
   if (rtn != pdPASS)
   {
      debug_if(DEBUG_MESSAGE, "Queue Send failure!\n");
   }
}
