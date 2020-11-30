#pragma once
#include "settings_view.h"
#include "localization.h"
#include "hardware/Wifi.h"
#include "loader.h"

class WifiSettings : public SettingsView
{
public:
    WifiSettings(WifiManager *wifi, std::function<void()> close_call_back) : SettingsView(LOC_WIFI_SETTINGS, close_call_back)
    {
        wifi_manager = wifi;
    }

protected:
    virtual void show_internal(lv_obj_t *parent) override
    {
        enableSwitch = lv_switch_create(topBar, NULL);
        lv_obj_align(enableSwitch, topBar, LV_ALIGN_IN_RIGHT_MID, 0, 0);
        lv_obj_set_event_cb(enableSwitch, __enable_switch_event);

        if (wifi_manager->get_status() == WifiState_t::Wifi_Off)
        {
            lv_switch_off(enableSwitch, false);
        }
        else
        {
            lv_switch_on(enableSwitch, false);
        }

        status = lv_label_create(parent, NULL);

        scanButton = lv_btn_create(parent, NULL);
        scanButtonLabel = lv_label_create(scanButton, NULL);
        lv_label_set_text(scanButtonLabel, LOC_WIFI_SCAN_LABEL);
        lv_obj_align(scanButton, parent, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
        scanButton->user_data = this;
        lv_obj_set_event_cb(scanButton, __scan_event);

        update_wifi_info();

        statusUpdateTicker = new Ticker();
        statusUpdateTicker->attach_ms<WifiSettings *>(1000, __update_ticker, this);
    }

    virtual bool hide_internal() override
    {
        statusUpdateTicker->detach();
        delete statusUpdateTicker;
        statusUpdateTicker = NULL;
        save_wifi_settings();
        return true;
    }

private:
    WifiManager *wifi_manager;
    lv_obj_t *enableSwitch;
    lv_obj_t *status;
    lv_obj_t *scanButton;
    lv_obj_t *scanButtonLabel;
    Loader *wifiScanLoader;
    Ticker *statusUpdateTicker;
    bool scanningWifi = false;
    int dummyCounter = 0;

    static void __update_ticker(WifiSettings *settings)
    {
        if (!settings->scanningWifi)
        {
            settings->update_wifi_info();
        }
        else
        {
            settings->scan_wifi_check();
        }
    }

    static void __enable_switch_event(lv_obj_t *obj, lv_event_t event)
    {
    }

    static void __scan_event(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_CLICKED)
        {
            ESP_LOGI("WIFI", "Scan event clicked.");
            ((WifiSettings *)obj->user_data)->scan_wifi_list();
        }
    }

    void scan_wifi_list()
    {
        //if (wifi_manager->scan_wifi())
        //{
            wifiScanLoader = new Loader(LOC_WIFI_SCANING_PROGRESS);
            dummyCounter = 3;
            scanningWifi = true;
        //}
    }

    void scan_wifi_check()
    {
        if (dummyCounter < 0)//wifi_manager->is_scan_complete())
        {
            delete wifiScanLoader;
            wifiScanLoader = NULL;
            scanningWifi = false;
            ESP_LOGI("WIFI", "Scan completed with %d found wifi APs", wifi_manager->found_wifi_count());
        }
        dummyCounter--;
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
        ESP_LOGI("Settings", "Saving wifi settings...");
    }
};