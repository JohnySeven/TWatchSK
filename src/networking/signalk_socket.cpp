#include "signalk_socket.h"
static const char *WS_TAG = "WS";

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
        socket->update_status(WS_Connected);

        //esp_websocket_client_send_text(socket->get_ws(), subscriptionMessage, sizeof(subscriptionMessage)-1, portMAX_DELAY);
    }
    else if (event_id == WEBSOCKET_EVENT_DISCONNECTED)
    {
        ESP_LOGI(WS_TAG, "Web socket disconnected from server!");
        socket->update_status(WS_Offline);
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
            if (data->data_len > 0)
            {
                socket->parse_data(data->data_len, data->data_ptr);
                ESP_LOGW(WS_TAG, "Received=%.*s", data->data_len, (char *)data->data_ptr);
            }
        }
        ESP_LOGW(WS_TAG, "Total payload length=%d, data_len=%d, current payload offset=%d\r\n", data->payload_len, data->data_len, data->payload_offset);
    }
}

SignalKSocket::SignalKSocket(WifiManager *wifi) : Configurable("/config/websocket"), SystemObject("websocket"), Observable(WS_Offline)
{
    server = "pi.boat";
    port = 3000;
    wifi->attach(this);

    if (wifi != NULL)
    {
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
    if (value == WS_Offline)
    {
        esp_websocket_client_config_t ws_cfg = {
            .host = server.c_str(),
            .path = "/signalk/v1/stream?subscribe=none"};
        ws_cfg.port = port;

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
    if (value != WS_Offline)
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
    server = json["server"].as<String>();
    port = json["port"].as<int>();
}

void SignalKSocket::set_config(const JsonObject &json)
{
    json["server"] = server;
    json["port"] = port;
}

void SignalKSocket::parse_data(int length, const char *data)
{
    DynamicJsonDocument doc(1024);

    auto result = deserializeJson(doc, data, length);
    if (result.code() == DeserializationError::Ok)
    {
        ESP_LOGI(WS_TAG, "Got message from websocket with len=%d", length);
    }
    else
    {
        ESP_LOGE(WS_TAG, "Websocket json deserialization failed=%d", result.code());
    }
}

void SignalKSocket::notify_change(const WifiState_t &wifiState)
{
    ESP_LOGI(WS_TAG, "Detected Wifi=%d", (int)wifiState);
    if (wifiState == Wifi_Connected)
    {
        if (this->connect())
        {
            ESP_LOGI(WS_TAG, "Auto connect OK!");
        }
        else
        {
            ESP_LOGW(WS_TAG, "Auto connect ERROR!");
        }
    }
}