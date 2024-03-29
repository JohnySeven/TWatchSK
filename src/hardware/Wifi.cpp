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
#include "system/events.h"
#include "system/async_dispatcher.h"

const char *WIFI_TAG = "WIFI";
static bool scan_done = false;
static bool scan_running = false;
static wifi_ap_record_t ap_info[WIFI_AP_LIST_MAX_SIZE];
static uint16_t ap_count = 0;

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
        wifi_event_sta_disconnected_t *disconnect_event = (wifi_event_sta_disconnected_t *)event_data;
        auto status = manager->value;
        ESP_LOGI(WIFI_TAG, "wifi_event_handler(): Wifi disconnected with reason=%d, status=%d", disconnect_event->reason, status);
        if (!manager->forced_disconnect_) // this is not an intentional wifi disconnect
        {
            if (manager->is_enabled())
            {
                if (manager->value == WifiState_t::Wifi_Connecting)
                {
                    post_gui_warning(GuiMessageCode_t::GUI_WARN_WIFI_CONNECTION_FAILED);   
                }
                else // manager->value is WifiState_t::Connected
                {
                    post_gui_warning(GuiMessageCode_t::GUI_WARN_WIFI_DISCONNECTED);
                }
                xTaskCreate(&wifi_reconnect_task, "wifi reconnect task", 2048, manager, 5, NULL);
            }
        }

        manager->update_status(Wifi_Disconnected);
        manager->forced_disconnect_ = false; // reset to the normal "waiting for the next disconnect" state
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(WIFI_TAG, "wifi_event_handler(): Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        char buff[24];
        sprintf(buff, IPSTR, IP2STR(&event->ip_info.ip));
        manager->set_ip(String(buff));
        manager->update_status(Wifi_Connected);
        //BS: add a new message here: "Wifi reconnected after X attempts"
        //post_gui_warning(GuiMessageCode_t::GUI_WARN_WIFI_RECONNECTED);
        manager->wifi_retry_counter_ = 0; // to be ready for the next disconnect

        if (!manager->is_known_wifi(manager->ssid_)) //add wifi to known list - we need to save it later on
        {
            KnownWifi_t wifi;
            wifi.known_ssid = manager->ssid_;
            wifi.known_password = manager->password_;
            manager->known_wifi_list_.push_back(wifi);
        }
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE)
    {
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
        ESP_LOGI(WIFI_TAG, "wifi_event_handler(): Scan complete. Total APs found = %u", ap_count);
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_info));
        scan_running = false;
        scan_done = true;
    }
}

void WifiManager::wifi_reconnect_task(void *pvParameter)
{
    auto manager = (WifiManager*)pvParameter;
    esp_wifi_disconnect();
    esp_wifi_stop();
    //manager->off(); //if wifi is turned off this way, the "Disable Wifi" button on the message box doesn't work
    manager->update_status(Wifi_Disconnected);
    ESP_LOGI(WIFI_TAG, "wifi_reconnect_task(): wifi_retry_counter is now %d", manager->wifi_retry_counter_);
    float wifi_retry_time = manager->wifi_retry_counter_ < WIFI_RETRY_ARRAY_SIZE ? manager->wifi_retry_minutes_[manager->wifi_retry_counter_] : WIFI_RETRY_MAX_MINUTES;
    ESP_LOGI(WIFI_TAG, "wifi_reconnect_task(): Will try to reconnect to wifi in %.1f minutes...", wifi_retry_time);
    vTaskDelay((60000 / portTICK_RATE_MS) * wifi_retry_time);
    if (manager->is_enabled())
    {
       manager->wifi_retry_counter_++;
       ESP_LOGI(WIFI_TAG, "wifi_reconnect_task(): Reconnect attempt %d", manager->wifi_retry_counter_);
       manager->connect();
    }
    //remove this task
    vTaskDelete(NULL);
}

int WifiManager::found_wifi_count()
{
    return ap_count;
}

bool WifiManager::is_scan_complete()
{
    return scan_done;
}

void WifiManager::initialize()
{
    if (!initialized_)
    {
        ESP_LOGI(WIFI_TAG, "WifiManager::initialize(): Initializing wifi...");
        tcpip_adapter_init();
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        cfg.nvs_enable = false;
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiManager::wifi_event_handler, this));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiManager::wifi_event_handler, this));
        ESP_LOGI(WIFI_TAG, "WifiManager::initialize(): Wifi is initialized!");
        initialized_ = true;
    }
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

    ESP_LOGI(WIFI_TAG, "wifi_enable(): esp_wifi_set_ps().");
    esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
}

static void disable_wifi()
{
    esp_wifi_disconnect();
    esp_wifi_stop();
    //esp_wifi_deinit();
}

void WifiManager::clear_wifi_list()
{
    ap_count = 0;
}

