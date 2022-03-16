#include "signalk_socket.h"
#include "system/uuid.h"
#include "system/events.h"
#include "ui/localization.h"
#include "esp_transport.h"
#define LOG_WS_DATA 0
static const char *WS_TAG = "WS";
const char UnsubscribeMessage[] = "{\"context\":\"*\",\"unsubscribe\":[{\"path\":\"*\"}]}";

struct esp_websocket_client_info
{
    esp_event_loop_handle_t event_handle;
    void *transport_list;
    esp_transport_handle_t transport;
    void *config;
    int state;
    uint64_t keepalive_tick_ms;
    uint64_t reconnect_tick_ms;
    uint64_t ping_tick_ms;
    int wait_timeout_ms;
    int auto_reconnect;
    bool run;
    EventGroupHandle_t status_bits;
    xSemaphoreHandle lock;
    char *rx_buffer;
    char *tx_buffer;
    int buffer_size;
    int last_opcode;
    int payload_len;
    int payload_offset;
};

void SignalKSocket::ws_event_handler(void *arg, esp_event_base_t event_base,
                                     int32_t event_id, void *event_data)
{
    SignalKSocket *socket = (SignalKSocket *)arg;
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    if (socket->websocket_initialized)
    {
        if (event_id == WEBSOCKET_EVENT_ERROR)
        {
            ESP_LOGE(WS_TAG, "Web socket error!");
        }
        else if (event_id == WEBSOCKET_EVENT_CONNECTED)
        {
            ESP_LOGI(WS_TAG, "Web socket connected to server!");
            socket->reconnect_counter_ = socket->reconnect_count_; //restore reconnect counter to 3
            socket->delta_counter = 0;                             //clear the socket delta counter

            socket->update_status(WebsocketState_t::WS_Connected);

            //if token is empty, request access
            if (socket->token.isEmpty())
            {
                socket->send_token_permission();
            }
            else //token isn't empty send subscription requests
            {
                socket->update_subscriptions(true);
            }
        }
        else if (event_id == WEBSOCKET_EVENT_DISCONNECTED)
        {
            socket->update_status(WebsocketState_t::WS_Offline);
            ESP_LOGI(WS_TAG, "Web socket disconnected from server! Wifi enabled=%d", (int)socket->wifi->is_enabled());
            if (!socket->wifi->is_connected())
            {
                ESP_LOGI(WS_TAG, "Wifi is disconnected, disconnecting and destroying WS client.");
                socket->disconnect();
            }
            else
            {
                ESP_LOGI(WS_TAG, "Unexpected disconnection. Will try reconnect for %d times.", socket->reconnect_counter_);

                if (socket->reconnect_counter_ == 0)
                {
                    post_gui_warning(GuiMessageCode_t::GUI_WARN_SK_LOST_CONNECTION);
                    socket->disconnect();
                }
                else
                {
                    socket->reconnect_counter_--;
                }
            }
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
#if LOG_WS_DATA==1
                    ESP_LOGW(WS_TAG, "Received=%.*s", data->data_len, (char *)data->data_ptr);
#endif
                }
            }
            ESP_LOGW(WS_TAG, "Total payload length=%d, data_len=%d, current payload offset=%d\r\n", data->payload_len, data->data_len, data->payload_offset);
        }
    }
}

SignalKSocket::SignalKSocket(WifiManager *wifi) : Configurable("/config/websocket"), SystemObject("websocket"), Observable(WS_Offline)
{
    load();
    if (clientId == "")
    {
        clientId = UUID::new_id();
    }

    wifi->attach(this);
    this->wifi = wifi;

    if (wifi != NULL)
    {
        ESP_LOGI(WS_TAG, "SignalK socket initialized with client ID=%s.", clientId.c_str());
    }
}

bool SignalKSocket::connect()
{
    bool ret = false;

    ESP_LOGI(WS_TAG, "Connecting socket to server %s:%d", server.c_str(), port);

    if (value == WS_Offline && server != "" && port > 0 && wifi->is_connected())
    {
        char url[256];
        sprintf(url, "/signalk/v1/stream?subscribe=none&token=%s", token.c_str());
        esp_websocket_client_config_t ws_cfg = {
            .host = server.c_str(),
            .path = url};
        ws_cfg.port = port;
        reconnect_counter_ = reconnect_count_; //restore reconnect counter

        ESP_LOGI(WS_TAG, "Initializing websocket ws://%s:%d%s...", ws_cfg.host, ws_cfg.port, url);

        websocket = esp_websocket_client_init(&ws_cfg);
        websocket_initialized = true;
        emit(WebsocketState_t::WS_Connecting);

        if (esp_websocket_register_events(websocket, WEBSOCKET_EVENT_ANY, ws_event_handler, this) == ESP_OK)
        {
            if (esp_websocket_client_start(websocket) == ESP_OK)
            {
                ESP_LOGI(WS_TAG, "Websocket initialized!");
                ret = true;
            }
        }
    }
    else
    {
        emit(WebsocketState_t::WS_Offline);
    }

    return ret;
}

