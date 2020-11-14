#pragma once
#include "system/configurable.h"
#include "FreeRTOS.h"
#include "system/systemobject.h"
#include "system/observable.h"

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
    void set_ip(String ip) { this->ip = ip; }
    void update_status(WifiState_t value) { Observable<WifiState_t>::emit(value); }
    void setup(String ssid, String password)
    {
        this->ssid = ssid;
        this->password = password;
    }

private:
    void get_config(const JsonObject &json) override;
    void set_config(const JsonObject &json) override;
    String ssid;
    String password;
    String ip = "";
    bool enabled;
    bool connected;
};