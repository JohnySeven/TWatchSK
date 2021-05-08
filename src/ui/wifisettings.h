#pragma once
#include "settings_view.h"
#include "localization.h"
#include "hardware/Wifi.h"
#include "loader.h"
#include "wifilist.h"
#include "ui_ticker.h"
#include "system/async_dispatcher.h"

enum WifiSettingsState_t
{
    WiS_Normal,          //showing status
    WiS_WifiScan,        //showing loader with scanning wifi
    WiS_ConnectingToWifi //connecting to wifi network
};

class WifiSettings : public SettingsView
{
public:
    WifiSettings(WifiManager *wifi) : SettingsView(LOC_WIFI_SETTINGS)
    {
        wifi_manager_ = wifi;
    }

protected:
    virtual void show_internal(lv_obj_t *parent) override
    {
        lv_cont_set_layout(parent, LV_LAYOUT_OFF);
        //Create on / off switch for WiFi control
        enable_switch_ = lv_switch_create(topBar, NULL);
        lv_obj_align(enable_switch_, topBar, LV_ALIGN_IN_RIGHT_MID, -6, 0);
        enable_switch_->user_data = this;

        if (wifi_manager_->get_status() == WifiState_t::Wifi_Off)
        {
            lv_switch_off(enable_switch_, LV_ANIM_OFF);
        }
        else
        {
            lv_switch_on(enable_switch_, LV_ANIM_OFF);
        }

        lv_obj_set_event_cb(enable_switch_, __enable_switch_event);

        //show configured wifi name
        wifi_name_ = lv_label_create(parent, NULL);
        lv_label_set_text_fmt(wifi_name_, LOC_WIFI_CONFIG_SSID_FMT, wifi_manager_->get_configured_ssid().c_str());
        lv_obj_set_pos(wifi_name_, spacing, spacing);
        
        //show wifi current status
        status_ = lv_label_create(parent, NULL);
        lv_obj_align(status_, wifi_name_, LV_ALIGN_OUT_BOTTOM_LEFT, 0, spacing);
        
        //show current IP address (if any)
        wifi_ip_ = lv_label_create(parent, NULL);
        lv_obj_align(wifi_ip_, status_, LV_ALIGN_OUT_BOTTOM_LEFT, 0, spacing);
        
        static lv_style_t buttonStyle;
        lv_style_init(&buttonStyle);
        lv_style_set_radius(&buttonStyle, LV_STATE_DEFAULT, 10);
        
        //connect button - visible only if not connected - just guide for user to enable wifi
        connect_button_ = lv_btn_create(parent, NULL);
        lv_obj_add_style(connect_button_, LV_OBJ_PART_MAIN, &buttonStyle);
        lv_obj_align_y(connect_button_, wifi_ip_, LV_ALIGN_OUT_BOTTOM_LEFT, spacing * 2); //first align the top with label above + 4pix
        lv_obj_align_x(connect_button_, parent, LV_ALIGN_IN_BOTTOM_MID, 0);               //then center the button in parent
        auto connectLabel = lv_label_create(connect_button_, NULL);
        lv_label_set_text(connectLabel, LOC_WIFI_CONNECT);
        connect_button_->user_data = this;
        lv_obj_set_event_cb(connect_button_, __connect_event);
        
        //scan button
        scan_button_ = lv_btn_create(parent, NULL);
        lv_obj_add_style(scan_button_, LV_OBJ_PART_MAIN, &buttonStyle);
        lv_obj_align(scan_button_, connect_button_, LV_ALIGN_OUT_BOTTOM_MID, 0, spacing);
        auto scanLabel = lv_label_create(scan_button_, NULL);
        lv_label_set_text(scanLabel, LOC_WIFI_SCAN_LABEL);
        scan_button_->user_data = this;
        lv_obj_set_event_cb(scan_button_, __scan_event);

        update_wifi_info(true);

        status_update_ticker_ = new UITicker(1000, [this]() {
            if (state_ == WifiSettingsState_t::WiS_Normal)
            {
                update_wifi_info();
            }
            else if (state_ == WifiSettingsState_t::WiS_WifiScan)
            {
                scan_wifi_check();
            }
            else if (state_ == WifiSettingsState_t::WiS_ConnectingToWifi)
            {
                wifi_connect_check();
            }
        });
    }

    virtual bool hide_internal() override
    {
        delete status_update_ticker_;
        status_update_ticker_ = NULL;
        if(wifi_changed_)
        {
            save_wifi_settings();
        }
        return true;
    }

    void set_ssid(const char *ssid)
    {
        strcpy(selected_ap_, ssid);
        lv_label_set_text_fmt(wifi_name_, LOC_WIFI_CONFIG_SSID_FMT, ssid);
    }

