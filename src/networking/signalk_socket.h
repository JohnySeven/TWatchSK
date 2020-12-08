#pragma once
#include "ArduinoJson.h"
#include "esp_websocket_client.h"
#include "vector"
#include "functional"
#include "system/configurable.h"
#include "system/systemobject.h"
#include "system/observable.h"
#include "hardware/Wifi.h"

enum WebsocketState_t
{
    WS_Offline = 0,
    WS_Connecting = 1,
    WS_Connected = 2
};

class SignalKSocket : public Configurable, public SystemObject, public Observable<WebsocketState_t>, public Observer<WifiState_t>
{
    public:
        SignalKSocket(WifiManager*wifi);
        bool connect();
        bool disconnect();
        void subscribe(std::function<void(String, JsonObject)> receiver) {
            receivers.push_back(receiver);
        }
        void parse_data(int length, const char*data);
        esp_websocket_client_handle_t get_ws() { return websocket; }
        void update_status(WebsocketState_t status)
        {
            Observable<WebsocketState_t>::emit(status);
        }
        void notify_change(const WifiState_t &wifi) override;
    private:
        String server = "";
        int port = 0;
        esp_websocket_client_handle_t websocket;
        std::vector<std::function<void(String, JsonObject)>> receivers;
        SystemObject*wifi;
        void get_config(const JsonObject &json) override;
        void set_config(const JsonObject &json) override;
};