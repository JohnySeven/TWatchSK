#pragma once
#include "settings_view.h"
#include "localization.h"
#include "networking/signalk_socket.h"
#include "loader.h"
#include "ui_ticker.h"

class SignalKSettings : public SettingsView
{
public:
    SignalKSettings(SignalKSocket *socket) : SettingsView(LOC_SIGNALK_SETTINGS)
    {
        sk_socket = socket;
    }

protected:
    virtual void show_internal(lv_obj_t *parent) override
    {
        static lv_style_t buttonStyle;
        lv_style_init(&buttonStyle);
        lv_style_set_radius(&buttonStyle, LV_STATE_DEFAULT, 10);
        
        lv_cont_set_layout(parent, LV_LAYOUT_COLUMN_LEFT);
        configuredServer = lv_label_create(parent, NULL);
        status = lv_label_create(parent, NULL);
        scanButton = lv_btn_create(parent, NULL);
        lv_obj_add_style(scanButton, LV_OBJ_PART_MAIN, &buttonStyle);
        scanButtonLabel = lv_label_create(scanButton, NULL);
        lv_label_set_text(scanButtonLabel, LOC_SIGNALK_FIND_SERVER);
        lv_obj_align(scanButton, parent, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
        lv_obj_set_hidden(scanButton, true);
        scanButton->user_data = this;
        lv_obj_set_event_cb(scanButton, __scan_event);
        update_sk_info();
        update_server_info();

        statusUpdateTicker = new UITicker(1000, [this]() {
            this->update_sk_info();
        });
    }

    virtual bool hide_internal() override
    {
        delete statusUpdateTicker;
        statusUpdateTicker = NULL;
        if(signalk_changed)
        {
            save_signalk_settings();
        }
        return true;
    }

    void update_server_info()
    {
        lv_label_set_text_fmt(configuredServer, LOC_SIGNALK_SERVER_ADDRESS, sk_socket->get_server_address(), sk_socket->get_server_port());
    }

    void set_server(const char *server)
    {
        strcpy(this->server_address, server);
        this->signalk_changed = true;
    }

    void set_port(int port)
    {
        this->server_port = port;
        this->signalk_changed = true;
    }

private:
    SignalKSocket *sk_socket;
    lv_obj_t *configuredServer;    
    lv_obj_t *status;
    lv_obj_t *scanButton;
    lv_obj_t *scanButtonLabel;
    Loader *ScanLoader;
    UITicker *statusUpdateTicker;
    char server_address[128];
    int server_port = 0;
    bool searching_sk = false;
    bool signalk_changed = false;

    static void __scan_event(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_CLICKED)
        {

        }
    }

    void scan_sk_check()
    {
    }

    void update_sk_info()
    {
        auto socketStatus = sk_socket->get_state();

        if (socketStatus == WebsocketState_t::WS_Connected)
        {
            if (!sk_socket->get_token_request_pending())
            {
                lv_label_set_text_fmt(status, LOC_SIGNALK_CONNECTED, sk_socket->get_server_name().c_str());
            }
            else
            {
                lv_label_set_text(status, LOC_SIGNALK_TOKEN_PENDING);
            }
        }
        else if (socketStatus == WebsocketState_t::WS_Offline)
        {
            lv_obj_set_hidden(scanButton, false);
            lv_label_set_text(status, LOC_SIGNALK_DISCONNECTED);
        }
        else if (socketStatus == WebsocketState_t::WS_Connecting)
        {
            lv_label_set_text(status, LOC_SIGNALK_CONNECTING);
        }

        ESP_LOGI(SETTINGS_TAG, "Status update %d", (int)socketStatus);
    }

    void save_signalk_settings()
    {
        ESP_LOGI(SETTINGS_TAG, "Saving SignalK settings...");
        //TODO:
    }
};