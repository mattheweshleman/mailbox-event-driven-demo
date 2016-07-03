/*
 * SensorStatemachine.cpp
 *
 *  Created on: Apr 24, 2016
 *      Author: Matthew Eshleman
 */

#include "SensorStatemachine.h"
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
#include "timers.h"
#include "ack_msg.h"

#define DEBUG_MESSAGE   1

static constexpr uint32_t RX_TIMEOUT_ONE_SECOND = 1000000;    // in microseconds

extern QueueHandle_t m_main_queue;
static TimerHandle_t m_timer = 0;
static SX1276inAir*  m_radio = nullptr;
static SensorMsg     m_msg_to_send;
static DigitalOut    m_led(LED1);
static uint32_t      m_retry_count = 0;
static DigitalInOut  m_radio_rf_switch(PC_13);

static void CreateTheRadio(void);
static void PlaceMcuIntoDeepSleep();
static void OnTenSecondTimerCallback( TimerHandle_t xTimer );

/******************************************************
 *  NOTE: All "OnRadioXyz()" methods are called from
 *        various ISR contexts!
 ******************************************************/
static void OnRadioTxDone(void);
static void OnRadioRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
static void OnRadioTxTimeout(void);
static void OnRadioRxTimeout(void);
static void OnRadioRxError(void);

/**
 *
 */
SensorStatemachine::SensorStatemachine() :
      mCurrent(nullptr)
{
}

/**
 *
 */
SensorStatemachine::~SensorStatemachine()
{
}

/**
 *
 */
void SensorStatemachine::SensorStatemachine::Init()
{
   m_retry_count = 0;
   m_led = 0;

   //disable all WKUP pins
   HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN1);
   HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN2);
   HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN3);

   m_timer = xTimerCreate("TO_10", pdMS_TO_TICKS(10*1000),
         pdFALSE, nullptr, OnTenSecondTimerCallback);
   if(m_timer == NULL)
   {
      debug_if(DEBUG_MESSAGE, "Fatal: Unable to create timer!\n");
   }

   mCurrent = InitRadio;
   ProcessEvent(SensorEvent::ENTER_STATE);
}

/**
 *
 */
void SensorStatemachine::ProcessEvent(const SensorEvent event)
{
   if (mCurrent != nullptr)
   {
      Handler new_handler = mCurrent(this, event);
      if (new_handler != mCurrent)
      {
         mCurrent(this, SensorEvent::EXIT_STATE);
         mCurrent = new_handler;
         mCurrent(this, SensorEvent::ENTER_STATE);
      }
   }
}

/**
 *
 */
SensorStatemachine::SmHandler_ SensorStatemachine::InitRadio(SensorStatemachine*,
      const SensorEvent event)
{
   //Create and find the SX1276inAir radio
   switch (event)
   {
   case SensorEvent::ENTER_STATE:
      //we cut the J1 jumper for power to the RF switch, which was applying power always.
      //Need to turn it on manually now.
      m_radio_rf_switch = true;
      m_led = 1;
      debug_if(DEBUG_MESSAGE, "Init Radio Enter\n");
      CreateTheRadio();
      InjectEvent(SensorEvent::RADIO_CREATED);
      break;
   case SensorEvent::RADIO_CREATED:
      return SendMsg;
      break;
   default:
      break;
   }
   return InitRadio;
}

/**
 *
 */
SensorStatemachine::SmHandler_ SensorStatemachine::SendMsg(SensorStatemachine*,
      const SensorEvent event)
{
   switch (event)
   {
   case SensorEvent::ENTER_STATE:
      debug_if(DEBUG_MESSAGE, "SendMsg Enter\n");
      //With the current HW, unable to
      //measure the battery voltage when
      //running exclusively off the battery.
      m_msg_to_send.is_low = false;
      m_msg_to_send.battery_mV = 0;
      m_radio->Send((uint8_t*)&m_msg_to_send, sizeof(SensorMsg));
      break;
   case SensorEvent::MSG_SENT:
      return ReceiveAck;
      break;
   case SensorEvent::MSG_SENT_ERROR:
      m_radio->Send((uint8_t*)&m_msg_to_send, sizeof(SensorMsg));
      break;
   default:
      break;
   }
   return SendMsg;
}

/**
 *
 */
SensorStatemachine::SmHandler_ SensorStatemachine::ReceiveAck(SensorStatemachine*,
      const SensorEvent event)
{
   switch (event)
   {
   case SensorEvent::ENTER_STATE:
      debug_if(DEBUG_MESSAGE, "Receive Ack Enter\n");
      xTimerStart(m_timer, 100);
      m_radio->Rx(RX_TIMEOUT_ONE_SECOND*6);
      break;
   case SensorEvent::EXIT_STATE:
      xTimerStop(m_timer, 100);
      break;
   case SensorEvent::ACK_RXD_OK:
      return DeepSleep;
      break;
   case SensorEvent::TIMEOUT_10_SEC:
      m_retry_count++;
      if(m_retry_count > 12)
      {
         debug_if(DEBUG_MESSAGE, "Too many retries, bailing out to deep sleep\n");
         return DeepSleep;
      }
      else
      {
         return SendMsg;
      }
      break;
   default:
      break;
   }
   return ReceiveAck;
}

