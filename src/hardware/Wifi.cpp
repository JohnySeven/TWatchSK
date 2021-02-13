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
        manager->update_status(Wifi_Disconnected);
        if (!manager->disconnecting)
        {
            if (manager->is_enabled())
            {
                post_gui_warning(GuiMessageCode_t::GUI_WARN_WIFI_DISCONNECTED);
            }
        }
        manager->disconnecting = false;
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
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
        ESP_LOGI(WIFI_TAG, "Scan complete. Total APs found = %u", ap_count);
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_info));
        scan_running = false;
        scan_done = true;
    }
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
    if (!initialized)
    {
        ESP_LOGI(WIFI_TAG, "Initializing wifi...");
        tcpip_adapter_init();
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        cfg.nvs_enable = false;
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiManager::wifi_event_handler, this));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiManager::wifi_event_handler, this));
        ESP_LOGI(WIFI_TAG, "Wifi is initialized!");
        initialized = true;
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

    ESP_LOGI(WIFI_TAG, "esp_wifi_set_ps().");
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
        enabled = false;
        disable_wifi();
        ESP_LOGI(WIFI_TAG, "WiFi has been disabled.");
        this->update_status(WifiState_t::Wifi_Off);
    }
}

void WifiManager::save_config_to_file(JsonObject &json)
{
    ESP_LOGI(WIFI_TAG, "Storing SSID %s to JSON.", ssid.c_str());
    json["enabled"] = enabled;
    json["ssid"] = ssid;
    json["password"] = password;
    JsonArray knownList = json.createNestedArray("known");
    for (int i = 0; i < known_wifi_list_.size(); i++)
    {
        JsonObject known = knownList.createNestedObject();
        auto wifiInfo = known_wifi_list_.at(i);
        known["ssid"] = wifiInfo.ssid;
        known["password"] = wifiInfo.password;
    }
}

void WifiManager::load_config_from_file(const JsonObject &json)
{
    enabled = json["enabled"].as<bool>();
    setup(json["ssid"].as<String>(), json["password"].as<String>());

    if (json.containsKey("known"))
    {
        JsonArray knownList = json["known"].as<JsonArray>();

        for (JsonObject known : knownList)
        {
            KnownWifi_t wifiInfo;
            wifiInfo.ssid = known["ssid"].as<String>();
            wifiInfo.password = known["password"].as<String>();
            known_wifi_list_.push_back(wifiInfo);
        }
    }
}

void WifiManager::setup(String ssid, String password)
{
    this->ssid = ssid;
    this->password = password;
    ESP_LOGI(WIFI_TAG, "SSID has been updated to %s with password ******.", ssid.c_str());
    if (!is_known_wifi(ssid))
    {
        KnownWifi_t wifi;
        wifi.ssid = ssid;
        wifi.password = password;
        known_wifi_list_.push_back(wifi);
    }
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
        ESP_LOGI(WIFI_TAG, "Scanning nearby wifi...");
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());
        ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, false));
        ret = true;
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
    if (ssid != "")
    {
        //if wifi is off enable it
        if (get_status() == WifiState_t::Wifi_Off)
        {
            on();
        }
        else if (get_status() == WifiState_t::Wifi_Disconnected)
        {
            wifi_enable(ssid.c_str(), password.c_str());
        }
        else if (get_status() == WifiState_t::Wifi_Connecting || get_status() == WifiState_t::Wifi_Connected)
        {
            disconnecting = true;
            disable_wifi();
            delay(250); //let the driver stop wifi
            wifi_enable(ssid.c_str(), password.c_str());
        }
    }
}

bool WifiManager::is_known_wifi(const String ssid)
{
    bool ret = false;

    for (KnownWifi_t wifi : known_wifi_list_)
    {
        if (wifi.ssid == ssid)
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
        if (wifi.ssid == ssid)
        {
            password = wifi.password;
            ret = true;
            break;
        }
    }

    return ret;
}