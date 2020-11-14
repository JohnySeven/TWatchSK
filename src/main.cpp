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

#define G_EVENT_VBUS_PLUGIN _BV(0)
#define G_EVENT_VBUS_REMOVE _BV(1)
#define G_EVENT_CHARGE_DONE _BV(2)

#define G_EVENT_WIFI_SCAN_START _BV(3)
#define G_EVENT_WIFI_SCAN_DONE _BV(4)
#define G_EVENT_WIFI_CONNECTED _BV(5)
#define G_EVENT_WIFI_BEGIN _BV(6)
#define G_EVENT_WIFI_OFF _BV(7)

enum
{
    Q_EVENT_WIFI_SCAN_DONE,
    Q_EVENT_WIFI_CONNECT,
    Q_EVENT_BMA_INT,
    Q_EVENT_AXP_INT,
};

#define DEFAULT_SCREEN_TIMEOUT 10 * 1000

#define WATCH_FLAG_SLEEP_MODE _BV(1)
#define WATCH_FLAG_SLEEP_EXIT _BV(2)
#define WATCH_FLAG_BMA_IRQ _BV(3)
#define WATCH_FLAG_AXP_IRQ _BV(4)

const char *TAG = "APP";

QueueHandle_t g_event_queue_handle = NULL;
EventGroupHandle_t g_event_group = NULL;
EventGroupHandle_t isr_group = NULL;
bool lenergy = false;
bool light_sleep = false;
TTGOClass *ttgo;
WifiManager *wifiManager;
SignalKSocket*sk_socket;

/*void setupNetwork()
{
    WiFi.mode(WIFI_STA);
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        xEventGroupClearBits(g_event_group, G_EVENT_WIFI_CONNECTED);
    }, WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);

    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        uint8_t data = Q_EVENT_WIFI_SCAN_DONE;
        xQueueSend(g_event_queue_handle, &data, portMAX_DELAY);
    }, WiFiEvent_t::SYSTEM_EVENT_SCAN_DONE);

    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        xEventGroupSetBits(g_event_group, G_EVENT_WIFI_CONNECTED);
    }, WiFiEvent_t::SYSTEM_EVENT_STA_CONNECTED);

    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        wifi_connect_status(true);
    }, WiFiEvent_t::SYSTEM_EVENT_STA_GOT_IP);
}*/

void low_energy()
{
    if (!lenergy)
    {
        xEventGroupSetBits(isr_group, WATCH_FLAG_SLEEP_MODE);
        ttgo->closeBL();
        ttgo->stopLvglTick();
        ttgo->bma->enableStepCountInterrupt(false);
        ttgo->displaySleep();
        lenergy = true;

        if (wifiManager->get_status() == Wifi_Off)
        {
            light_sleep = true;
            WiFi.mode(WIFI_OFF);
            setCpuFrequencyMhz(10);
            ESP_LOGI(TAG, "Entering light sleep mode");
            gpio_wakeup_enable((gpio_num_t)AXP202_INT, GPIO_INTR_LOW_LEVEL);
            gpio_wakeup_enable((gpio_num_t)BMA423_INT1, GPIO_INTR_HIGH_LEVEL);
            esp_sleep_enable_gpio_wakeup();
            esp_light_sleep_start();
        }
        else
        {
            light_sleep = false;
            ESP_LOGI(TAG, "WiFi is enabled, no LIGHT sleep is enabled.");

            EventBits_t bits = xEventGroupGetBits(isr_group);
            while (!(bits & WATCH_FLAG_SLEEP_EXIT))
            {
                delay(250);
                ESP_LOGI(TAG, "Sleeping at %d MHz...", getCpuFrequencyMhz());
                bits = xEventGroupGetBits(isr_group);
            }

            ESP_LOGI(TAG,"Wakeup request from sleep %d", bits);
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

        lenergy = false;
        ttgo->startLvglTick();
        ttgo->displayWakeup();
        ttgo->rtc->syncToSystem();
        updateStepCounter(ttgo->bma->getCounter());
        updateBatteryLevel();
        updateBatteryIcon(LV_ICON_CALCULATION);
        lv_disp_trig_activity(NULL);
        ttgo->openBL();
        ttgo->bl->adjust(150);
        ttgo->bma->enableStepCountInterrupt();
    }
}

void setup()
{
#if !CONFIG_PM_ENABLE
#error "CONFIG_PM_ENABLE missing"
#endif
#if !CONFIG_FREERTOS_USE_TICKLESS_IDLE
#error "CONFIG_FREERTOS_USE_TICKLESS_IDLE missing"
#endif
    //Create a program that allows the required message objects and group flags
    g_event_queue_handle = xQueueCreate(20, sizeof(uint8_t));
    g_event_group = xEventGroupCreate();
    isr_group = xEventGroupCreate();
    setCpuFrequencyMhz(80);
    ttgo = TTGOClass::getWatch();
    if (!SPIFFS.begin(true))
    {
        ESP_LOGE(TAG, "Failed to initialize SPIFFS!");
    }

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

    // Enable BMA423 interrupt ，
    // The default interrupt configuration,
    // you need to set the acceleration parameters, please refer to the BMA423_Accel example
    ttgo->bma->attachInterrupt();

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
                uint8_t data = Q_EVENT_BMA_INT;
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
                uint8_t data = Q_EVENT_AXP_INT;
                xQueueSendFromISR(g_event_queue_handle, &data, &xHigherPriorityTaskWoken);
            }
            if (xHigherPriorityTaskWoken)
            {
                portYIELD_FROM_ISR();
            }
        },
        FALLING);

    ESP_LOGI(TAG, "Sensors initialized!");

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    //Check if the RTC clock matches, if not, use compile time
    //ttgo->rtc->check();

    //Synchronize time to system time
    ttgo->rtc->syncToSystem();
    //Setting up the network
    wifiManager = new WifiManager();
    //Setting up websocket
    sk_socket = new SignalKSocket();
    //Execute your own GUI interface
    setupGui(wifiManager);

    ESP_LOGI(TAG, "Wifi states (%d,%d,%d, %d)", (int)WifiState_t::Wifi_Off, (int)WifiState_t::Wifi_Connecting, (int)WifiState_t::Wifi_Connected, (int)WifiState_t::Wifi_Disconnected);

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
            //TODO: Only accept axp power pek key short press
            xEventGroupClearBits(isr_group, WATCH_FLAG_AXP_IRQ);
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
        case Q_EVENT_BMA_INT:
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
        case Q_EVENT_AXP_INT:
            ttgo->power->readIRQ();
            if (ttgo->power->isVbusPlugInIRQ())
            {
                updateBatteryIcon(LV_ICON_CHARGE);
            }
            if (ttgo->power->isVbusRemoveIRQ())
            {
                updateBatteryIcon(LV_ICON_CALCULATION);
            }
            if (ttgo->power->isChargingDoneIRQ())
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
        case Q_EVENT_WIFI_SCAN_DONE:
        {
            int16_t len = WiFi.scanComplete();
            for (int i = 0; i < len; ++i)
            {
                wifi_list_add(WiFi.SSID(i).c_str());
            }
            break;
        }
        default:
            break;
        }
    }

    if (!lenergy)
    {
        if (lv_disp_get_inactive_time(NULL) < DEFAULT_SCREEN_TIMEOUT)
        {
            lv_task_handler();
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
    // initialize arduino library before we start the tasks
    initArduino();
    setup();
    xTaskCreate(&arduinoTask, "app_task", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
}