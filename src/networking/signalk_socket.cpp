#include "signalk_socket.h"
#include "system/uuid.h"
#include "system/events.h"

static const char *WS_TAG = "WS";
const char UnsubscribeMessage[] = "{\"context\":\"*\",\"unsubscribe\":[{\"path\":\"*\"}]}";

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
        socket->update_subscriptions();
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
    if (value == WS_Offline && server != "")
    {
        char url[256];
        sprintf(url, "/signalk/v1/stream?subscribe=none&token=%s", token.c_str());
        esp_websocket_client_config_t ws_cfg = {
            .host = server.c_str(),
            .path = url};
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
    DynamicJsonDocument doc(2048);

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
        else if (doc.containsKey("requestId"))
        {
            String requestState = doc["state"];

            messageType = "Request status";

            if (requestState == "COMPLETED" && doc.containsKey("accessRequest"))
            {
                messageType = "Access request";

                JsonObject accessRequest = doc["accessRequest"].as<JsonObject>();
                String permission = accessRequest["permission"].as<String>();
                ESP_LOGI(WS_TAG, "Got token request response with status %s from server!", permission.c_str());
                if (permission == "APPROVED")
                {
                    token = accessRequest["token"].as<String>();
                    save();
                }
            }
        }
        else if (doc.containsKey("updates"))
        {
            messageType = "Delta";
            JsonArray updates = doc["updates"].as<JsonArray>();

            for (int i = 0; i < updates.size(); i++)
            {
                JsonObject update = updates[i].as<JsonObject>();
                JsonObject source = update["source"].as<JsonObject>();
                String sourceName = source["label"].as<String>();
                JsonArray values = update["values"].as<JsonArray>();
                for (int v = 0; v < values.size(); v++)
                {
                    JsonObject value = values[i].as<JsonObject>();
                    String path = value["path"].as<String>();

                    if (path.startsWith("notifications."))
                    {
                        bool active = is_notification_active(path);
                        JsonObject notification = value["value"];
                        String state = notification["state"].as<String>();
                        ESP_LOGI(WS_TAG, "Got SK notification %s with state %s, active=%d", path.c_str(), state.c_str(), active);

                        if (state == "alarm" || state == "alert")
                        {
                            if (!active)
                            {
                                String message = notification["message"];
                                GuiEvent_t event;
                                event.argument = malloc(message.length() + 1);
                                strcpy((char *)event.argument, message.c_str());
                                event.event = GuiEventType_t::GUI_SHOW_WARNING;
                                post_gui_update(event);
                                activeNotifications.push_back(path);
                            }
                        }
                        else
                        {
                            remove_active_notification(path);
                        }
                    }
                    else if(is_low_power())
                    {
                        ESP_LOGI(WS_TAG, "Got SK value update %s", path.c_str());
                        String json;
                        serializeJson(value, json);
                        GuiEvent_t event;
                        event.argument = malloc(json.length() + 1);
                        strcpy((char *)event.argument, json.c_str());
                        event.event = GuiEventType_t::GUI_SIGNALK_UPDATE;
                        post_gui_update(event);
                    }
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
}

SignalKSubscription *SignalKSocket::add_subscription(String path, uint period, bool is_low_power)
{
    auto iterator = subscriptions.find(path);
    if (iterator != subscriptions.end())
    {
        return iterator->second;
    }
    else
    {
        auto newSubscription = new SignalKSubscription(path, period, is_low_power);
        subscriptions[path] = newSubscription;
        return newSubscription;
    }
}

void SignalKSocket::update_subscriptions()
{
    if (value == WebsocketState_t::WS_Connected)
    {
        bool is_lp = xEventGroupGetBits(g_app_state) & G_APP_STATE_LOW_POWER;
        DynamicJsonDocument subscriptionsJson(subscriptions.size() * 100);
        int count = 0;
        subscriptionsJson["context"] = "vessels.self";
        JsonArray subscribe = subscriptionsJson.createNestedArray("subscribe");
        for (auto subscription : this->subscriptions)
        {
            if (!is_lp || (is_lp && subscription.second->get_low_power()))
            {
                count++;
                String path = subscription.second->get_path();
                uint period = subscription.second->get_period();
                JsonObject subscribePath = subscribe.createNestedObject();

                subscribePath["path"] = path;
                subscribePath["period"] = period;
                ESP_LOGI(WS_TAG, "Adding %s subscription with listen_delay %d ms", path.c_str(), period);
            }
        }

        esp_websocket_client_send_text(websocket, UnsubscribeMessage, strlen(UnsubscribeMessage), portMAX_DELAY);

        if (count > 0)
        {
            String subscriptionMessage;
            serializeJson(subscriptionsJson, subscriptionMessage);
            ESP_LOGI(WS_TAG, "Subscription: %s", subscriptionMessage.c_str());
            esp_websocket_client_send_text(websocket, subscriptionMessage.c_str(), subscriptionMessage.length(), portMAX_DELAY);
        }
    }
}

bool SignalKSocket::is_notification_active(String path)
{
    auto ret = false;
    for (int i = 0; i < activeNotifications.size(); i++)
    {
        if (activeNotifications[i] == path)
        {
            ret = true;
            break;
        }
    }
    return ret;
}

void SignalKSocket::remove_active_notification(String path)
{    
    for (auto it = activeNotifications.begin(); it != activeNotifications.end(); it++ )
    {
        if (it.base()->equals(path))
        {
            activeNotifications.erase(it);
            break;
        }

    }
}