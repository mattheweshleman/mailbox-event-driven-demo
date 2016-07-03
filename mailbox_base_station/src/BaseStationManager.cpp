/*
 * BaseStationManager.cpp
 *
 *  Created on: May 29, 2016
 *      Author: Matthew Eshleman
 */

#include "BaseStationManager.h"
#include "RadioManager.h"
#include <cstdint>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "mbed.h"
#include "myDebug.h"
#include "core_cmFunc.h"
#include "im4oled.h"
#include "myDebug.h"
#include "Adafruit_SSD1306.h"

namespace BaseStationManager
{
static void OnRadioSensorMsgReceivedOk();

enum class ManagerEvent : char
{
   ENTER_STATE,
   EXIT_STATE,
   SENSOR_MSG_RXD_OK,
   CLEAR_BUTTON_PRESSED,
   HEARTBEAT,
   EXCEED_DAYS_WITHOUT_SENSOR_MSG
};

static void InjectEvent(ManagerEvent event);
static void BaseStationManagerTask(void *);
static void ButtonCheckTimerCallback(TimerHandle_t xTimer);
static void HeartBeatTimerCallback(TimerHandle_t xTimer);
static void CheckTooManyDaysWithoutSensor();
static void OledDisplayMailMessage();
static void OledDisplayBatteryMessage();

//simple flat state machine defining base station
//overall high level behavior, primarily focused
//on UI behavior.
class Statemachine
{
public:
   Statemachine() :
         mCurrent(nullptr)
   {
   }

   virtual ~Statemachine()
   {
   }

   void Init();
   void ProcessEvent(const ManagerEvent event);

private:
   /*
    * Check this out... http://www.gotw.ca/gotw/057.htm
    *   For background on the below.
    */
   struct SmHandler_;
   typedef SmHandler_ (*Handler)(Statemachine*, const ManagerEvent);
   struct SmHandler_
   {
      SmHandler_(Handler pp) :
            p(pp)
      {
      }
      operator Handler()
      {
         return p;
      }
      Handler p;
   };

   static SmHandler_ Idle(Statemachine* me, const ManagerEvent event);
   static SmHandler_ DisplayMailReceived(Statemachine* me,
         const ManagerEvent event);
   static SmHandler_ DisplayCheckSensorBattery(Statemachine* me,
         const ManagerEvent event);

