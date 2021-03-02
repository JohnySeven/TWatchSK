#pragma once
#include "settings_view.h"
#include "localization.h"
#include "networking/signalk_socket.h"
#include "ui/keyboard.h"
#include "loader.h"
#include "ui_ticker.h"
#include "mdns.h"
#include "system/async_dispatcher.h"
#include "system/events.h"

class SignalKSettings : public SettingsView
{
public:
    SignalKSettings(SignalKSocket *socket) : SettingsView(LOC_SIGNALK_SETTINGS)
    {
        sk_socket_ = socket;
    }

protected:
    virtual void show_internal(lv_obj_t *parent) override
    {
        //enable layout into column on left side of the screen
        lv_cont_set_layout(parent, LV_LAYOUT_OFF);
        //show status label - connected/connecting/authorization pending/disconnected
        status_label_ = lv_label_create(parent, NULL);
        lv_obj_set_pos(status_label_, 10, 10);
        //shows server name and version (if connected)
        server_label_ = lv_label_create(parent, NULL);
        lv_obj_align(server_label_, status_label_, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
        //add address and port changing buttons - we will use keyboard for input!
        server_address_button_ = lv_btn_create(parent, NULL);
        server_address_button_->user_data = this;
        lv_obj_set_event_cb(server_address_button_, change_server_address_event);
        lv_obj_align(server_address_button_, server_label_, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
        server_address_label_ = lv_label_create(server_address_button_, NULL);
        //server port
        server_port_button_ = lv_btn_create(parent, NULL);
        server_port_label_ = lv_label_create(server_port_button_, NULL);
        lv_obj_set_width(server_port_button_, 50);
        lv_obj_set_event_cb(server_port_button_, change_server_port_event);
        server_port_button_->user_data = this;

        //find sk with mDNS service
        find_button_ = lv_btn_create(parent, NULL);
        lv_obj_align(find_button_, server_address_button_, LV_ALIGN_OUT_BOTTOM_LEFT, 0, -10);
        find_button_label_ = lv_label_create(find_button_, NULL);
        lv_label_set_text(find_button_label_, LOC_SIGNALK_FIND_SERVER);
        lv_obj_align(find_button_, parent, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
        //lv_obj_set_hidden(find_button_, true);
        find_button_->user_data = this;
        lv_obj_set_event_cb(find_button_, __scan_event);
        //update current configuration to class variables
        server_address_ = sk_socket_->get_server_address();
        server_port_ = sk_socket_->get_server_port();
        //update current status
        update_sk_info();
        update_server_info();

        status_update_ticker_ = new UITicker(1000, [this]() {
            this->update_sk_info();
        });
    }

    virtual bool hide_internal() override
    {
        delete status_update_ticker_;
        status_update_ticker_ = NULL;
        if (signalk_changed_)
        {
            save_signalk_settings();
        }
        return true;
    }

    void update_server_info()
    {
        if (server_address_ == "")
        {
            lv_label_set_text_fmt(server_address_label_, LOC_SIGNALK_ADDRESS_EMPTY);
        }
        else
        {
            lv_label_set_text(server_address_label_, server_address_.c_str());
        }

        lv_label_set_text_fmt(server_port_label_, "%d", server_port_);
        lv_obj_align(server_port_button_, server_address_button_, LV_ALIGN_OUT_RIGHT_MID, 4, 0);
    }

    void set_server(const char *server)
    {
        server_address_ = String(server);
        signalk_changed_ = true;
        update_server_info();
    }

    void set_port(int port)
    {
        server_port_ = port;
        signalk_changed_ = true;
        update_server_info();
    }

private:
    SignalKSocket *sk_socket_;
    lv_obj_t *status_label_;
    lv_obj_t *server_label_;
    lv_obj_t *find_button_;
    lv_obj_t *find_button_label_;
    lv_obj_t *server_address_button_;
    lv_obj_t *server_address_label_;
    lv_obj_t *server_port_button_;
    lv_obj_t *server_port_label_;
    bool server_search_running_ = false;
    bool server_search_completed_ = false;
    Loader *search_loader_ = NULL;
    UITicker *status_update_ticker_;
    String server_address_;
    int server_port_ = 3000;
    bool searching_sk_ = false;
    bool signalk_changed_ = false;

    static void change_server_address_event(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_CLICKED)
        {
            auto *settings = (SignalKSettings *)obj->user_data;
            auto keyboard = new Keyboard(LOC_SIGNALK_INPUT_ADDRESS, KeyboardType_t::Normal);
            keyboard->on_close([keyboard, settings]() {
                if (keyboard->is_success())
                {
                    settings->set_server(keyboard->get_text());
                }
            });
            keyboard->show(lv_scr_act());
        }
    }

    static void change_server_port_event(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_CLICKED)
        {
            auto *settings = (SignalKSettings *)obj->user_data;
            auto keyboard = new Keyboard(LOC_SIGNALK_INPUT_PORT, KeyboardType_t::Number, 5);
            keyboard->on_close([keyboard, settings]() {
                if (keyboard->is_success())
                {
                    int port = atoi(keyboard->get_text());
                    if (port > 0 && port < 65536)
                    {
                        settings->set_port(port);
                    }
                }
            });
            keyboard->show(lv_scr_act());
        }
    }

    static void __scan_event(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_CLICKED)
        {
            auto settings = ((SignalKSettings *)obj->user_data);
            settings->find_sk_server();
        }
    }