    void set_password_and_connect(const char *password)
    {
        if (state_ != WifiSettingsState_t::WiS_ConnectingToWifi) //JD: Workaround, something is wrong as this is called 2x, will fix that later 
                                                                 // BS: I think it was a bug with keyboards, and may be fixed now?
                                                                 // See changes to keyboard.h in https://github.com/JohnySeven/TWatchSK/commit/551d1be1fce5e36144c4c00131db384b8cce88ae#
        {
            strcpy(password_, password);
            wifi_changed_ = true;

            ESP_LOGI(SETTINGS_TAG, "Connecting to wifi %s", selected_ap_);

            wifi_manager_->setup(String(selected_ap_), String(password));
            wifi_manager_->connect();

            state_ = WifiSettingsState_t::WiS_ConnectingToWifi;
            loader_ = new Loader(LOC_WIFI_CONNECTING);
        }
    }

private:
    WifiManager *wifi_manager_;
    lv_obj_t *enable_switch_;
    lv_obj_t *wifi_name_;
    lv_obj_t *wifi_connection_;
    lv_obj_t *wifi_ip_;
    lv_obj_t *status_;
    lv_obj_t *connect_button_;
    lv_obj_t *scan_button_;
    Loader *loader_;
    UITicker *status_update_ticker_;
    char selected_ap_[64];
    char password_[64];
    bool wifi_changed_ = false;
    WifiSettingsState_t state_ = WifiSettingsState_t::WiS_Normal;

