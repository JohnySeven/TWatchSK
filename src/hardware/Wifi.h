#pragma once
#include <vector>
#include "system/configurable.h"
#include "FreeRTOS.h"
#include "system/systemobject.h"
#include "system/observable.h"
#include "esp_wifi.h"
#define WIFI_AP_LIST_MAX_SIZE 32

enum WifiState_t
{
    Wifi_Off,
    Wifi_Disconnected,
    Wifi_Connecting,
    Wifi_Connected
};

struct KnownWifi_t
{
    String ssid;
    String password;
};

class WifiManager : public Configurable, public SystemObject, public Observable<WifiState_t>
{
public:
    WifiManager();
    void on();
    void off(bool force = false);
    void connect();
    String get_ip() { return ip; }
    String get_configured_ssid ()
    {
        return ssid;
    }
    WifiState_t get_status() { return value; }
    bool is_enabled() { return enabled; }
    bool is_connected() { return value == WifiState_t::Wifi_Connected; }
    void set_ip(String ip) { this->ip = ip; }
    void update_status(WifiState_t value) { Observable<WifiState_t>::emit(value); }
    void setup(String ssid, String password);
    bool is_configured() { return configured; }
    bool scan_wifi();
    bool is_scan_complete();
    int found_wifi_count();
    const wifi_ap_record_t get_found_wifi(int index);
    bool is_known_wifi(const String ssid);
    bool get_known_wifi_password(const String ssid, String &password);
    int get_wifi_rssi();
private:
    void initialize();
    virtual void load_config_from_file(const JsonObject &json) override final;
    virtual void save_config_to_file(JsonObject &json) override final;
    String ssid;
    String password;
    String ip = "";
    bool enabled = false;
    bool connected = false;
    bool initialized = false;
    bool disconnecting = false;
    bool configured = false;
    static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data);
    void clear_wifi_list();
    std::vector<KnownWifi_t> known_wifi_list_;
};