bool SignalKSocket::disconnect()
{
    auto ret = false;

    if (websocket_initialized && websocket != NULL)
    {
        ESP_LOGI(WS_TAG, "Disconnecting websocket...");

        if (esp_websocket_client_is_connected(websocket))
        {
            esp_websocket_client_stop(websocket);
        }

        delay(100);
        auto clenaup = esp_websocket_client_destroy(websocket);
        if (clenaup == ESP_OK)
        {
            websocket_initialized = false;
            websocket = NULL;
            ESP_LOGI(WS_TAG, "Websocket destroyed.");
        }
        else
        {
            ESP_LOGE(WS_TAG, "Failed to cleanup websocket with error %d", clenaup);
        }

        update_status(WebsocketState_t::WS_Offline);
        ret = true;
    }

    return ret;
}

void SignalKSocket::load_config_from_file(const JsonObject &json)
{
    server = json["server"].as<String>();
    port = json["port"].as<int>();
    token = json["token"].as<String>();
    clientId = json["id"].as<String>();
    sync_time_with_server = json["synctime"].as<bool>();
    ESP_LOGI(WS_TAG, "Loaded config with server %s:%d", server.c_str(), port);
}

void SignalKSocket::save_config_to_file(JsonObject &json)
{
    json["server"] = server;
    json["port"] = port;
    json["token"] = token;
    json["id"] = clientId;
    json["synctime"] = sync_time_with_server;
}

bool updateSystemTime(String time)
{
       bool ret = false;
       char timeCh[30];
       const char pattern[] = ":T-.";
       time.toCharArray(timeCh, 30);

       //2020-09-15T07:56:44.225Z
       auto *ptr = strtok(timeCh, pattern);
       if (ptr != NULL)
       {
              auto year = atoi(ptr);
              ptr = strtok(NULL, pattern);
              if (ptr != NULL)
              {
                     auto month = atoi(ptr);
                     ptr = strtok(NULL, pattern);

                     if (ptr != NULL)
                     {
                            auto day = atoi(ptr);
                            ptr = strtok(NULL, pattern);

                            if (ptr != NULL)
                            {
                                   auto hour = atoi(ptr);
                                   ptr = strtok(NULL, pattern);

                                   if (ptr != NULL)
                                   {
                                          auto minute = atoi(ptr);
                                          ptr = strtok(NULL, pattern);
                                          if (ptr != NULL)
                                          {
                                                 auto second = atoi(ptr);

                                                 if (year > 2000 && month > 0 && month < 13 && day > 1 && day < 32 && hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 && second >= 0 && second <= 59)
                                                 {
                                                        ESP_LOGI(WS_TAG, "Parsed time from server: %d-%d-%d %d:%d:%d", year, month, day, hour, minute, second);
                                                        ret = true;
                                                 }
                                          }
                                   }
                            }
                     }
              }
       }

       return ret;
}

