#pragma once
#include "ArduinoJson.h"
#include "esp_websocket_client.h"
#include "vector"
#include "functional"
#include "system/configurable.h"
#include "system/observableobject.h"

static const char *WS_TAG = "WS";

class SignalKSocket : public Configurable, public ObservableObject
{
    public:
        SignalKSocket();
        bool connect();
        bool disconnect();
        void subscribe(std::function<void(String, JsonObject)> receiver) {
            receivers.push_back(receiver);
        }
        void update_connected(bool value)
        {
            connected = value;
            notify("connected", &value);
        }
        void parse_data(int length, const char*data);
        esp_websocket_client_handle_t get_ws() { return websocket; }
    private:
        String server = "";
        int port = 0;
        bool connected = false;
        esp_websocket_client_handle_t websocket;
        std::vector<std::function<void(String, JsonObject)>> receivers;
        ObservableObject*wifi;
        void get_config(const JsonObject &json) override;
        void set_config(const JsonObject &json) override;
};