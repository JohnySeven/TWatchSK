#pragma once
#include "ArduinoJson.h"
#include "config.h"
#include "functional"
#include "component.h"
#include <vector>
#include "networking/signalk_socket.h"
#include "system/uuid.h"

struct Data_formating_t
{
    float multiply = 1.0;
    float offset = 0.0;
    int decimal_places = 1;
    char *string_format = NULL;
};

class DataAdapter
{
public:
    DataAdapter(String sk_path, int sk_subscription_period, Component *target);
    const String &get_path() { return path; }
    int get_subscription_period() { return subscription_period; }
    void on_updated(const JsonVariant &value)
    {
        targetObject_->update(value);
    }

    void on_offline()
    {
        targetObject_->on_offline();
    }

    bool put_request(bool value)
    {
        ESP_LOGI("DataAdapter", "Put request %s with bool value %s", path.c_str(), value ? "true" : "false");

        DynamicJsonDocument request(1024);
        JsonObject root = request.to<JsonObject>();
        root["requestId"] = UUID::new_id();
        JsonObject put_data = root.createNestedObject("put");
        put_data["path"] = path;
        put_data["value"] = value;
        
        return put_request(root);
    }

    bool put_request(JsonObject &obj)
    {
        if (ws_socket_->get_state() == WebsocketState_t::WS_Connected)
        {
            return ws_socket_->send_put_request(obj);
        }
        else
        {
            return false;
        }
    }
    void initialize(SignalKSocket *socket)
    {
        ws_socket_ = socket;
        socket->add_subscription(get_path(), get_subscription_period(), false);
    }

    static std::vector<DataAdapter *> &get_adapters();

protected:
    int subscription_period = 0;
    Data_formating_t formating_options_;
    String path = "";
    Component *targetObject_ = NULL;
    SignalKSocket *ws_socket_ = NULL;
};