/**
 *
 */
SensorStatemachine::SmHandler_ SensorStatemachine::DeepSleep(SensorStatemachine*,
      const SensorEvent event)
{
   switch (event)
   {
   case SensorEvent::ENTER_STATE:
      debug_if(DEBUG_MESSAGE, "DeepSleep Enter\n");

      m_led = 0;

      /* Point of no return! */
      PlaceMcuIntoDeepSleep();

      /* should not get here! */
      debug_if(DEBUG_MESSAGE, "Very strange?!? After enter deep sleep standby mode\n");
      break;
   default:
      break;
   }
   return DeepSleep;
}

/**
 *   CreateTheRadio()
 */
void CreateTheRadio(void)
{
   if (m_radio != nullptr)
   {
      debug_if(DEBUG_MESSAGE,
         "ASSERT: radio exists, why call CreateTheRadio() ??\n");
      return;
   }

   //Create and Initialize instance of SX1276inAir
   m_radio = new SX1276inAir(OnRadioTxDone, OnRadioTxTimeout, OnRadioRxDone, OnRadioRxTimeout,
      OnRadioRxError, NULL, NULL);
   m_radio->SetBoardType(BOARD_INAIR9);
   wait_ms(10);

   //Wait for radio to be detected
   while (m_radio->Read(REG_VERSION) == 0x00)
   {
      wait_ms(400);
      debug_if(DEBUG_MESSAGE, "Waiting on radio....\n", NULL);
   }

   //Initialize radio
   m_radio->SetChannel(RF_FREQUENCY);

   m_radio->SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
   LORA_SPREADING_FACTOR, LORA_CODINGRATE,
   LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
   LORA_CRC_ENABLED, LORA_FHSS_ENABLED, LORA_NB_SYMB_HOP,
   LORA_IQ_INVERSION_ON, 2000000);

   m_radio->SetRxConfig(MODEM_LORA, LORA_BANDWIDTH,
   LORA_SPREADING_FACTOR, LORA_CODINGRATE, 0,
   LORA_PREAMBLE_LENGTH, LORA_SYMBOL_TIMEOUT,
   LORA_FIX_LENGTH_PAYLOAD_ON, 0, LORA_CRC_ENABLED,
   LORA_FHSS_ENABLED, LORA_NB_SYMB_HOP, LORA_IQ_INVERSION_ON, true);
}

/**
 *
 */
void OnRadioTxDone(void)
{
   m_radio->Sleep();

   SensorEvent event = SensorEvent::MSG_SENT;
   xQueueSendToBackFromISR(m_main_queue, &event, nullptr);
}

/**
 *
 */
void OnRadioRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
   static const AckMsg m_valid_ack;

   if(size != sizeof(AckMsg))
   {
      debug_if(DEBUG_MESSAGE, "Rx size is not expected, size = %d\n", size);
      m_radio->Sleep();
   }
   else
   {
      m_radio->Sleep();
      if(0 == memcmp(&m_valid_ack, payload, sizeof(AckMsg)))
      {
         SensorEvent event = SensorEvent::ACK_RXD_OK;
         xQueueSendToBackFromISR(m_main_queue, &event, nullptr);
      }
      else
      {
         debug_if(DEBUG_MESSAGE, "size matched, but rx msg contents do not pass\n", size);
      }
   }
}

/**
 *
 */
void OnRadioTxTimeout(void)
{
   debug_if(DEBUG_MESSAGE, "OnTxTimeout\n");
   m_radio->Sleep();

   SensorEvent event = SensorEvent::MSG_SENT_ERROR;
   xQueueSendToBackFromISR(m_main_queue, &event, nullptr);
}

/**
 *
 */
void OnRadioRxTimeout(void)
{
   debug_if(DEBUG_MESSAGE, "OnRxTimeout\n");
   m_radio->Sleep();
}

/**
 *
 */
void OnRadioRxError(void)
{
   debug_if(DEBUG_MESSAGE, "OnRxError\n");
   m_radio->Sleep();
}

/**
 *  PlaceMcuIntoDeepSleep()
 *    Places the micro into deep sleep
 *    and configures PA0 as the wakeup pin.
 *
 *    This method does not return.
 *    When the microcontroller wakes up
 *    it will be the equivalent of a
 *    power on reset.
 */
void PlaceMcuIntoDeepSleep()
{
   //enable PA0 WKUP pin
   HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);

   __HAL_SYSCFG_VREFINT_OUT_DISABLE();

   HAL_PWR_DisablePVD();

   //standby...point of no return.
   HAL_PWR_EnterSTANDBYMode();
}

/**
 *
 */
void OnTenSecondTimerCallback( TimerHandle_t xTimer )
{
   InjectEvent(SensorEvent::TIMEOUT_10_SEC);
}
