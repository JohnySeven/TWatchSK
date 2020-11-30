#include "Wifi.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

const char *WIFI_TAG = "WIFI";
static bool scan_done = false;
static bool scan_running = false;

void WifiManager::wifi_event_handler(void *arg, esp_event_base_t event_base,
                                     int32_t event_id, void *event_data)
{
    auto manager = (WifiManager *)arg;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        manager->update_status(Wifi_Disconnected);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(WIFI_TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        char buff[24];
        sprintf(buff, IPSTR, IP2STR(&event->ip_info.ip));
        manager->set_ip(String(buff));
        manager->update_status(Wifi_Connected);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE)
    {
        WifiManager *manager = (WifiManager *)event_data;
        uint16_t number = WIFI_AP_LIST_MAX_SIZE;
        wifi_ap_record_t ap_info[WIFI_AP_LIST_MAX_SIZE];
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&manager->ap_count));
        ESP_LOGI(WIFI_TAG, "Total APs found = %u", manager->ap_count);
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
        for (int i = 0; (i < number) && (i < manager->ap_count); i++)
        {
            ESP_LOGI(WIFI_TAG, "SSID \t%s\tRSSI %d", ap_info[i].ssid, ap_info[i].rssi);
        }

        memcpy(manager->ap_info, ap_info, WIFI_AP_LIST_MAX_SIZE * sizeof(wifi_ap_record_t));

        scan_running = false;
        scan_done = true;
    }
}

void WifiManager::initialize()
{
    ESP_LOGI(WIFI_TAG, "Initializing wifi...");
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiManager::wifi_event_handler, this));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiManager::wifi_event_handler, this));
    initialized = true;
}

static void wifi_enable(const char *ssid, const char *password)
{
    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config_t));
    strcpy(reinterpret_cast<char *>(wifi_config.sta.ssid), ssid);
    strcpy(reinterpret_cast<char *>(wifi_config.sta.password), password);
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

WifiManager::WifiManager() : Configurable("/config/wifi"), SystemObject("wifi"), Observable(Wifi_Off)
{
    tcpip_adapter_init();
    initialize();

    if (enabled)
    {
        on();
    }
}

void WifiManager::on()
{
    if (!ssid.isEmpty())
    {
        enabled = true;
        wifi_enable(ssid.c_str(), password.c_str());
        ESP_LOGI(WIFI_TAG, "WiFi has been enabled, SSID=%s.", ssid.c_str());
        update_status(Wifi_Connecting);
    }
    else
    {
        ESP_LOGW(WIFI_TAG, "No SSID is configured!");
        this->off();
    }
}

void WifiManager::off()
{
    if (enabled)
    {
        disable_wifi();
        ESP_LOGI(WIFI_TAG, "WiFi has been disabled.");
        update_status(Wifi_Off);
        enabled = false;
        initialized = false;
    }
}

void WifiManager::get_config(const JsonObject &json)
{
    this->setup("DryII", "wifi4boat");
    //enabled = json["enabled"].as<bool>();
    //this->setup(json["ssid"].as<String>(), json["password"].as<String>());
}

void WifiManager::set_config(const JsonObject &json)
{
    json["enabled"] = enabled;
    json["ssid"] = ssid;
    json["password"] = password;
}

bool WifiManager::scan_wifi()
{
    bool ret = false;
    if (!scan_running)
    {
        if(!initialized)
        {
            this->initialize();
        }
        this->clear_wifi_list();
        if (!scan_running)
        {
            scan_running = true;
            scan_done = false;
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
            ESP_ERROR_CHECK(esp_wifi_start());
            ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, false));
        }
        else
        {
            ESP_LOGE(WIFI_TAG, "Scan is already in progress!");
        }
    }

    return ret;
}

bool WifiManager::is_scan_complete()
{
    return scan_done;
}