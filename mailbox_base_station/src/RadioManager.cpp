/*
 * RadioManager.cpp
 *
 *  Created on: May 28, 2016
 *      Author: Matthew Eshleman
 */

#include "RadioManager.h"
#include <cstdint>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "mbed.h"
#include "sx1276-inAir.h"
#include "myDebug.h"
#include "app_config_data_types.hpp"
#include "radio_settings.h"
#include "sensor_msg.h"
#include "core_cmFunc.h"
#include "ack_msg.h"

namespace RadioManager
{

enum class RadioEvent : char
{
   ENTER_STATE,  //internal use only
   EXIT_STATE,   //internal use only
   RADIO_CREATED,
   MSG_SENT_OK,
   SENSOR_MSG_RXD_OK,
   MSG_SEND_ERROR,
   RX_TIMEOUT,
   RX_ERROR
};

static void InjectEvent(RadioEvent event);

/**
 * simple flat state machine defining radio manager
 * behavior.
 */
class Statemachine
{
public:
   Statemachine() : mCurrent(nullptr)  {}
   virtual ~Statemachine()  {}

   void Init();
   void ProcessEvent(const RadioEvent event);

private:
   /*
    * Check this out... http://www.gotw.ca/gotw/057.htm
    *   For background on the below.
    */
   struct Handler_;
   typedef Handler_ (*Handler)(Statemachine*, const RadioEvent);
   struct Handler_
   {
      Handler_(Handler pp) :
            p(pp)
      {
      }
      operator Handler()
      {
         return p;
      }
      Handler p;
   };

   static Handler_ InitRadio(Statemachine* me, const RadioEvent event);
   static Handler_ Receive(Statemachine* me, const RadioEvent event);
   static Handler_ SendAck(Statemachine* me, const RadioEvent event);

