#pragma once
#include "gui.h"
#include "esp_timer.h"
#include "settings_view.h"
#include "localization.h"
#include "ui_ticker.h"

/**
 * @brief Shows (and sets) watch name, version, author, and uptime, number of wake-ups.
 * Future: number of handled SK deltas, more?
 **/

class WatchInfo : public SettingsView
{
public:
    WatchInfo(Gui* gui) : SettingsView(LOC_WATCH_INFO)
    {
        gui_ = gui;
    }

    void set_watch_name(const char* new_name)
    {
        strcpy(watch_name_, new_name);
    }

    char* get_watch_name()
    {
        return watch_name_;
    }

protected:
    virtual void show_internal(lv_obj_t *parent) override
    {
        lv_cont_set_layout(parent, LV_LAYOUT_OFF);

        static lv_style_t buttonStyle;
        lv_style_init(&buttonStyle);
        lv_style_set_radius(&buttonStyle, LV_STATE_DEFAULT, 10);

        watchNameLabel_ = lv_label_create(parent, NULL);
        lv_obj_set_pos(watchNameLabel_, 4, 15);
        lv_label_set_text(watchNameLabel_, LOC_WATCH_NAME);
        watchNameButton_ = lv_btn_create(parent, NULL);
        watchNameButton_->user_data = this;
        lv_obj_align(watchNameButton_, watchNameLabel_, LV_ALIGN_OUT_RIGHT_MID, 5, 5);
        lv_obj_add_style(watchNameButton_, LV_OBJ_PART_MAIN, &buttonStyle);
        watchName_ = lv_label_create(watchNameButton_, NULL);
        lv_label_set_text(watchName_, watch_name_);
        lv_obj_set_event_cb(watchNameButton_, watch_name_button_callback);
        lv_obj_set_width(watchNameButton_, 160);
        lv_obj_set_height(watchNameButton_, 35);
        
        author_ = lv_label_create(parent, NULL);
        lv_obj_align(author_, watchNameLabel_, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
        lv_label_set_text(author_, LOC_AUTHOR);

        version_ = lv_label_create(parent, NULL);
        lv_obj_align(version_, author_, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
        lv_label_set_text(version_, LOC_WATCH_VERSION);

        uptime_ = lv_label_create(parent, NULL);
        lv_obj_align(uptime_, version_, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
        update_uptime();
        
        uptimeTicker_ = new UITicker(1000, [this]() {
            this->update_uptime();
        });

        wakeup_count_ = lv_label_create(parent, NULL);
        lv_obj_align(wakeup_count_, uptime_, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
        lv_label_set_text_fmt(wakeup_count_, LOC_WAKEUP_COUNT, gui_->get_wakeup_count());
    }

    virtual bool hide_internal() override
    {
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

    void update_watch_name(const char *new_name) // for when user changes watch name
    {
        strcpy(watch_name_, new_name);
        if (strcmp(watch_name_, "") == 0) // new name is an empty string
        {
            lv_label_set_text(watchName_, LOC_WATCH_NAME_EMPTY);
        }
        else
        {
            lv_label_set_text(watchName_, watch_name_);
        }
    }

private:
    Gui* gui_;
    lv_obj_t* version_;
    lv_obj_t* author_;
    lv_obj_t* uptime_;
    UITicker* uptimeTicker_;
    lv_obj_t* wakeup_count_;
    lv_obj_t* watchNameLabel_;
    lv_obj_t* watchNameButton_;
    lv_obj_t* watchName_;
    char watch_name_[16] = "";

    static void watch_name_button_callback(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_CLICKED)
        {
            auto* watch_info = (WatchInfo* )obj->user_data;
            int max_chars = 15;
            auto keyboard = new Keyboard(LOC_INPUT_WATCH_NAME, KeyboardType_t::Normal, max_chars);
            keyboard->on_close([keyboard, watch_info]() 
            {
                if (keyboard->is_success())
                {
                    const char* text = keyboard->get_text();
                    ESP_LOGI(SETTINGS_TAG, "keyboard->get_text() returned %s", text); //Jan: this is always executed twice - why?
                    watch_info->update_watch_name(text);
                }
                delete keyboard;
            });
            keyboard->show(lv_scr_act());
        }
    }
};