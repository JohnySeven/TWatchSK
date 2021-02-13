#pragma once
#include "settings_view.h"
#include "localization.h"
#include "hardware/Wifi.h"
#include "loader.h"
#include "wifilist.h"
#include "ui_ticker.h"

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
        lv_cont_set_layout(parent, LV_LAYOUT_COLUMN_LEFT);
        //Create on / off switch for WiFi control
        enable_switch_ = lv_switch_create(topBar, NULL);
        lv_obj_align(enable_switch_, topBar, LV_ALIGN_IN_RIGHT_MID, -6, 0);
        enable_switch_->user_data = this;

        if (wifi_manager_->get_status() == WifiState_t::Wifi_Off)
        {
            lv_switch_off(enable_switch_, false);
        }
        else
        {
            lv_switch_on(enable_switch_, false);
        }

        lv_obj_set_event_cb(enable_switch_, __enable_switch_event);

        //show configured wifi name
        wifi_name_ = lv_label_create(parent, NULL);
        lv_label_set_text_fmt(wifi_name_, LOC_WIFI_CONFIG_SSID_FMT, wifi_manager_->get_configured_ssid());
        //show wifi current status
        status_ = lv_label_create(parent, NULL);
        //show current IP address (if any)
        wifi_ip_ = lv_label_create(parent, NULL);
        //connect button - visible only if not connected - just guide for user to enable wifi
        connect_button_ = lv_btn_create(parent, NULL);
        auto connectLabel = lv_label_create(connect_button_, NULL);
        lv_label_set_text(connectLabel, LOC_WIFI_CONNECT);
        connect_button_->user_data = this;
        lv_obj_set_event_cb(connect_button_, __connect_event);
        //scan button
        scan_button_ = lv_btn_create(parent, NULL);
        auto scanLabel = lv_label_create(scan_button_, NULL);
        lv_label_set_text(scanLabel, LOC_WIFI_SCAN_LABEL);
        lv_obj_align(scan_button_, parent, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
        scan_button_->user_data = this;
        lv_obj_set_event_cb(scan_button_, __scan_event);

        update_wifi_info();

        status_update_ticker_ = new UITicker(1000, [this]() {
            if (!this->scanning_wifi_)
            {
                this->update_wifi_info();
            }
            else
            {
                this->scan_wifi_check();
            }
        });
    }

    virtual bool hide_internal() override
    {
        delete status_update_ticker_;
        status_update_ticker_ = NULL;
        save_wifi_settings();
        return true;
    }

    void set_ssid(const char *ssid)
    {
        strcpy(selected_ap_, ssid);
        lv_label_set_text_fmt(wifi_name_, LOC_WIFI_CONFIG_SSID_FMT, ssid);
    }

    void set_password(const char *password)
    {
        strcpy(password_, password);
        wifi_changed_ = true;

        wifi_manager_->setup(String(selected_ap_), String(password));
        wifi_manager_->connect();
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
    Loader *wifi_scan_loader_;
    UITicker *status_update_ticker_;
    char selected_ap_[64];
    char password_[64];
    bool scanning_wifi_ = false;
    bool wifi_changed_ = false;

    static void __enable_switch_event(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_VALUE_CHANGED)
        {
            ((WifiSettings *)obj->user_data)->enable_switch_updated();
        }
    }

    void enable_switch_updated()
    {
        ESP_LOGI(SETTINGS_TAG, "Wifi enable switch value changed!");
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
            wifi_scan_loader_ = new Loader(LOC_WIFI_SCANNING_PROGRESS);
            scanning_wifi_ = true;
        }
    }

    void scan_wifi_check()
    {
        if (wifi_manager_->is_scan_complete())
        {
            delete wifi_scan_loader_;
            wifi_scan_loader_ = NULL;
            scanning_wifi_ = false;
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
                        set_password(password.c_str());
                    }
                    else
                    {
                        auto passwordPicker = new Keyboard(LOC_WIFI_PASSWORD_PROMPT);
                        passwordPicker->on_close([this, passwordPicker]() {
                            if (passwordPicker->is_success())
                            {
                                ESP_LOGI(SETTINGS_TAG, "User password input.");
                                set_password(passwordPicker->get_text());
                            }
                            else
                            {
                                ESP_LOGI(SETTINGS_TAG, "User canceled password input.");
                            }
                        });

                        passwordPicker->show(lv_scr_act());
                    }
                }
                else
                {
                    delete wifiList;
                }
            });
        }
    }

    void update_wifi_info()
    {
        auto wifiStatus = wifi_manager_->get_status();

        if (wifiStatus == WifiState_t::Wifi_Connected)
        {
            lv_label_set_text_fmt(status_, LOC_WIFI_CONNECTED);
            lv_label_set_text_fmt(wifi_ip_, LOC_WIFI_IP_FMT, wifi_manager_->get_ip().c_str());
            lv_obj_set_hidden(this->connect_button_, true);
        }
        else if (wifiStatus == WifiState_t::Wifi_Disconnected)
        {
            lv_label_set_text(status_, LOC_WIFI_DISCONNECTED);
            lv_label_set_text_fmt(wifi_ip_, LOC_WIFI_IP_FMT, "--");
            lv_obj_set_hidden(this->connect_button_, false);
        }
        else if (wifiStatus == WifiState_t::Wifi_Connecting)
        {
            lv_label_set_text(status_, LOC_WIFI_CONNECTING);
            lv_label_set_text_fmt(wifi_ip_, LOC_WIFI_IP_FMT, "--");
            lv_obj_set_hidden(this->connect_button_, true);
        }
        else if (wifiStatus == WifiState_t::Wifi_Off)
        {
            lv_label_set_text(status_, LOC_WIFI_OFF);
            lv_label_set_text_fmt(wifi_ip_, LOC_WIFI_IP_FMT, "--");
            lv_obj_set_hidden(this->connect_button_, false);
        }

        ESP_LOGI("WIFI", "Status update %d", wifiStatus);
    }

    void save_wifi_settings()
    {
        ESP_LOGI(SETTINGS_TAG, "Saving WiFi settings...");
        wifi_manager_->save();
        ESP_LOGI(SETTINGS_TAG, "WiFi settings saved!");
    }

    void check_enabled_if_not()
    {
        if (!lv_switch_get_state(enable_switch_))
        {
            //we need to bypass enable switch callback
            lv_obj_set_event_cb(enable_switch_, NULL);
            lv_switch_on(enable_switch_, LV_ANIM_ON);
            lv_obj_set_event_cb(enable_switch_, __enable_switch_event);
        }
    }

    static void __connect_event(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_CLICKED)
        {
            auto settings = ((WifiSettings *)obj->user_data);
            settings->check_enabled_if_not();
            settings->wifi_manager_->connect();
        }
    }
};