void SignalKSocket::parse_data(int length, const char *data)
{
    DynamicJsonDocument doc(4096);

    auto result = deserializeJson(doc, data, length);
    if (result.code() == DeserializationError::Ok)
    {
        delta_counter++;
        String messageType = "Unknown";

        if (doc.containsKey("requestId"))
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
                    token_request_pending = false;
                    pending_token_request_id = "";
                    update_subscriptions(); //update subscriptions
                    save();
                }
                else
                {
                    post_gui_warning(GuiMessageCode_t::GUI_WARN_SK_REJECTED);
                }
            }
        }
        else if (doc.containsKey("updates"))
        {
            messageType = "Delta";
            delta_counter++;
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

                        if (state == "alarm" || state == "alert" || state == "warn" || state == "emergency") //alarm is active we need to wake up the watch and show alert text on the display
                        {
                            if (!active)
                            {
                                String message = notification["message"];
                                post_gui_warning(message);
                                activeNotifications.push_back(path);
                            }
                        }
                        else
                        {
                            remove_active_notification(path);
                        }
                    }
                    else if (!is_low_power())
                    {
                        ESP_LOGI(WS_TAG, "Got SK value update %s", path.c_str());
                        String json;
                        serializeJson(value, json);
                        post_gui_sk_dv_update(json);
                    }
                }
            }
        }
        else if(doc.containsKey("timestamp"))
        {
            String timestamp = doc["timestamp"];
            updateSystemTime(timestamp);
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
    ESP_LOGI(WS_TAG, "Detected Wifi=%d", (int)wifiState);
    ESP_LOGI(WS_TAG, "Socket status=%d", (int)value);

    if (wifiState == Wifi_Connected && value == WebsocketState_t::WS_Offline)
    {
        if (connect())
        {
            ESP_LOGI(WS_TAG, "Wifi is connected! Auto connect OK!");
        }
        else
        {
            ESP_LOGW(WS_TAG, "Auto connect ERROR!");
        }
    }
    else if (wifiState == WifiState_t::Wifi_Disconnected || wifiState == WifiState_t::Wifi_Off)
    {
        //just make reconnect counter = 0
        reconnect_counter_ = 0;
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
    token_request_pending = true;
    pending_token_request_id = requestId;
    StaticJsonDocument<256> requestJson;
    requestJson["requestId"] = requestId;
    auto accessRequest = requestJson.createNestedObject("accessRequest");
    accessRequest["clientId"] = clientId;
    if (device_name_ == NULL)
    {
        accessRequest["description"] = "TWatchSK";
    }
    else
    {
        accessRequest["description"] = device_name_;
    }
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

void SignalKSocket::update_subscriptions(bool force)
{
    if (value == WebsocketState_t::WS_Connected && !token_request_pending)
    {
        bool is_lp = xEventGroupGetBits(g_app_state) & G_APP_STATE_LOW_POWER;
        if (force || is_lp || (low_power_subscriptions_ == true && !is_lp))
        {
            low_power_subscriptions_ = is_lp;
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
                //ESP_LOGI(WS_TAG, "Subscription: %s", subscriptionMessage.c_str());
                esp_websocket_client_send_text(websocket, subscriptionMessage.c_str(), subscriptionMessage.length(), portMAX_DELAY);
            }
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
    for (auto it = activeNotifications.begin(); it != activeNotifications.end(); it++)
    {
        if (it.base()->equals(path))
        {
            activeNotifications.erase(it);
            break;
        }
    }
}

///This is kind of HACK to control period 10 seconds ping of websocket - we need to avoid it to save some battery
void update_ws_timeout(esp_websocket_client_info *client)
{
    //ESP_LOGI(WS_TAG, "Ping timeout %d ms", (int)((esp_timer_get_time() / 1000) - client->ping_tick_ms));
    client->ping_tick_ms = esp_timer_get_time() / 1000;
}

void SignalKSocket::send_status_message()
{
    StaticJsonDocument<512> statusJson;
    char buff[512];

    JsonArray updates = statusJson.createNestedArray("updates");
    JsonObject current = updates.createNestedObject();
    JsonObject source = current.createNestedObject("source");
    source["label"] = device_name_;
    JsonArray values = current.createNestedArray("values");

    JsonObject battery = values.createNestedObject();
    sprintf(buff, "%s.battery", device_name_);
    battery["path"] = buff;
    battery["value"] = TTGOClass::getWatch()->power->getBattPercentage();

    JsonObject uptime = values.createNestedObject();
    sprintf(buff, "%s.uptime", device_name_);
    uptime["path"] = buff;

    int32_t elapsed_seconds = esp_timer_get_time() / 1000000;
    int hours = elapsed_seconds/3600;
	elapsed_seconds = elapsed_seconds%3600;
	int minutes = elapsed_seconds/60;
	elapsed_seconds = elapsed_seconds%60;
	int seconds = elapsed_seconds;

    sprintf(buff, "%d:%.2d:%.2d", hours, minutes, seconds);
    uptime["value"] = buff;

    JsonObject temp = values.createNestedObject();
    sprintf(buff, "%s.temperature", device_name_);
    temp["path"] = buff;
    temp["value"] =  (273.15f + TTGOClass::getWatch()->power->getTemp());

    if (serializeJson(statusJson, buff))
    {
        ESP_LOGI(WS_TAG, "Status json: %s", buff);
        esp_websocket_client_send_text(websocket, buff, strlen(buff), portMAX_DELAY);
    }
}

void SignalKSocket::handle_power_event(PowerCode_t code, uint32_t arg)
{
    if (code == PowerCode_t::POWER_ENTER_LOW_POWER && !low_power_subscriptions_)
    {
        update_subscriptions();
    }
    else if (code == PowerCode_t::POWER_LOW_TICK)
    {
        //we need to avoid client ping for now
        if (value == WebsocketState_t::WS_Connected)
        {
            update_ws_timeout((esp_websocket_client_info *)websocket);
            //send status message every minute in low power mode
            if (arg % 60 == 0)
            {
                send_status_message();
            }
        }
    }
}

bool SignalKSocket::reconnect()
{
    auto ret = false;

    if (websocket_initialized)
    {
        if (esp_websocket_client_is_connected(websocket))
        {
            disconnect();
        }

        ret = connect();
    }
    else
    {
        ret = connect();
    }

    return ret;
}

void SignalKSocket::clear_token()
{
    token = "";
    save();
}

bool SignalKSocket::send_put_request(JsonObject& request)
{
    char buff[1024];

    if (serializeJson(request, buff))
    {
        ESP_LOGI(WS_TAG, "Sending put json(len=%d): %s", strlen(buff), buff);
        esp_websocket_client_send_text(websocket, buff, strlen(buff), portMAX_DELAY);
        return true;
    }
    else
    {
        return false;
    }
}