    void find_sk_server()
    {
        ESP_LOGI(SETTINGS_TAG, "Starting SK mDNS search...");
        if (!server_search_running_)
        {
            server_search_completed_ = false;
            server_search_running_ = true;
            search_loader_ = new Loader(LOC_SIGNALK_FINDING_SERVER);
            twatchsk::run_async("mDNS search", [this]()
            {
                const char *service_name = "signalk-ws";
                const char *service_proto = "tcp";
                mdns_result_t *results = NULL;
                if (mdns_init() == ESP_OK)
                {
                    ESP_LOGI(SETTINGS_TAG, "Query PTR: %s.%s.local", service_name, service_proto);
                    esp_err_t err = mdns_query_ptr(service_name, service_proto, 5000, 5, &results);
                    if (err == ESP_OK)
                    {
                        if (results != NULL)
                        {
                            mdns_result_t *r = results;

                            while (r)
                            {
                                ESP_LOGI(SETTINGS_TAG, "mDNS search result %s:%d", r->hostname, r->port);
                                server_address_ = String(r->hostname);
                                server_port_ = r->port;
                                signalk_changed_ = true;
                                break;
                            }
                        }
                        else
                        {
                            ESP_LOGW(SETTINGS_TAG, "No results found!");
                            post_gui_warning(String(LOC_SIGNALK_MDNS_NOT_FOUND));
                        }
                    }
                    else
                    {
                        ESP_LOGE(SETTINGS_TAG, "mDNS SK query failed with %d", (int)err);
                        post_gui_warning(String(LOC_SIGNALK_MDNS_ERORR));
                    }

                    mdns_free();
                }

                server_search_completed_ = true;
            });
        }
    }

    void stop_find_sk_server()
    {
        if (server_search_running_)
        {
            delete search_loader_;
            search_loader_ = NULL;
            server_search_running_ = false;
        }
    }

    void update_sk_info()
    {
        if (!server_search_running_)
        {
            auto socketStatus = sk_socket_->get_state();

            if (socketStatus == WebsocketState_t::WS_Connected)
            {
                if (!sk_socket_->get_token_request_pending())
                {
                    lv_label_set_text_fmt(status_label_, LOC_SIGNALK_CONNECTED_FMT, sk_socket_->get_handled_delta_count());
                }
                else
                {
                    lv_label_set_text(status_label_, LOC_SIGNALK_TOKEN_PENDING);
                }

                lv_label_set_text_fmt(server_label_, LOC_SIGNALK_SERVER_INFO, sk_socket_->get_server_name().c_str(), sk_socket_->get_server_version().c_str());

                //lv_obj_set_hidden(find_button_, true);
            }
            else if (socketStatus == WebsocketState_t::WS_Offline)
            {
                lv_obj_set_hidden(find_button_, false);
                lv_label_set_text(status_label_, LOC_SIGNALK_DISCONNECTED);
                lv_label_set_text_fmt(server_label_, LOC_SIGNALK_SERVER_INFO, "--", "--");
            }
            else if (socketStatus == WebsocketState_t::WS_Connecting)
            {
                lv_label_set_text(status_label_, LOC_SIGNALK_CONNECTING);
                lv_label_set_text_fmt(server_label_, LOC_SIGNALK_SERVER_INFO, "--", "--");
            }

            ESP_LOGI(SETTINGS_TAG, "Status update %d", (int)socketStatus);
        }
        else if (server_search_completed_)
        {
            ESP_LOGI(SETTINGS_TAG, "mDNS search is complete!");

            stop_find_sk_server();
        }
    }

    void save_signalk_settings()
    {
        ESP_LOGI(SETTINGS_TAG, "Saving SignalK settings (server=%s,port=%d)...", server_address_.c_str(), server_port_);
        sk_socket_->set_server(server_address_, server_port_);
        sk_socket_->save();
        if (sk_socket_->get_state() == WebsocketState_t::WS_Connected)
        {
            ESP_LOGI(SETTINGS_TAG, "SK websocket will be reconnected.");
            auto result = sk_socket_->reconnect();
            ESP_LOGI(SETTINGS_TAG, "SK websocket reconnect result=%d.", result);
        }
        //TODO:
    }
};