WifiManager::WifiManager() : Configurable("/config/wifi"), SystemObject("wifi"), Observable(Wifi_Off)
{
    load();
    initialize();

    if (enabled_)
    {
        on();
    }
}

void WifiManager::on()
{
    if (!ssid_.isEmpty())
    {
        enabled_ = true;
        wifi_enable(ssid_.c_str(), password_.c_str());
        ESP_LOGI(WIFI_TAG, "WifiManager::on(): Wifi has been enabled, SSID=%s.", ssid_.c_str());
        update_status(Wifi_Connecting);
    }
    else
    {
        ESP_LOGW(WIFI_TAG, "WifiManager::on(): No SSID is configured!");
        this->off();
        this->configured_ = false;
    }
}

void WifiManager::off(bool force)
{
    if (enabled_ || force)
    {
        enabled_ = false;
        disable_wifi();
        ESP_LOGI(WIFI_TAG, "WifiManager::off(): Wifi has been disabled.");
        this->update_status(Wifi_Off);
    }
}

void WifiManager::save_config_to_file(JsonObject &json)
{
    ESP_LOGI(WIFI_TAG, "Storing SSID %s to JSON.", ssid_.c_str());
    json["enabled"] = enabled_;
    json["ssid"] = ssid_;
    json["password"] = password_;
    JsonArray knownList = json.createNestedArray("known");
    for (int i = 0; i < known_wifi_list_.size(); i++)
    {
        JsonObject known = knownList.createNestedObject();
        auto wifiInfo = known_wifi_list_.at(i);
        known["ssid"] = wifiInfo.known_ssid;
        known["password"] = wifiInfo.known_password;
    }
}

void WifiManager::load_config_from_file(const JsonObject &json)
{
    enabled_ = json["enabled"].as<bool>();
    setup(json["ssid"].as<String>(), json["password"].as<String>());

    if (json.containsKey("known"))
    {
        JsonArray knownList = json["known"].as<JsonArray>();

        for (JsonObject known : knownList)
        {
            KnownWifi_t wifiInfo;
            wifiInfo.known_ssid = known["ssid"].as<String>();
            wifiInfo.known_password = known["password"].as<String>();
            known_wifi_list_.push_back(wifiInfo);
        }
    }
}

void WifiManager::setup(String ssid, String password)
{
    ssid_ = ssid;
    password_ = password;
    ESP_LOGI(WIFI_TAG, "SSID has been updated to %s with password ******.", ssid.c_str());
    configured_ = !ssid.isEmpty();
}

bool WifiManager::scan_wifi()
{
    bool ret = false;
    if (!scan_running)
    {
        scan_running = true;
        initialize();
        clear_wifi_list();
        scan_done = false;
        ESP_LOGI(WIFI_TAG, "WifiManager::scan_wifi(): Scanning nearby Wifi state=%d...", (int)value);
        if (value == WifiState_t::Wifi_Connected)
        {
            ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, false));
            ret = true;
        }
        else if (value == WifiState_t::Wifi_Disconnected || value == WifiState_t::Wifi_Off)
        {
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
            ESP_ERROR_CHECK(esp_wifi_start());
            delay(150);
            ESP_ERROR_CHECK(esp_wifi_disconnect());
            delay(150);
            ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, false));
            ret = true;
        }
    }
    else
    {
        ESP_LOGE(WIFI_TAG, "Scan is already in progress!");
    }

    return ret;
}

const wifi_ap_record_t WifiManager::get_found_wifi(int index)
{
    return ap_info[index];
}

void WifiManager::connect()
{
    if (ssid_ != "")
    {
        if (get_status() == WifiState_t::Wifi_Connecting || get_status() == WifiState_t::Wifi_Connected)
        {
            forced_disconnect_ = true; // to prevent "Wifi disconnected" messages from appearing on the watch for an "on-purpose" disconnect
        }

        off(true);  //let's force calling disable wifi
        delay(100); //let the driver stop wifi
        on();
    }
    else
    {
        configured_ = false;
    }
}

bool WifiManager::is_known_wifi(const String ssid)
{
    bool ret = false;

    for (KnownWifi_t wifi : known_wifi_list_)
    {
        if (wifi.known_ssid == ssid)
        {
            ret = true;
            break;
        }
    }

    return ret;
}

bool WifiManager::get_known_wifi_password(const String ssid, String &password)
{
    bool ret = false;

    for (KnownWifi_t wifi : known_wifi_list_)
    {
        if (wifi.known_ssid == ssid)
        {
            password = wifi.known_password;
            ret = true;
            break;
        }
    }

    return ret;
}

int WifiManager::get_wifi_rssi()
{
    int ret = 0;
    wifi_ap_record_t info;
    if (esp_wifi_sta_get_ap_info(&info) == ESP_OK)
    {
        ret = info.rssi;
    }

    return ret;
}