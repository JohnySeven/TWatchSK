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
    Wifi_Off = 0,
    Wifi_Disconnected = 1,
    Wifi_Connecting = 2,
    Wifi_Connected = 3
};

class WifiManager : public Configurable, public SystemObject, public Observable<WifiState_t>
{
public:
    WifiManager();
    void on();
    void off();
    String get_ip() { return ip; }
    WifiState_t get_status() { return value; }
    bool is_enabled() { return enabled; }
    bool is_connected() { return value == WifiState_t::Wifi_Connected; }
    bool is_configured() { return this->ssid != ""; }
    void set_ip(String ip) { this->ip = ip; }
    void update_status(WifiState_t value) { Observable<WifiState_t>::emit(value); }
    void setup(String ssid, String password);

    bool scan_wifi();
    bool is_scan_complete();
    int found_wifi_count();
    const wifi_ap_record_t get_found_wifi(int index);
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
    static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data);
    void clear_wifi_list();
};