    static void __enable_switch_event(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_VALUE_CHANGED)
        {
            ((WifiSettings *)obj->user_data)->enable_switch_updated();
        }
    }

    void enable_switch_updated()
    {
        if (lv_switch_get_state(enable_switch_))
        {
            wifi_manager_->on();
        }
        else
        {
            wifi_manager_->off();
        }
    }

    static void __scan_event(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_CLICKED)
        {
            ((WifiSettings *)obj->user_data)->scan_wifi_list();
        }
    }

    void scan_wifi_list()
    {
        if (wifi_manager_->scan_wifi())
        {
            loader_ = new Loader(LOC_WIFI_SCANNING_PROGRESS);
            state_ = WifiSettingsState_t::WiS_WifiScan;
        }
    }

    void scan_wifi_check()
    {
        if (wifi_manager_->is_scan_complete())
        {
            delete loader_;
            loader_ = NULL;
            state_ = WifiSettingsState_t::WiS_Normal;
            ESP_LOGI(SETTINGS_TAG, "Scan completed with %d found WiFi APs", wifi_manager_->found_wifi_count());

            auto wifiList = new WifiList();
            wifiList->show(lv_scr_act());

            for (int i = 0; i < wifi_manager_->found_wifi_count(); i++)
            {
                auto ap = wifi_manager_->get_found_wifi(i);
                auto isknown = wifi_manager_->is_known_wifi(String((char *)ap.ssid));
                wifiList->add_ssid((char *)ap.ssid, isknown ? (void *)LV_SYMBOL_SAVE : (void *)LV_SYMBOL_WIFI);
            }

            wifiList->on_close([this, wifiList]() {
                auto ssid = wifiList->selected_ssid();

                if (ssid != nullptr)
                {
                    ESP_LOGI(SETTINGS_TAG, "User has selected %s SSID", ssid);
                    set_ssid(ssid);
                    delete wifiList;
                    String password;
                    if (wifi_manager_->get_known_wifi_password(ssid, password))
                    {
                        set_password_and_connect(password.c_str());
                    }
                    else
                    {
                        auto passwordPicker = new Keyboard(LOC_WIFI_PASSWORD_PROMPT);
                        passwordPicker->on_close([this, passwordPicker]() {
                            if (passwordPicker->is_success())
                            {
                                ESP_LOGI(SETTINGS_TAG, "User password input.");
                                set_password_and_connect(passwordPicker->get_text());
                            }
                            else
                            {
                                ESP_LOGI(SETTINGS_TAG, "User canceled password input.");
                            }
                            delete passwordPicker;
                        });

                        passwordPicker->show(lv_scr_act());
                    }
                }
            });
        }
    }

    void wifi_connect_check()
    {
        auto wifi_status = wifi_manager_->get_status();
        ESP_LOGI(SETTINGS_TAG, "Connecting to Wifi status=%d", (int)wifi_status);

        if (wifi_status != WifiState_t::Wifi_Connecting)
        {
            delete loader_;
            loader_ = NULL;
            state_ = WifiSettingsState_t::WiS_Normal;

            if (wifi_status == WifiState_t::Wifi_Connected)
            {
                show_message(LOC_WIFI_CONNECT_SUCCESS);
            }
            else
            {
                //erase wifi settings
                wifi_manager_->setup("", "");
                set_ssid("");
                strcpy(password_, "");
                update_wifi_info(true);
            }
        }
    }

    void update_wifi_info(bool force = false)
    {
        static WifiState_t lastState = WifiState_t::Wifi_Off;
        auto wifiStatus = wifi_manager_->get_status();
        auto isConfigured = wifi_manager_->is_configured();

        if (wifiStatus == WifiState_t::Wifi_Connected) //update IP and RSSI regardless of the other values
        {
            lv_label_set_text_fmt(wifi_ip_, LOC_WIFI_IP_RSSI_FMT, wifi_manager_->get_ip().c_str(), wifi_manager_->get_wifi_rssi());
        }

        if (lastState != wifiStatus || force) //just update the UI if status has been changed or force = true
        {
            ESP_LOGI(SETTINGS_TAG, "Wifi status update %d=>%d (force:%d)", lastState, wifiStatus, force);

            lastState = wifiStatus;
            if (wifiStatus == WifiState_t::Wifi_Connected)
            {
                lv_label_set_text_fmt(status_, LOC_WIFI_CONNECTED);
                lv_obj_set_hidden(connect_button_, true);
                lv_obj_set_hidden(scan_button_, false);
                update_silently_enable_switch(true);
            }
            else if (wifiStatus == WifiState_t::Wifi_Disconnected)
            {
                lv_label_set_text(status_, LOC_WIFI_DISCONNECTED);
                lv_label_set_text_fmt(wifi_ip_, LOC_WIFI_IP_RSSI_FMT, "--", 0);
                lv_obj_set_hidden(connect_button_, !isConfigured); //if wifi isn't configured connect will not happen
                lv_obj_set_hidden(scan_button_, false);
                update_silently_enable_switch(true);
            }
            else if (wifiStatus == WifiState_t::Wifi_Connecting)
            {
                lv_label_set_text(status_, LOC_WIFI_CONNECTING);
                lv_label_set_text_fmt(wifi_ip_, LOC_WIFI_IP_RSSI_FMT, "--", 0);
                lv_obj_set_hidden(connect_button_, true);
                lv_obj_set_hidden(scan_button_, true); //if Wifi is connecting to AP it's not allowed to do scan at all
                update_silently_enable_switch(true);
            }
            else if (wifiStatus == WifiState_t::Wifi_Off)
            {
                if (isConfigured)
                {
                    lv_label_set_text(status_, LOC_WIFI_OFF);
                    lv_label_set_text_fmt(wifi_ip_, LOC_WIFI_IP_RSSI_FMT, "--", 0);
                    lv_obj_set_hidden(connect_button_, false); //if wifi isn't configured connect will not happen
                    lv_obj_set_hidden(scan_button_, false);
                    update_silently_enable_switch(false);
                }
                else
                {
                    lv_label_set_text(status_, LOC_WIFI_NO_CONFIG);
                    lv_label_set_text(wifi_ip_, LOC_WIFI_PLEASE_CONFIGURE);
                    lv_obj_set_hidden(connect_button_, false); //if wifi isn't configured connect will not happen
                    lv_obj_set_hidden(scan_button_, true);
                    update_silently_enable_switch(false);
                }
            }

            ESP_LOGI("WIFI", "Status update %d", wifiStatus);
        }
    }

    void save_wifi_settings()
    {
        twatchsk::run_async("Wifi save", [this]() {
            ESP_LOGI(SETTINGS_TAG, "Saving WiFi settings...");
            this->wifi_manager_->save();
            ESP_LOGI(SETTINGS_TAG, "WiFi settings saved!");
        });
    }

    /**
     * Updates Enable switch to value, for update time it disables callback (will not trigger on or off on wifi manager)
     **/
    void update_silently_enable_switch(bool value)
    {
        if (lv_switch_get_state(enable_switch_) != value)
        {
            //we need to bypass enable switch callback
            lv_obj_set_event_cb(enable_switch_, NULL);
            if (value)
            {
                lv_switch_on(enable_switch_, LV_ANIM_ON);
            }
            else
            {
                lv_switch_off(enable_switch_, LV_ANIM_OFF);
            }
            lv_obj_set_event_cb(enable_switch_, __enable_switch_event);
        }
    }

    static void __connect_event(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_CLICKED)
        {
            auto settings = ((WifiSettings *)obj->user_data);
            if (settings->wifi_manager_->is_configured())
            {
                settings->wifi_manager_->connect();
            }
            else
            {
                settings->scan_wifi_list();
            }
        }
    }
};