   Handler mCurrent;
};

static QueueHandle_t m_queue = nullptr;
static TimerHandle_t m_button_check_timer = 0;
static TimerHandle_t m_heartbeat_timer = 0;
static DigitalOut led(LED1);
static Im4Oled im4OLED(PC_1, PC_12, PC_2, PC_10);  //OK, Star, Up, Down
static uint32_t m_seconds_without_mail_sensor_msg = 0;

// Class and Variables for OLED ///////////////////////////////////////////////
// An I2C sub-class that provides a constructed default, for OLED
// Use Baud = 200,000.
class I2CPreInit: public I2C
{
public:
   I2CPreInit(PinName sda, PinName scl) :
         I2C(sda, scl)
   {
      frequency(200000);
      //start();  //Does NOT work when this is defined!
   }
};

static I2CPreInit gI2C(PB_9, PB_8);
static Adafruit_SSD1306_I2c* gOled = NULL;

/**
 *
 */
void Init()
{
   //Startup Delay
   wait_ms(100);

   //Only create Adafruit_SSD1306_I2c object here - after startup delay!
   gOled = new Adafruit_SSD1306_I2c(gI2C, PA_8, 0x78, 64);

   xTaskCreate(BaseStationManagerTask, "StationManager",
      configMINIMAL_STACK_SIZE*3, nullptr, tskIDLE_PRIORITY, nullptr);
}

/**
 *
 */
void BaseStationManagerTask(void*)
{
   debug_if(1, "Starting Base Station Manager Task\n");

   m_queue = xQueueCreate(5, sizeof(ManagerEvent));
   if (m_queue == nullptr)
   {
      return;
   }

   Statemachine statemachine;
   statemachine.Init();

   //now that our high level statemachine and queue
   //are ready, Init the Radio thread/statemachine
   RadioManager::Init(OnRadioSensorMsgReceivedOk);

   //Init the button monitor timer
   m_button_check_timer = xTimerCreate("BTN_50ms", pdMS_TO_TICKS(50), pdTRUE,
      nullptr, ButtonCheckTimerCallback);
   if (m_button_check_timer == NULL)
   {
      debug_if(1, "ERROR: Unable to create button check timer!\n");
   }
   xTimerStart(m_button_check_timer, 100);

   //Init the heart beat timer
   m_heartbeat_timer = xTimerCreate("Heart", pdMS_TO_TICKS(1000), pdTRUE,
      nullptr, HeartBeatTimerCallback);
   if (m_heartbeat_timer == NULL)
   {
      debug_if(1, "ERROR: Unable to create heart timer!\n");
   }
   xTimerStart(m_heartbeat_timer, 100);

   ManagerEvent event;
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
void OnRadioSensorMsgReceivedOk()
{
   InjectEvent(ManagerEvent::SENSOR_MSG_RXD_OK);
}

/**
 *
 */
void InjectEvent(ManagerEvent event)
{
   BaseType_t rtn = xQueueSend(m_queue, &event, 100);
   if (rtn != pdPASS)
   {
      debug_if(1, "Queue Send failure!\n");
   }
}

/**
 *
 */
void Statemachine::Init()
{
   mCurrent = Idle;
   ProcessEvent(ManagerEvent::ENTER_STATE);
}

/**
 *
 */
void Statemachine::ProcessEvent(const ManagerEvent event)
{
   if (mCurrent != nullptr)
   {
      Handler new_handler = mCurrent(this, event);
      if (new_handler != mCurrent)
      {
         mCurrent(this, ManagerEvent::EXIT_STATE);
         mCurrent = new_handler;
         mCurrent(this, ManagerEvent::ENTER_STATE);
      }
   }
}

/**
 *
 */
Statemachine::SmHandler_ Statemachine::Idle(Statemachine* me,
      const ManagerEvent event)
{
   static bool heart = false;

   switch (event)
   {
   case ManagerEvent::ENTER_STATE:
      led = 0;
      gOled->clearDisplay();
      gOled->display();
      break;
   case ManagerEvent::SENSOR_MSG_RXD_OK:
      return DisplayMailReceived;
      break;
   case ManagerEvent::EXCEED_DAYS_WITHOUT_SENSOR_MSG:
      return DisplayCheckSensorBattery;
      break;
   case ManagerEvent::HEARTBEAT:
      m_seconds_without_mail_sensor_msg++;

      gOled->clearDisplay();
      gOled->setTextSize(1);
      if (heart)
      {
         gOled->setTextCursor(0, 0);
         gOled->printf("*");
      }

      /* Activating the LED
       * before the OLED display,
       * as it results in a more pleasing
       * synchronized blink between
       * the LED and the OLED
       */
      led = (int) heart;
      gOled->display();
      heart = !heart;

      CheckTooManyDaysWithoutSensor();
      break;
   default:
      break;
   }
   return Idle;
}

/**
 *
 */
Statemachine::SmHandler_ Statemachine::DisplayMailReceived(Statemachine* me,
      const ManagerEvent event)
{
   switch (event)
   {
   case ManagerEvent::ENTER_STATE:
      m_seconds_without_mail_sensor_msg = 0;
      led = 1;
      OledDisplayMailMessage();
      break;
   case ManagerEvent::EXIT_STATE:
      led = 0;
      break;
   case ManagerEvent::CLEAR_BUTTON_PRESSED:
      return Idle;
      break;
   default:
      break;
   }
   return DisplayMailReceived;
}

/**
 *
 */
Statemachine::SmHandler_ Statemachine::DisplayCheckSensorBattery(
      Statemachine* me, const ManagerEvent event)
{
   switch (event)
   {
   case ManagerEvent::ENTER_STATE:
      m_seconds_without_mail_sensor_msg = 0;
      led = 1;
      OledDisplayBatteryMessage();
      break;
   case ManagerEvent::SENSOR_MSG_RXD_OK:
      return DisplayMailReceived;
      break;
   case ManagerEvent::CLEAR_BUTTON_PRESSED:
      return Idle;
      break;
   default:
      break;
   }
   return DisplayCheckSensorBattery;
}

/**
 *
 */
void ButtonCheckTimerCallback(TimerHandle_t)
{
   //the im4OLED object latches
   //button events, so we just need
   //to check here from time to time.
   if (im4OLED.getStarBtnFalling() || im4OLED.getOkBtnFalling())
   {
      InjectEvent(ManagerEvent::CLEAR_BUTTON_PRESSED);
   }
}

/**
 *
 */
void HeartBeatTimerCallback(TimerHandle_t)
{
   InjectEvent(ManagerEvent::HEARTBEAT);
}

/**
 *
 */
void CheckTooManyDaysWithoutSensor()
{
   static constexpr uint32_t LIMIT_WITHOUT_MAIL_SENSOR_MSG_SECS = 3 * 24 * 60 * 60; //3 days

   if (m_seconds_without_mail_sensor_msg > LIMIT_WITHOUT_MAIL_SENSOR_MSG_SECS)
   {
      InjectEvent(ManagerEvent::EXCEED_DAYS_WITHOUT_SENSOR_MSG);
   }
}

/**
 *
 */
void OledDisplayMailMessage()
{
   gOled->clearDisplay();
   gOled->display();
   gOled->setTextSize(2);
   gOled->setTextCursor(0, 16);
   gOled->printf("!--------!");
   gOled->printf("!  MAIL  !");
   gOled->printf("!--------!");
   gOled->display();
}

/**
 *
 */
void OledDisplayBatteryMessage()
{
   gOled->clearDisplay();
   gOled->display();
   gOled->setTextSize(2);
   gOled->setTextCursor(0, 16);
   gOled->printf("----------");
   gOled->printf("-  BATT  -");
   gOled->printf("----------");
   gOled->display();
}

} //namespace
