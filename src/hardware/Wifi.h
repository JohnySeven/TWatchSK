#pragma once
#include "system/configurable.h"
#include "FreeRTOS.h"
#include "system/ObservableObject.h"

class WifiManager : public Configurable, public ObservableObject
{
public:
    WifiManager() : Configurable("/config/wifi"), ObservableObject("wifi")
    {
        if (enabled)
        {
            on();
        }
    }
    void on();
    void off();
    bool get_enabled() { return enabled; }
    bool get_connected() { return connected; }
    String get_ip() { return ip; }
    void set_ip(String ip)
    {
        this->ip = ip;
        notify("ip", &ip);
    }
    void set_connected(bool value)
    {
        this->connected = value;
        notify("connected", &connected);
    }
    void setup(String ssid, String password)
    {
        this->ssid = ssid;
        this->password = password;
        notify("ssid", &ssid);
        notify("password", &password);
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