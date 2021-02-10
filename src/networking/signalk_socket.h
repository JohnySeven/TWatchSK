#pragma once
#include "ArduinoJson.h"
#include "esp_websocket_client.h"
#include "vector"
#include "functional"
#include "map"
#include "system/configurable.h"
#include "system/systemobject.h"
#include "system/observable.h"
#include "hardware/Wifi.h"
#include "networking/signalk_subscription.h"
#include "hardware/Hardware.h"

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
        bool reconnect();
        void clear_token();
        void parse_data(int length, const char*data);
        esp_websocket_client_handle_t get_ws() { return websocket; }
        void update_status(WebsocketState_t status)
        {
            Observable<WebsocketState_t>::emit(status);
        }
        WebsocketState_t get_state() { return value; }
        void notify_change(const WifiState_t &wifi) override;
        String get_server_name() { return serverName; }
        String get_server_version() { return serverVersion; }
        String get_server_address() { return server;}
        int get_server_port() { return port; }
        bool get_token_request_pending() { return token_request_pending; }
        bool get_sync_time_with_server() { return sync_time_with_server; }
        void set_sync_time_with_server(bool enabled) { sync_time_with_server = enabled; }
        uint get_handled_delta_count() { return delta_counter; }
        SignalKSubscription* add_subscription(String path, uint period, bool is_low_power);
        void update_subscriptions();
        ///This is intended to be wired with Hardware class power events
        void handle_power_event(PowerCode_t code, uint32_t arg);
        ///Updates server configuration (address and port)
        void set_server(String server_address, int port)
        {
            server = server_address;
            this->port = port;
        }
    private:
        static void ws_event_handler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data);
        String server = "";
        int port = 0;
        uint delta_counter = 0;
        bool sync_time_with_server = false;
        String token = "";
        String clientId = "";
        String serverName = "";
        String serverVersion = "";
        bool token_request_pending = false;
        String pending_token_request_id = "";
        esp_websocket_client_handle_t websocket;
        bool websocket_initialized = false;
        std::map<String,SignalKSubscription*> subscriptions;
        std::vector<String> activeNotifications;
        WifiManager*wifi;
        void load_config_from_file(const JsonObject &json) override;
        void save_config_to_file(JsonObject &json) override;
        void send_token_permission();
        void send_json(const JsonObject &json);
        bool is_notification_active(String path);
        void remove_active_notification(String path);
};