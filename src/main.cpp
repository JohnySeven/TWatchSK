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
#include "hardware/hardware.h"
#include <functional>
using std::placeholders::_1;
using std::placeholders::_2;
#include "system/async_dispatcher.h"
#include "sounds/sound_player.h"
#include "sounds/beep.h"
#include "ui/localization.h"
#include "imgs/sk_image_low.h"

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

void set_splash_screen_status(TTGOClass* watch, int percent, char*message = NULL)
{
    auto y = 160;
    if(percent > 100)
    {
        percent = 100;
    }
    watch->tft->fillRect(21, y, percent * 2, 28, watch->tft->color565(0, 51, 153));
    watch->tft->drawRect(19, y, 202, 30, TFT_WHITE);
    if(message != NULL)
    {
        watch->tft->fillRect(0, TFT_HEIGHT - 20, TFT_WIDTH, 20, TFT_BLACK);
        watch->tft->setTextColor(TFT_WHITE);
        watch->tft->setTextFont(2);
        watch->tft->setCursor(0, TFT_HEIGHT - 20);
        watch->tft->println(message);
    }

    ESP_LOGI(TAG, "Loader status=%d%%", percent);
}

void init_splash_screen(TTGOClass* watch)
{
    watch->tft->setTextColor(TFT_WHITE);
    watch->tft->setTextFont(2);
    watch->tft->setCursor(0, TFT_HEIGHT - 20);
    watch->tft->println("Initializing...");
    watch->tft->setCursor((TFT_WIDTH / 2) - 30, 130);
    watch->tft->println("TWatchSK");
    watch->tft->setCursor(0, 0);
    watch->tft->print(LOC_WATCH_VERSION);
    watch->tft->drawXBitmap((TFT_WIDTH / 2) - (skIcon_width / 2), 60, skIcon_bits, skIcon_width, skIcon_height, watch->tft->color565(0, 51, 153));

    set_splash_screen_status(watch, 10);
}


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
    initialize_events();
    twatchsk::initialize_async();
    //Initialize TWatch
    ttgo->begin();
#ifdef LILYGO_WATCH_2020_V2
    ttgo->power->setLDO2Voltage(3300);
    ttgo->power->setLDO3Voltage(3300);
    ttgo->power->setPowerOutPut(AXP202_LDO2, true);
    ttgo->power->setPowerOutPut(AXP202_LDO3, true);
#endif
    ttgo->bl->on();
    init_splash_screen(ttgo);
    ESP_LOGI(TAG, "Initializing SPIFFS...");
    set_splash_screen_status(ttgo, 10, LOC_STARTUP_SPIFFS);
    if (!SPIFFS.begin(true))
    {
        ESP_LOGE(TAG, "Failed to initialize SPIFFS!");
    }
    set_splash_screen_status(ttgo, 30, LOC_STARTUP_HW_GUI);

    //initalize Hardware (power management, sensors and interupts)
    hardware = new Hardware();
    hardware->initialize(ttgo);
    //Initialize lvgl
     if(ttgo->lvgl_begin())
    {
        ESP_LOGI(TAG, "LVGL initialized!");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to initialize LVGL!");
        return;
    }

    hardware->initialize_touch();
    
    ESP_LOGI(TAG, "Touch initialized!");
    //Synchronize time to system time
    ttgo->rtc->syncToSystem();
    ESP_LOGI(TAG, "Time synced with RTC!");
    set_splash_screen_status(ttgo, 40, LOC_STARTUP_NETWORKING);
    //Setting up the network
    wifiManager = new WifiManager();
    //Setting up websocket
    sk_socket = new SignalKSocket(wifiManager);
    sk_socket->add_subscription("notifications.*", 1000, true);
    sk_socket->add_subscription("environment.mode", 5000, false);
    //Attach power management events to sk_socket
    hardware->attach_power_callback(std::bind(&SignalKSocket::handle_power_event, sk_socket, _1, _2));
    set_splash_screen_status(ttgo, 60);
    //Intialize watch GUI
    gui = new Gui();
    //Setup GUI
    gui->setup_gui(wifiManager, sk_socket, hardware);
    //set SK socket pointer to device name in gui
    sk_socket->set_device_name(gui->get_watch_name());
    //Clear lvgl counter
    lv_disp_trig_activity(NULL);
    //When the initialization is complete, turn on the backlight
    ttgo->bl->adjust(gui->get_adjusted_display_brightness());
    hardware->get_player()->play_raw_from_const("beep", beep_sound, beep_sound_len);
    set_splash_screen_status(ttgo, 90);

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

    ESP_LOGI(TAG, "TWatch SK app initialized, Build date " __DATE__ ", GIT revision ");
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