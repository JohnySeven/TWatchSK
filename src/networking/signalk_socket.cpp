#include "signalk_socket.h"

static void ws_event_handler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data)
{
    SignalKSocket *socket = (SignalKSocket *)arg;
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    if (event_id == WEBSOCKET_EVENT_ERROR)
    {
        ESP_LOGE(WS_TAG, "Web socket error!");
    }
    else if (event_id == WEBSOCKET_EVENT_CONNECTED)
    {
        ESP_LOGI(WS_TAG, "Web socket connected to server!");
        ESP_LOGI(WS_TAG, "Sending subscription message...");
        socket->update_connected(true);

        //esp_websocket_client_send_text(socket->get_ws(), subscriptionMessage, sizeof(subscriptionMessage)-1, portMAX_DELAY);
    }
    else if (event_id == WEBSOCKET_EVENT_DISCONNECTED)
    {
        ESP_LOGI(WS_TAG, "Web socket disconnected from server!");
        socket->update_connected(false);
    }
    else if (event_id == WEBSOCKET_EVENT_DATA)
    {
        ESP_LOGI(WS_TAG, "WEBSOCKET_EVENT_DATA");
        ESP_LOGI(WS_TAG, "Received opcode=%d", data->op_code);
        if (data->op_code == 0x08 && data->data_len == 2)
        {
            ESP_LOGW(WS_TAG, "Received closed message with code=%d", 256 * data->data_ptr[0] + data->data_ptr[1]);
        }
        else
        {
            socket->parse_data(data->data_len, data->data_ptr);
            ESP_LOGW(WS_TAG, "Received=%.*s", data->data_len, (char *)data->data_ptr);
        }
        ESP_LOGW(WS_TAG, "Total payload length=%d, data_len=%d, current payload offset=%d\r\n", data->payload_len, data->data_len, data->payload_offset);
    }
}

SignalKSocket::SignalKSocket() : Configurable("/config/websocket"), ObservableObject("websocket")
{
    wifi = get_object("wifi");
    if (wifi != NULL)
    {
        wifi->subscribe([this](ObservableObject *wifi, String& property, void *value) {
            if(property=="connected")
            {
                bool connected = *reinterpret_cast<bool*>(value);
                ESP_LOGI(WS_TAG, "Detected Wifi=%d", connected);
                if(connected)
                {
                    if(this->connect())
                    {
                        ESP_LOGI(WS_TAG, "Auto connect OK!");
                    }
                    else
                    {
                        ESP_LOGW(WS_TAG, "Auto connect ERROR!");
                    }
                    
                }           
            }
        });

        ESP_LOGI(WS_TAG, "SignalK socket initialized.");
    }
    else
    {
        ESP_LOGE(WS_TAG, "Wifi object not found!!!");
    }    
}

bool SignalKSocket::connect()
{
    bool ret = false;
    if (!connected)
    {
        esp_websocket_client_config_t ws_cfg = {
            .host = "pi.boat",
            .path = "/signalk/v1/stream?subscribe=none"
        };
        ws_cfg.port = 3000;

        ESP_LOGI(WS_TAG, "Initializing websocket %s:%d...", ws_cfg.host, ws_cfg.port);

        websocket = esp_websocket_client_init(&ws_cfg);

        if (esp_websocket_register_events(websocket, WEBSOCKET_EVENT_ANY, ws_event_handler, this) == ESP_OK)
        {
            if (esp_websocket_client_start(websocket) == ESP_OK)
            {
                ESP_LOGI(WS_TAG, "Websocket initialized!");
                ret = true;
            }
        }
    }
 
    return ret;
}

bool SignalKSocket::disconnect()
{
    if (connected)
    {
        ESP_LOGI(WS_TAG, "Disconnecting websocket...");

        if (esp_websocket_client_stop(websocket) == ESP_OK)
        {
            if (esp_websocket_client_destroy(websocket) == ESP_OK)
            {
                ESP_LOGI(WS_TAG, "Websocket destroyed.");
            }
        }
    }
}

void SignalKSocket::get_config(const JsonObject &json)
{
    //server = json["server"].as<String>();
    //port = json["port"].as<int>();
    server = "pi.boat";
    port = 3000;
    notify("server", &server);
    notify("port", &port);
}

void SignalKSocket::set_config(const JsonObject &json)
{
    json["server"] = server;
    json["port"] = port;
}

void SignalKSocket::parse_data(int length, const char *data)
{
    DynamicJsonDocument doc(1024);

    deserializeJson(doc, data, length);
}