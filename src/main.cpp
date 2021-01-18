/* Blink Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "freertos/timers.h"
#include "freertos/queue.h"
#include <driver/gpio.h>
#include "sdkconfig.h"
#include <Arduino.h>
#include "config.h"
#include <soc/rtc.h>
#include "esp_wifi.h"
#include "esp_sleep.h"
#include <WiFi.h>
#include "gui.h"
#include "SPIFFS.h"
#include "hardware/Wifi.h"
#include "networking/signalk_socket.h"
#include "esp_int_wdt.h"
#include "esp_pm.h"
#include "system/events.h"
#include "system/system_data.h"

const char *TAG = "APP";
bool lenergy = false;
bool light_sleep = false;
int screen_timeout = 10;
TTGOClass *ttgo;
WifiManager *wifiManager;
SignalKSocket*sk_socket;
SystemData*system_data;
EventGroupHandle_t isr_group = NULL;

#define WATCH_FLAG_SLEEP_MODE _BV(1)
#define WATCH_FLAG_SLEEP_EXIT _BV(2)
#define WATCH_FLAG_BMA_IRQ _BV(3)
#define WATCH_FLAG_AXP_IRQ _BV(4)
#define WATCH_FLAT_TOUCH_IRQ _BV(5)

void low_energy()
{
    if (!lenergy)
    {
        xEventGroupSetBits(isr_group, WATCH_FLAG_SLEEP_MODE);
        set_low_power(true);
        sk_socket->update_subscriptions();
        ttgo->closeBL();
        ttgo->stopLvglTick();
        ttgo->bma->enableStepCountInterrupt(false);
        ttgo->displaySleep();
        lenergy = true;

        if (wifiManager->get_status() == Wifi_Off)
        {
            light_sleep = true;
            setCpuFrequencyMhz(10);
            ESP_LOGI(TAG, "Entering light sleep mode");
            gpio_wakeup_enable((gpio_num_t)AXP202_INT, GPIO_INTR_LOW_LEVEL);
            gpio_wakeup_enable((gpio_num_t)BMA423_INT1, GPIO_INTR_HIGH_LEVEL);
            gpio_wakeup_enable((gpio_num_t)TOUCH_INT, GPIO_INTR_HIGH_LEVEL);
            esp_sleep_enable_gpio_wakeup();
            esp_light_sleep_start();
        }
        else
        {
            light_sleep = false;
            ESP_LOGI(TAG, "WiFi is enabled, will not enter light sleep.");

            EventBits_t isr_bits = xEventGroupGetBits(isr_group);
            EventBits_t app_bits = xEventGroupGetBits(g_app_state);

            while (!(isr_bits & WATCH_FLAG_SLEEP_EXIT) && !(app_bits & G_APP_STATE_WAKE_UP))
            {
                delay(500);
                isr_bits = xEventGroupGetBits(isr_group);
                app_bits = xEventGroupGetBits(g_app_state);
            }

            xEventGroupClearBits(g_app_state, G_APP_STATE_WAKE_UP);
            if(app_bits & G_APP_STATE_WAKE_UP)
            {
                xEventGroupSetBits(isr_group, WATCH_FLAG_SLEEP_EXIT);
            }

            ESP_LOGI(TAG,"Wakeup request from sleep. System isr=%d,app=%d", isr_bits, app_bits);
        }
    }
    else
    {
        if(light_sleep)
        {
            setCpuFrequencyMhz(80);
            light_sleep = false;
            ESP_LOGI(TAG, "Left light sleep with MCU freq=%d Mhz", getCpuFrequencyMhz());
        }

        set_low_power(false);
        lenergy = false;
        ttgo->startLvglTick();
        ttgo->displayWakeup();
        ttgo->touch->setPowerMode(PowerMode_t::FOCALTECH_PMODE_ACTIVE);
        ttgo->rtc->syncToSystem();
        updateStepCounter(ttgo->bma->getCounter());
        updateBatteryLevel();
        updateBatteryIcon(LV_ICON_CALCULATION);
        lv_disp_trig_activity(NULL);
        sk_socket->update_subscriptions();
        ttgo->openBL();
        ttgo->bl->adjust(125);
        ttgo->bma->enableStepCountInterrupt();
    }
}

#if LV_USE_LOG
void lv_log_cb(lv_log_level_t level, const char * file, uint32_t line, const char * func, const char * dsc)
{
  /*Send the logs via serial port*/

  if(level == LV_LOG_LEVEL_ERROR)
  {
      ESP_LOGE("LVGL", "%s in %s:%d", dsc, file, line);
  }
  else if(level == LV_LOG_LEVEL_INFO)
  {
      ESP_LOGI("LVGL", "%s in %s:%d", dsc, file, line);
  }
  else if(level == LV_LOG_LEVEL_WARN)
  {
      ESP_LOGW("LVGL", "%s in %s:%d", dsc, file, line);
  }
  else
  {
      ESP_LOGI("LVGL", "%s in %s:%d", dsc, file, line);
  }
}
#endif

