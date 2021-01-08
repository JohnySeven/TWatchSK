#pragma once
#include "settings_view.h"
#include "localization.h"
#include "loader.h"
#include "ui_ticker.h"
#include "keyboard.h"

class TimeSettings : public SettingsView
{
public:
    TimeSettings(TTGOClass *watch) : SettingsView(LOC_TIME_SETTINGS)
    {
        this->watch = watch;
    }

    void update_hours(int hours)
    {
        this->hours = hours;
        time_changed = true;
        lv_label_set_text_fmt(hourLabel, "%d", hours);
        ESP_LOGI(SETTINGS_TAG, "User set hours to %d", hours);
    }

    void update_minutes(int minutes)
    {
        this->minutes = minutes;
        time_changed = true;
        lv_label_set_text_fmt(minuteLabel, "%d", minutes);
        ESP_LOGI(SETTINGS_TAG, "User set minutes to %d", minutes);

    }

protected:
    virtual void show_internal(lv_obj_t *parent) override
    {
        auto time = watch->rtc->getDateTime();
        hours = time.hour;
        minutes = time.minute;
        lv_cont_set_layout(parent, LV_LAYOUT_OFF);
        timeLabel = lv_label_create(parent, NULL);
        lv_label_set_text(timeLabel, LOC_TIME);
        static lv_style_t buttonStyle;
        lv_style_init(&buttonStyle);
        lv_style_set_radius(&buttonStyle, LV_STATE_DEFAULT, 0);
        lv_style_set_radius(&buttonStyle, LV_STATE_PRESSED, 0);
        lv_style_set_radius(&buttonStyle, LV_STATE_DISABLED, 0);

        hourButton = lv_btn_create(parent, NULL);
        lv_obj_add_style(hourButton, LV_OBJ_PART_MAIN, &buttonStyle);
        hourLabel = lv_label_create(hourButton, NULL);
        lv_label_set_text_fmt(hourLabel, "%d", hours);
        lv_obj_set_event_cb(hourButton, TimeSettings::hour_button_callback);
        lv_obj_align(hourButton, timeLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 4);
        lv_obj_set_width(hourButton, 75);
        minuteButton = lv_btn_create(parent, NULL);
        lv_obj_add_style(minuteButton, LV_OBJ_PART_MAIN, &buttonStyle);
        minuteLabel = lv_label_create(minuteButton, NULL);
        lv_label_set_text_fmt(minuteLabel, "%d", minutes);
        lv_obj_set_event_cb(minuteButton, TimeSettings::minute_button_callback);
        lv_obj_align(minuteButton, hourButton, LV_ALIGN_OUT_RIGHT_MID, 4, 0);
        lv_obj_set_width(minuteButton, 75);

        dateButton = lv_btn_create(parent, NULL);
        lv_obj_add_style(dateButton, LV_OBJ_PART_MAIN, &buttonStyle);
        dateLabel = lv_label_create(dateButton, NULL);
        lv_label_set_text_fmt(dateLabel, "%d. %s %d", time.day, months[time.month], time.year);
        lv_obj_align(dateButton, hourButton, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 4);

        hourButton->user_data = this;
        minuteButton->user_data = this;
        dateButton->user_data = this;

        /*
        status = lv_label_create(parent, NULL);
        scanButton = lv_btn_create(parent, NULL);
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
        });*/
    }

    virtual bool hide_internal() override
    {
        if (time_changed)
        {
            save_settings();
        }
        return true;
    }

private:
    TTGOClass *watch;
    lv_obj_t *timeLabel;
    lv_obj_t *hourButton;
    lv_obj_t *hourLabel;
    lv_obj_t *minuteButton;
    lv_obj_t *minuteLabel;
    lv_obj_t *dateLabel;
    lv_obj_t *dateButton;
    int hours = 0;
    int minutes = 0;
    char *months[3][13] = LOC_MONTHS;
    //Loader *ScanLoader;
    //UITicker *statusUpdateTicker;
    bool time_changed = false;

    static void hour_button_callback(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_CLICKED)
        {
            TimeSettings *settings = (TimeSettings *)obj->user_data;
            auto keyboard = new Keyboard(LOC_INPUT_HOUR, KeyboardType_t::Number, 2);
            keyboard->on_close([keyboard, settings]() {
                if (keyboard->is_success())
                {
                    const char *text = keyboard->get_text();
                    int hour = atoi(text);
                    if (hour >= 0 && hour <= 23)
                    {
                        settings->update_hours(hour);
                    }
                }
            });
            keyboard->show(lv_scr_act());
        }
    }

    static void minute_button_callback(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_CLICKED)
        {
            TimeSettings *settings = (TimeSettings *)obj->user_data;
            auto keyboard = new Keyboard(LOC_INPUT_MINUTE, KeyboardType_t::Number, 2);
            keyboard->on_close([keyboard, settings]() {
                if (keyboard->is_success())
                {
                    const char *text = keyboard->get_text();

                    int minutes = atoi(text);
                    if (minutes >= 0 && minutes <= 59)
                    {
                        settings->update_minutes(minutes);
                    }
                }
            });
            keyboard->show(lv_scr_act());
        }
    }

    static void date_button_callback(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_CLICKED)
        {
        }
    }

    void update_time_info()
    {
    }

    void save_settings()
    {
        ESP_LOGI(SETTINGS_TAG, "Saving time %d:%d into RTC...", hours, minutes);
        auto rtc = watch->rtc;

        auto currentTime = rtc->getDateTime();
        currentTime.hour = hours;
        currentTime.minute = minutes;
        currentTime.second = 0;
        rtc->setDateTime(currentTime);
        rtc->syncToSystem();

        ESP_LOGI(SETTINGS_TAG, "Updated RTC time is %s", rtc->formatDateTime(PCF_TIMEFORMAT_YYYY_MM_DD_H_M_S));

    }
};