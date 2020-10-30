#pragma once
#include "config/configurable.h"
#include "FreeRTOS.h"

enum {
    Q_EVENT_WIFI_SCAN_DONE,
    Q_EVENT_WIFI_CONNECTED,
    Q_EVENT_WIFI_DISCONNECTED,
    Q_EVENT_WIFI_ENABLED,
    Q_EVENT_WIFI_DISABLED
} WIFI_EVENTS_T;

class WifiManager : public Configurable
{
public:
    WifiManager() : Configurable("config/wifi")
    {
        if (enabled)
        {
            on();
        }
    }
    void on();
    void off();
    bool is_enabled() { return enabled; }

private:
    void get_config(const JsonObject &json) override;
    void set_config(const JsonObject &json) override;
    String ssid;
    String password;
    bool enabled;
};