void setup()
{
#if !CONFIG_PM_ENABLE
#error "CONFIG_PM_ENABLE missing"
#endif
#if !CONFIG_FREERTOS_USE_TICKLESS_IDLE
#error "CONFIG_FREERTOS_USE_TICKLESS_IDLE missing"
#endif
#if LV_USE_LOG
    lv_log_register_print_cb(lv_log_cb);
#endif
    ttgo = TTGOClass::getWatch();
    if (!SPIFFS.begin(true))
    {
        ESP_LOGE(TAG, "Failed to initialize SPIFFS!");
    }
    
    isr_group = xEventGroupCreate();
    initialize_events();

    //Initialize TWatch
    ttgo->begin();

    // Turn on the IRQ used
    ttgo->power->adc1Enable(AXP202_BATT_VOL_ADC1 | AXP202_BATT_CUR_ADC1 | AXP202_VBUS_VOL_ADC1 | AXP202_VBUS_CUR_ADC1, AXP202_ON);
    ttgo->power->enableIRQ(AXP202_VBUS_REMOVED_IRQ | AXP202_VBUS_CONNECT_IRQ | AXP202_CHARGING_FINISHED_IRQ, AXP202_ON);
    ttgo->power->clearIRQ();
    ttgo->power->setChgLEDMode(axp_chgled_mode_t::AXP20X_LED_OFF);

    // Turn off unused power
    ttgo->power->setPowerOutPut(AXP202_EXTEN, AXP202_OFF);
    ttgo->power->setPowerOutPut(AXP202_DCDC2, AXP202_OFF);
    ttgo->power->setPowerOutPut(AXP202_LDO3, AXP202_OFF);
    ttgo->power->setPowerOutPut(AXP202_LDO4, AXP202_OFF);

    ESP_LOGI(TAG, "Watch power initialized!");

    //Initialize lvgl
    ttgo->lvgl_begin();

    ESP_LOGI(TAG, "LVGL initialized!");


    // Enable BMA423 interrupt ，
    // The default interrupt configuration,
    // you need to set the acceleration parameters, please refer to the BMA423_Accel example
    ttgo->bma->attachInterrupt();
    ttgo->bma->enableTiltInterrupt(false);
    /*pinMode(TOUCH_INT, INPUT);
    attachInterrupt(
        TOUCH_INT, [] {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            EventBits_t bits = xEventGroupGetBitsFromISR(isr_group);
            if (bits & WATCH_FLAG_SLEEP_MODE)
            {
                //! For quick wake up, use the group flag
                xEventGroupSetBitsFromISR(isr_group, WATCH_FLAG_SLEEP_EXIT | WATCH_FLAT_TOUCH_IRQ, &xHigherPriorityTaskWoken);
            }

            if (xHigherPriorityTaskWoken)
            {
                portYIELD_FROM_ISR();
            }
        },
        RISING);*/
    //Connection interrupted to the specified pin
    pinMode(BMA423_INT1, INPUT);
    attachInterrupt(
        BMA423_INT1, [] {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            EventBits_t bits = xEventGroupGetBitsFromISR(isr_group);
            if (bits & WATCH_FLAG_SLEEP_MODE)
            {
                //! For quick wake up, use the group flag
                xEventGroupSetBitsFromISR(isr_group, WATCH_FLAG_SLEEP_EXIT | WATCH_FLAG_BMA_IRQ, &xHigherPriorityTaskWoken);
            }
            else
            {
                uint8_t data = ApplicationEvents_T::Q_EVENT_BMA_INT;
                xQueueSendFromISR(g_event_queue_handle, &data, &xHigherPriorityTaskWoken);
            }

            if (xHigherPriorityTaskWoken)
            {
                portYIELD_FROM_ISR();
            }
        },
        RISING);
    // Connection interrupted to the specified pin
    pinMode(AXP202_INT, INPUT);
    attachInterrupt(
        AXP202_INT, [] {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            EventBits_t bits = xEventGroupGetBitsFromISR(isr_group);
            if (bits & WATCH_FLAG_SLEEP_MODE)
            {
                //! For quick wake up, use the group flag
                xEventGroupSetBitsFromISR(isr_group, WATCH_FLAG_SLEEP_EXIT | WATCH_FLAG_AXP_IRQ, &xHigherPriorityTaskWoken);
            }
            else
            {
                uint8_t data = ApplicationEvents_T::Q_EVENT_AXP_INT;
                xQueueSendFromISR(g_event_queue_handle, &data, &xHigherPriorityTaskWoken);
            }
            if (xHigherPriorityTaskWoken)
            {
                portYIELD_FROM_ISR();
            }
        },
        FALLING);

    ESP_LOGI(TAG, "Sensors initialized!");
    //Check if the RTC clock matches, if not, use compile time
    //ttgo->rtc->check();

    //Synchronize time to system time
    ttgo->rtc->syncToSystem();
    //Setting up system_data
    system_data = new SystemData();
    //Setting up the network
    wifiManager = new WifiManager();
    //Setting up websocket
    sk_socket = new SignalKSocket(wifiManager);
    sk_socket->add_subscription("notifications.*", 1000, true);
    sk_socket->add_subscription("environment.mode", 5000, false);
    //Execute your own GUI interface
    setupGui(wifiManager, sk_socket, system_data);
    //Clear lvgl counter
    lv_disp_trig_activity(NULL);
    //When the initialization is complete, turn on the backlight
    ttgo->openBL();

#if CONFIG_PM_ENABLE
    // Configure dynamic frequency scaling:
    // maximum and minimum frequencies are set in sdkconfig,
    // automatic light sleep is enabled if tickless idle support is enabled.
    esp_pm_config_esp32_t pm_config = {
        .max_freq_mhz = 80,
        .min_freq_mhz = 10,
#if CONFIG_FREERTOS_USE_TICKLESS_IDLE
        .light_sleep_enable = true
#endif
    };
    ESP_ERROR_CHECK(esp_pm_configure(&pm_config));
#endif // CONFIG_PM_ENABLE

    ESP_LOGI(TAG, "TWatch SK app initialized, Build date " __DATE__ ", GIT revision " GIT_REV);
}
void loop()
{
    bool rlst;
    uint8_t data;
    //! Fast response wake-up interrupt
    EventBits_t bits = xEventGroupGetBits(isr_group);
    if (bits & WATCH_FLAG_SLEEP_EXIT)
    {
        low_energy();

        if (bits & WATCH_FLAG_BMA_IRQ)
        {
            do
            {
                rlst = ttgo->bma->readInterrupt();
            } while (!rlst);
            xEventGroupClearBits(isr_group, WATCH_FLAG_BMA_IRQ);
        }
        if (bits & WATCH_FLAG_AXP_IRQ)
        {
            ttgo->power->readIRQ();
            ttgo->power->clearIRQ();

            xEventGroupClearBits(isr_group, WATCH_FLAG_AXP_IRQ);
        }
        if (bits & WATCH_FLAT_TOUCH_IRQ)
        {
            xEventGroupClearBits(isr_group, WATCH_FLAT_TOUCH_IRQ);
            ESP_LOGD(TAG, "Touch interupt!");
        }

        xEventGroupClearBits(isr_group, WATCH_FLAG_SLEEP_EXIT);
        xEventGroupClearBits(isr_group, WATCH_FLAG_SLEEP_MODE);
    }
    if ((bits & WATCH_FLAG_SLEEP_MODE))
    {
        //! No event processing after entering the information screen
        return;
    }

    //! Normal polling
    if (xQueueReceive(g_event_queue_handle, &data, 5 / portTICK_RATE_MS) == pdPASS)
    {
        switch (data)
        {
        case ApplicationEvents_T::Q_EVENT_BMA_INT:
            do
            {
                rlst = ttgo->bma->readInterrupt();
            } while (!rlst);

            //! setp counter
            if (ttgo->bma->isStepCounter())
            {
                updateStepCounter(ttgo->bma->getCounter());
            }
            break;
        case ApplicationEvents_T::Q_EVENT_AXP_INT:
            ttgo->power->readIRQ();
            if (ttgo->power->isVbusPlugInIRQ())
            {
                updateBatteryIcon(LV_ICON_CHARGE);
            }
            if (ttgo->power->isVbusRemoveIRQ() || ttgo->power->isChargingDoneIRQ())
            {
                updateBatteryIcon(LV_ICON_CALCULATION);
            }
            
            if (ttgo->power->isPEKShortPressIRQ())
            {
                ttgo->power->clearIRQ();
                low_energy();
                return;
            }
            ttgo->power->clearIRQ();
            break;
        default:
            break;
        }
    }

    if (!lenergy)
    {
        if (lv_disp_get_inactive_time(NULL) < (system_data->get_screen_timeout() * 1000))
        {
            auto sleep = lv_task_handler();
            if(sleep > 250)
            {
                sleep = 250;
            }

            delay(sleep);
        }
        else
        {
            low_energy();
        }
    }
    else
    {
        ESP_LOGI(TAG, "Low power");
        delay(250);
    }
}

void arduinoTask(void *pvParameter)
{
    while (1)
    {
        loop();
    }
}

void app_main()
{
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("WEBSOCKET_CLIENT", ESP_LOG_DEBUG);
    esp_log_level_set("TRANS_TCP", ESP_LOG_DEBUG);
    // initialize arduino library before we start the tasks
    initArduino();
    setup();
    xTaskCreate(&arduinoTask, "app_task", configMINIMAL_STACK_SIZE * 2, NULL, 5, NULL);
}