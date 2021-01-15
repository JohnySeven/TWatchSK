#pragma once
#include "esp_timer.h"
#include "settings_view.h"
#include "localization.h"
#include "ui_ticker.h"
#include "system/system_data.h"

/**
 * @brief Shows version, author, uptime, and screen timeout (which you can set here).
 * Future: wake-ups, number of handled SK deltas, more?
 **/

class WatchInfo : public SettingsView
{
public:
    WatchInfo(SystemData* system_data) : SettingsView(LOC_WATCH_INFO)
    {
        system_data_ = system_data;
    }

    void update_timeout(int timeout_seconds) // for when user changes the screen timeout value
    {
        timeout_ = timeout_seconds;
        timeout_changed_ = true;
        lv_label_set_text_fmt(timeoutLabel_, "%d", timeout_);
        ESP_LOGI(SETTINGS_TAG, "User set screen timeout to %d", timeout_);
    }

protected:
    virtual void show_internal(lv_obj_t *parent) override
    {
        lv_cont_set_layout(parent, LV_LAYOUT_COLUMN_LEFT);

        author_ = lv_label_create(parent, NULL);
        lv_label_set_text(author_, LOC_AUTHOR);

        version_ = lv_label_create(parent, NULL);
        lv_label_set_text(version_, LOC_WATCH_VERSION);

        uptime_ = lv_label_create(parent, NULL);
        update_uptime();
        
        uptimeTicker_ = new UITicker(1000, [this]() {
            this->update_uptime();
        });

        screenTimeoutLabel_ = lv_label_create(parent, NULL);
        lv_label_set_text(screenTimeoutLabel_, LOC_SCREEN_TIMEOUT);
        
        static lv_style_t buttonStyle;
        lv_style_init(&buttonStyle);
        lv_style_set_radius(&buttonStyle, LV_STATE_DEFAULT, 0);
        lv_style_set_radius(&buttonStyle, LV_STATE_PRESSED, 0);
        lv_style_set_radius(&buttonStyle, LV_STATE_DISABLED, 0);
        
        timeoutButton_ = lv_btn_create(parent, NULL);
        lv_obj_add_style(timeoutButton_, LV_OBJ_PART_MAIN, &buttonStyle);
        timeoutLabel_ = lv_label_create(timeoutButton_, NULL);
        lv_label_set_text_fmt(timeoutLabel_, "%d", timeout_);
        lv_obj_set_event_cb(timeoutButton_, timeout_button_callback);
        lv_obj_set_width(timeoutButton_, 50);

        timeoutButton_->user_data = this;
    }

    virtual bool hide_internal() override
    {
        if (timeout_changed_)
        {
            ESP_LOGI(SETTINGS_TAG, "Calling save_system_data()");
            save_system_data();
        }
        delete uptimeTicker_;
        uptimeTicker_ = NULL;
        return true;
    }

    void update_uptime()
    {
        int32_t elapsed_seconds = esp_timer_get_time() / 1000000;
        int hours = elapsed_seconds/3600;
	    elapsed_seconds = elapsed_seconds%3600;
	    int minutes = elapsed_seconds/60;
	    elapsed_seconds = elapsed_seconds%60;
	    int seconds = elapsed_seconds;

        lv_label_set_text_fmt(uptime_, LOC_UPTIME, hours, minutes, seconds);
    }

private:
    lv_obj_t* version_;
    lv_obj_t* author_;
    lv_obj_t* uptime_;
    UITicker* uptimeTicker_;
    lv_obj_t* screenTimeoutLabel_;
    lv_obj_t* timeoutButton_;
    lv_obj_t* timeoutLabel_;
    SystemData* system_data_;
    int timeout_ = 10;
    bool timeout_changed_ = false;

    static void timeout_button_callback(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_CLICKED)
        {
            WatchInfo* settings = (WatchInfo* )obj->user_data;
            auto keyboard = new Keyboard(LOC_INPUT_SCREEN_TIMEOUT, KeyboardType_t::Number, 2);
            keyboard->on_close([keyboard, settings]() {
                if (keyboard->is_success())
                {
                    const char *text = keyboard->get_text();

                    int timeout = atoi(text);
                    if (timeout >= 5 && timeout <= 120)
                    {
                        settings->update_timeout(timeout);
                    }
                }
            });
            keyboard->show(lv_scr_act());
        }
    }

    void save_system_data()
    {
            // THIS IS THE BUG: ANY ATTEMPT AT CALLING A FUNCTION FROM SYSTEM_DATA PUKES.
            ESP_LOGI(SETTINGS_TAG, "Calling set_screen_timeout()");
            system_data_->set_screen_timeout(timeout_);
            ESP_LOGI(SETTINGS_TAG, "Calling system_data->save()");
            system_data_->save(); // saves to SPIFFS

    }
};