#include "signalk_socket.h"
#include "system/uuid.h"

static const char *WS_TAG = "WS";

void SignalKSocket::ws_event_handler(void *arg, esp_event_base_t event_base,
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
        socket->update_status(WebsocketState_t::WS_Connected);
        //esp_websocket_client_send_text(socket->get_ws(), subscriptionMessage, sizeof(subscriptionMessage)-1, portMAX_DELAY);
    }
    else if (event_id == WEBSOCKET_EVENT_DISCONNECTED)
    {
        ESP_LOGI(WS_TAG, "Web socket disconnected from server!");
        socket->update_status(WebsocketState_t::WS_Offline);
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
    load();
    if (clientId == "")
    {
        clientId = UUID::new_id();
    }
    server = "pi.boat";
    port = 3000;
    wifi->attach(this);

    if (wifi != NULL)
    {
        ESP_LOGI(WS_TAG, "SignalK socket initialized with client ID=%s.", clientId.c_str());
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
        char url[256];
        sprintf(url, "/signalk/v1/stream?subscribe=none&token=%s", token.c_str());
        esp_websocket_client_config_t ws_cfg = {
            .host = server.c_str(),
            .path = url,
        };
        ws_cfg.port = port;

        ESP_LOGI(WS_TAG, "Initializing websocket ws://%s:%d%s...", ws_cfg.host, ws_cfg.port, url);

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
    if (value != WebsocketState_t::WS_Offline)
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

void SignalKSocket::load_config_from_file(const JsonObject &json)
{
    server = json["server"].as<String>();
    port = json["port"].as<int>();
    token = json["token"].as<String>();
    clientId = json["id"].as<String>();
    ESP_LOGI(WS_TAG, "Loaded config with server %s:%d", server.c_str(), port);
}

void SignalKSocket::save_config_to_file(JsonObject &json)
{
    json["server"] = server;
    json["port"] = port;
    json["token"] = token;
    json["id"] = clientId;
}

void SignalKSocket::parse_data(int length, const char *data)
{
    DynamicJsonDocument doc(1024);

    auto result = deserializeJson(doc, data, length);
    if (result.code() == DeserializationError::Ok)
    {
        String messageType = "Unknown";
        if (doc.containsKey("name"))
        {
            messageType = "Welcome";
            serverName = doc["name"].as<String>();
            serverVersion = doc["version"].as<String>();

            if (token.isEmpty())
            {
                send_token_permission();
            }
        }
        else if(doc.containsKey("requestId"))
        {
            String requestState = doc["state"];

            messageType = "Request status";

            if(requestState == "COMPLETED" && doc.containsKey("accessRequest"))
            {
                messageType = "Access request";

                JsonObject accessRequest = doc["accessRequest"].as<JsonObject>();
                String permission = accessRequest["permission"].as<String>();
                ESP_LOGI(WS_TAG, "Got token request response with status %s from server!", permission.c_str());
                if(permission == "APPROVED")
                {
                    token = accessRequest["token"].as<String>();
                    save();
                }
            }
        }

        ESP_LOGI(WS_TAG, "Got message %s from websocket with len=%d", messageType.c_str(), length);
    }
    else
    {
        ESP_LOGE(WS_TAG, "Websocket json deserialization failed=%d", result.code());
    }
}

void SignalKSocket::notify_change(const WifiState_t &wifiState)
{
    ESP_LOGI(WS_TAG, "Detected Wifi=%d, Socket state=%d", (int)wifiState, (int)this->value);

    if (wifiState == Wifi_Connected && value == WebsocketState_t::WS_Offline)
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
    else if ((wifiState == Wifi_Disconnected || wifiState == Wifi_Off) && value != WebsocketState_t::WS_Offline)
    {
        this->disconnect();
    }
}

void SignalKSocket::send_json(const JsonObject &json)
{
    static char buff[1024];
    size_t len = serializeJson(json, buff);
    ESP_LOGI(WS_TAG, "Sending json payload=%s", buff);
    esp_websocket_client_send_text(websocket, buff, len, portMAX_DELAY);
}

void SignalKSocket::send_token_permission()
{
    String requestId = UUID::new_id();

    ESP_LOGI(WS_TAG, "Requesting SignalK access token RequestId=%s...", requestId.c_str());

    StaticJsonDocument<256> requestJson;
    requestJson["requestId"] = requestId;
    auto accessRequest = requestJson.createNestedObject("accessRequest");
    accessRequest["clientId"] = clientId;
    accessRequest["description"] = "TWatchSK";
    accessRequest["permissions"] = "admin";
    send_json(requestJson.as<JsonObject>());

    /*let accessRequest = {
      requestId: requestId,
      accessRequest: {
        clientId: this.AppSettingsService.getKipUUID(),
        description: "Kip web app",
        permissions: "admin"
      }
    }*/
}