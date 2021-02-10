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
        wifi_manager = wifi;
    }

protected:
    virtual void show_internal(lv_obj_t *parent) override
    {
        lv_cont_set_layout(parent, LV_LAYOUT_COLUMN_LEFT);
        enableSwitch = lv_switch_create(topBar, NULL);
        lv_obj_align(enableSwitch, topBar, LV_ALIGN_IN_RIGHT_MID, -6, 0);
        enableSwitch->user_data = this;

        if (wifi_manager->get_status() == WifiState_t::Wifi_Off)
        {
            lv_switch_off(enableSwitch, false);
        }
        else
        {
            lv_switch_on(enableSwitch, false);
        }

        lv_obj_set_event_cb(enableSwitch, __enable_switch_event);

        status = lv_label_create(parent, NULL);

        static lv_style_t buttonStyle;
        lv_style_init(&buttonStyle);
        lv_style_set_radius(&buttonStyle, LV_STATE_DEFAULT, 10);

        scanButton = lv_btn_create(parent, NULL);
        lv_obj_add_style(scanButton, LV_OBJ_PART_MAIN, &buttonStyle);
        scanButtonLabel = lv_label_create(scanButton, NULL);
        lv_label_set_text(scanButtonLabel, LOC_WIFI_SCAN_LABEL);
        lv_obj_align(scanButton, parent, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
        scanButton->user_data = this;
        lv_obj_set_event_cb(scanButton, __scan_event);

        update_wifi_info();

        statusUpdateTicker = new UITicker(1000, [this]() {
            if (!this->scanning_wifi)
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
        delete statusUpdateTicker;
        statusUpdateTicker = NULL;
        save_wifi_settings();
        return true;
    }

    void set_ssid(const char *ssid)
    {
        strcpy(this->selectedAp, ssid);
    }

    void set_password(const char *password)
    {
        strcpy(this->password, password);
        this->wifi_changed = true;

        wifi_manager->setup(String(selectedAp), String(password));
    }

private:
    WifiManager *wifi_manager;
    lv_obj_t *enableSwitch;
    lv_obj_t *status;
    lv_obj_t *scanButton;
    lv_obj_t *scanButtonLabel;
    Loader *wifiScanLoader;
    UITicker *statusUpdateTicker;
    char selectedAp[64];
    char password[64];
    bool scanning_wifi = false;
    bool wifi_changed = false;

    static void __enable_switch_event(lv_obj_t *obj, lv_event_t event)
    {
        if(event == LV_EVENT_VALUE_CHANGED)
        {
            ((WifiSettings*)obj->user_data)->enable_switch_updated();
        }
    }

    void enable_switch_updated()
    {
        if(lv_switch_get_state(enableSwitch))
        {
            wifi_manager->on();
        }
        else
        {
            wifi_manager->off();
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
        if (wifi_manager->scan_wifi())
        {
            wifiScanLoader = new Loader(LOC_WIFI_SCANING_PROGRESS);
            scanning_wifi = true;
        }
    }

    void scan_wifi_check()
    {
        if (wifi_manager->is_scan_complete())
        {
            delete wifiScanLoader;
            wifiScanLoader = NULL;
            scanning_wifi = false;
            ESP_LOGI(SETTINGS_TAG, "Scan completed with %d found WiFi APs", wifi_manager->found_wifi_count());

            auto wifiList = new WifiList();
            wifiList->show(lv_scr_act());

            for (int i = 0; i < wifi_manager->found_wifi_count(); i++)
            {
                auto ap = wifi_manager->get_found_wifi(i);
                wifiList->add_ssid((char *)ap.ssid);
            }

            wifiList->on_close([this, wifiList]() {
                auto ssid = wifiList->selected_ssid();

                if (ssid != nullptr)
                {
                    ESP_LOGI(SETTINGS_TAG, "User has selected %s SSID", ssid);
                    this->set_ssid(ssid);
                    delete wifiList;
                    auto passwordPicker = new Keyboard(LOC_WIFI_PASSWORD_PROMPT);
                    passwordPicker->on_close([this, passwordPicker]() {
                        if (passwordPicker->is_success())
                        {
                            ESP_LOGI(SETTINGS_TAG, "User password input.");
                            this->set_password(passwordPicker->get_text());
                        }
                        else
                        {
                            ESP_LOGI(SETTINGS_TAG, "User canceled password input.");
                        }
                    });

                    passwordPicker->show(lv_scr_act());
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
        auto wifiStatus = wifi_manager->get_status();

        if (wifiStatus == WifiState_t::Wifi_Connected)
        {
            lv_label_set_text_fmt(status, LOC_WIFI_CONNECTED_STATUS_FMT, wifi_manager->get_ip().c_str());
        }
        else if (wifiStatus == WifiState_t::Wifi_Disconnected)
        {
            lv_label_set_text(status, LOC_WIFI_DISCONNECTED);
        }
        else if (wifiStatus == WifiState_t::Wifi_Connecting)
        {
            lv_label_set_text(status, LOC_WIFI_CONNECTING);
        }
        else if (wifiStatus == WifiState_t::Wifi_Off)
        {
            lv_label_set_text(status, LOC_WIFI_OFF);
        }

        ESP_LOGI("WIFI", "Status update %d", wifiStatus);
    }

    void save_wifi_settings()
    {
        ESP_LOGI(SETTINGS_TAG, "Saving WiFi settings...");
        wifi_manager->save();
        ESP_LOGI(SETTINGS_TAG, "WiFi settings saved!");
    }
};