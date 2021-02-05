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
#include "hardware/Hardware.h"
#include <functional>
using std::placeholders::_1;
using std::placeholders::_2;
#include "system/async_dispatcher.h"

const char *TAG = "APP";
TTGOClass *ttgo;
WifiManager *wifiManager;
SignalKSocket *sk_socket;
Hardware *hardware;
Gui *gui;

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
    initialize_events();
    twatchsk::initialize_async();
    //Initialize TWatch
    ttgo->begin();
    //initalize Hardware (power management, sensors and interupts)
    hardware = new Hardware();
    hardware->initialize(ttgo);
    //Initialize lvgl
    ttgo->lvgl_begin();

    ESP_LOGI(TAG, "LVGL initialized!");
    //Synchronize time to system time
    ttgo->rtc->syncToSystem();
    //Setting up the network
    wifiManager = new WifiManager();
    //Setting up websocket
    sk_socket = new SignalKSocket(wifiManager);
    sk_socket->add_subscription("notifications.*", 1000, true);
    sk_socket->add_subscription("environment.mode", 5000, false);
    //Attach power management events to sk_socket
    hardware->attach_power_callback(std::bind(&SignalKSocket::handle_power_event, sk_socket, _1, _2));
    //Intialize watch GUI
    gui = new Gui();
    //Setup GUI
    gui->setup_gui(wifiManager, sk_socket, hardware);
    //Clear lvgl counter
    lv_disp_trig_activity(NULL);
    //When the initialization is complete, turn on the backlight
    ttgo->openBL();
    ttgo->bl->adjust(gui->get_adjusted_display_brightness());

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
   hardware->loop();
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