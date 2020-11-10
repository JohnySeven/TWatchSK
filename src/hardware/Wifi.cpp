#include "Wifi.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

const char*WIFI_TAG = "WIFI";

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    auto manager = (WifiManager*)arg;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if(manager->get_enabled())
        {
            esp_wifi_connect();
        }
        manager->set_connected(false);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(WIFI_TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        char buff[24];
        sprintf(buff, IPSTR, IP2STR(&event->ip_info.ip));
        manager->set_ip(String(buff));
        manager->set_connected(true);
    }
}

static void wifi_enable(const char *ssid, const char *password, WifiManager*arg)
{
    tcpip_adapter_init();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, arg));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, arg));

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config_t));
    strcpy(reinterpret_cast<char*>(wifi_config.sta.ssid), ssid);
    strcpy(reinterpret_cast<char*>(wifi_config.sta.password), password);
    wifi_config.sta.listen_interval = 9;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(WIFI_TAG, "esp_wifi_set_ps().");
    esp_wifi_set_ps(WIFI_PS_MAX_MODEM);

}

static void disable_wifi()
{
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();
}

void WifiManager::on()
{
    if (!ssid.isEmpty())
    {
        enabled = true;
        wifi_enable(ssid.c_str(), password.c_str(), this);
    }
    ESP_LOGI(WIFI_TAG, "WiFi has been enabled.");

    notify("enabled", &enabled);
}

void WifiManager::off()
{
    enabled = false;
    disable_wifi();
    ESP_LOGI(WIFI_TAG, "WiFi has been disabled.");
    notify("enabled", &enabled);
}

void WifiManager::get_config(const JsonObject&json)
{
    enabled = json["enabled"].as<bool>();
    this->setup(json["ssid"].as<String>(), json["password"].as<String>());
}

void WifiManager::set_config(const JsonObject&json)
{
    json["enabled"] = enabled;
    json["ssid"] = ssid;
    json["password"] = password;
}