   Handler mCurrent;
};

static constexpr uint32_t RX_TIMEOUT_ONE_SECOND = 1000000;    // in us

static QueueHandle_t m_queue = nullptr;
static SX1276inAir* m_radio = nullptr;
static RadioEventHandler m_sensorMsgReceivedHandler = nullptr;
static void RadioManagerTask(void *);
static void CreateTheRadio(void);

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
void Init(RadioEventHandler sensorMsgRxEventHandler)
{
   m_sensorMsgReceivedHandler = sensorMsgRxEventHandler;
   auto rtn = xTaskCreate(RadioManagerTask, "RadioManager",
      configMINIMAL_STACK_SIZE*2, nullptr, tskIDLE_PRIORITY+1, nullptr);
   if (rtn != pdPASS)
   {
      debug_if(1, "radiomanager failed task create!\n");
   }
}

/**
 *
 */
void RadioManagerTask(void*)
{
   debug_if(1, "Starting Radio Manager Task\n");

   m_queue = xQueueCreate(5, sizeof(RadioEvent));
   if (m_queue == nullptr)
   {
      return;
   }

   Statemachine statemachine;
   statemachine.Init();

   RadioEvent event;
   while (1)
   {
      if (xQueueReceive(m_queue, &(event), (TickType_t ) portMAX_DELAY))
      {
         statemachine.ProcessEvent(event);
      }
   }
}

/**
 *
 */
void Statemachine::Init()
{
   mCurrent = InitRadio;
   ProcessEvent(RadioEvent::ENTER_STATE);
}

/**
 *
 */
void Statemachine::ProcessEvent(const RadioEvent event)
{
   if (mCurrent != nullptr)
   {
      Handler new_handler = mCurrent(this, event);
      if (new_handler != mCurrent)
      {
         mCurrent(this, RadioEvent::EXIT_STATE);
         mCurrent = new_handler;
         mCurrent(this, RadioEvent::ENTER_STATE);
      }
   }
}

/**
 *
 */
Statemachine::Handler_ Statemachine::InitRadio(Statemachine* me,
      const RadioEvent event)
{
   switch (event)
   {
   case RadioEvent::ENTER_STATE:
      //Create and find the SX1276inAir radio
      CreateTheRadio();
      InjectEvent(RadioEvent::RADIO_CREATED);
      break;
   case RadioEvent::RADIO_CREATED:
      return Receive;
      break;
   default:
      break;
   }
   return InitRadio;
}

/**
 *
 */
Statemachine::Handler_ Statemachine::Receive(Statemachine* me,
      const RadioEvent event)
{
   switch (event)
   {
   case RadioEvent::RX_ERROR:
   case RadioEvent::RX_TIMEOUT:
   case RadioEvent::ENTER_STATE:
      m_radio->Rx(RX_TIMEOUT_ONE_SECOND * 30);
      break;
   case RadioEvent::SENSOR_MSG_RXD_OK:
      if (m_sensorMsgReceivedHandler)
      {
         m_sensorMsgReceivedHandler();
      }
      return SendAck;
      break;
   default:
      break;
   }
   return Receive;
}

/**
 *
 */
Statemachine::Handler_ Statemachine::SendAck(Statemachine* me,
      const RadioEvent event)
{
   switch (event)
   {
   case RadioEvent::ENTER_STATE:
      static AckMsg ack;
      m_radio->Send((uint8_t*) &ack, sizeof(AckMsg));
      break;
   case RadioEvent::MSG_SEND_ERROR:
      return Receive;
      break;
   case RadioEvent::MSG_SENT_OK:
      return Receive;
      break;
   default:
      break;
   }
   return SendAck;
}

/**
 *
 */
void InjectEvent(RadioEvent event)
{
   BaseType_t rtn = xQueueSend(m_queue, &event, 100);
   if (rtn != pdPASS)
   {
      debug_if(1, "Queue Send failure!\n");
   }
}

/**
 *   CreateTheRadio()
 */
void CreateTheRadio(void)
{
   if (m_radio != nullptr)
   {
      debug_if(1, "ASSERT: radio exists, why call CreateTheRadio() ??\n");
      return;
   }

   //Create and Initialize instance of SX1276inAir
   m_radio = new SX1276inAir(OnRadioTxDone, OnRadioTxTimeout, OnRadioRxDone,
      OnRadioRxTimeout, OnRadioRxError, NULL, NULL);
   m_radio->SetBoardType(BOARD_INAIR9);
   wait_ms(10);

   //Wait for radio to be detected
   while (m_radio->Read(REG_VERSION) == 0x00)
   {
      wait_ms(400);
      debug_if(1, "Waiting on radio....\n", NULL);
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
   RadioEvent event = RadioEvent::MSG_SENT_OK;
   xQueueSendToBackFromISR(m_queue, &event, nullptr);
}

/**
 *
 */
void OnRadioRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
   static const SensorMsg REFERENCE_SENSOR_MSG;

   if (size != sizeof(SensorMsg))
   {
      debug_if(1, "Rx size is not expected, size = %d\n", size);
      m_radio->Sleep();
   }
   else
   {
      SensorMsg* pRxdMsg = (SensorMsg*) payload;
      m_radio->Sleep();

      /**
       * The Sensor hardware is not able
       * to measure its battery level, so this check
       * is ignoring the battery fields.
       */
      if ((0
            == memcmp(pRxdMsg->hdr_id, REFERENCE_SENSOR_MSG.hdr_id,
               sizeof(REFERENCE_SENSOR_MSG.hdr_id)))
            && (pRxdMsg->msg_id == REFERENCE_SENSOR_MSG.msg_id))
      {
         RadioEvent event = RadioEvent::SENSOR_MSG_RXD_OK;
         xQueueSendToBackFromISR(m_queue, &event, nullptr);
      }
      else
      {
         debug_if(1, "size matched, but rx msg contents do not pass\n", size);
      }
   }
}

/**
 *
 */
void OnRadioTxTimeout(void)
{
   debug_if(1, "OnTxTimeout\n");
   m_radio->Sleep();

   RadioEvent event = RadioEvent::MSG_SEND_ERROR;
   xQueueSendToBackFromISR(m_queue, &event, nullptr);
}

/**
 *
 */
void OnRadioRxTimeout(void)
{
   debug_if(1, "OnRxTimeout\n");
   m_radio->Sleep();
   RadioEvent event = RadioEvent::RX_TIMEOUT;
   xQueueSendToBackFromISR(m_queue, &event, nullptr);
}

/**
 *
 */
void OnRadioRxError(void)
{
   debug_if(1, "OnRxError\n");
   m_radio->Sleep();
   RadioEvent event = RadioEvent::RX_ERROR;
   xQueueSendToBackFromISR(m_queue, &event, nullptr);
}

} //namespace
