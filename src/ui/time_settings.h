#pragma once
#include <array>
#include "settings_view.h"
#include "localization.h"
#include "loader.h"
#include "ui_ticker.h"
#include "keyboard.h"
#include "roller.h"
#include "networking/signalk_socket.h"
#include "date_picker.h"

class TimeSettings : public SettingsView
{
public:
    TimeSettings(TTGOClass *watch, SignalKSocket *socket) : SettingsView(LOC_TIME_SETTINGS)
    {
        this->watch = watch;
        ws_socket = socket;
    }

    void update_hours(int hours)
    {
        this->hours = hours;
        time_changed = true;
        update_time_date_display();
        ESP_LOGI(SETTINGS_TAG, "User set hours to %d", hours);
    }

    void update_minutes(int minutes)
    {
        this->minutes = minutes;
        time_changed = true;
        update_time_date_display();
        ESP_LOGI(SETTINGS_TAG, "User set minutes to %d", minutes);
    }

    void toggle_am_pm()
    {
        if (!time_24hour_format)
        {
            is_time_am = !is_time_am;
            time_changed = true;
            update_time_date_display();
        }
    }

    void update_date(int year, int month, int day)
    {
        this->year = year;
        this->month = month;
        this->day = day;
        this->date_changed = true;
        update_time_date_display();
    }

    void update_timezone_id(int8_t new_timezone_id) // for when user changes the timezone
    {
        timezone_id = new_timezone_id;
        lv_label_set_text_fmt(timezone_label, "%s", get_timezone_string(timezone_id).c_str());
        ESP_LOGI(SETTINGS_TAG, "User set timezone to %s", get_timezone_string(timezone_id).c_str());
        // BS: Need to adjust the actual time for this timezone change
    }

    bool get_24hour_format() { return time_24hour_format; }
    void set_24hour_format(bool value)
    {
        time_24hour_format = value; 
    }

    int8_t get_timezone_id() { return timezone_id; }
    void set_timezone_id(int8_t new_timezone_id)
    {
        timezone_id = new_timezone_id; 
    }

    String get_timezone_string(int8_t id) 
    {
        return timezone_strings[id];
    }

protected:
    virtual void show_internal(lv_obj_t *parent) override
    {
        auto time = watch->rtc->getDateTime();
        hours = time.hour;
        minutes = time.minute;
        day = time.day;
        month = time.month;
        year = time.year;
        is_time_am = hours <= 12;
        if (!time_24hour_format && !is_time_am)
        {
            hours = (hours % 12);
        }
        else if(!time_24hour_format && is_time_am && hours == 0)
        {
            hours = 12;
        }

        lv_cont_set_layout(parent, LV_LAYOUT_OFF);
        timeLabel = lv_label_create(parent, NULL);
        lv_label_set_text(timeLabel, LOC_TIME);
        lv_obj_set_pos(timeLabel, 4, 4);
        static lv_style_t buttonStyle;
        lv_style_init(&buttonStyle);
        lv_style_set_radius(&buttonStyle, LV_STATE_DEFAULT, 0);
        lv_style_set_radius(&buttonStyle, LV_STATE_PRESSED, 0);
        lv_style_set_radius(&buttonStyle, LV_STATE_DISABLED, 0);

        hourButton = lv_btn_create(parent, NULL);
        lv_obj_add_style(hourButton, LV_OBJ_PART_MAIN, &buttonStyle);
        hourLabel = lv_label_create(hourButton, NULL);
        lv_obj_set_event_cb(hourButton, TimeSettings::hour_button_callback);
        lv_obj_align(hourButton, timeLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 4);
        lv_obj_set_width(hourButton, 40);

        minuteButton = lv_btn_create(parent, NULL);
        lv_obj_add_style(minuteButton, LV_OBJ_PART_MAIN, &buttonStyle);
        minuteLabel = lv_label_create(minuteButton, NULL);

        lv_obj_set_event_cb(minuteButton, TimeSettings::minute_button_callback);
        lv_obj_align(minuteButton, hourButton, LV_ALIGN_OUT_RIGHT_MID, 4, 0);
        lv_obj_set_width(minuteButton, 40);

        timeModeButton = lv_btn_create(parent, NULL);
        lv_obj_add_style(timeModeButton, LV_OBJ_PART_MAIN, &buttonStyle);
        timeModeLabel = lv_label_create(timeModeButton, NULL);

        lv_obj_set_event_cb(timeModeButton, TimeSettings::toggle_am_pm_callback);
        lv_obj_align(timeModeButton, minuteButton, LV_ALIGN_OUT_RIGHT_MID, 4, 0);
        lv_obj_set_width(timeModeButton, 40);

        dateButton = lv_btn_create(parent, NULL);
        lv_obj_add_style(dateButton, LV_OBJ_PART_MAIN, &buttonStyle);
        dateLabel = lv_label_create(dateButton, NULL);
        lv_obj_align(dateButton, timeModeButton, LV_ALIGN_OUT_RIGHT_MID, 4, 0);
        lv_obj_set_event_cb(dateButton, TimeSettings::date_button_callback);
        lv_obj_set_width(dateButton, 100);

        timezoneTextLabel = lv_label_create(parent, NULL);
        lv_obj_align(timezoneTextLabel, hourButton, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 16);
        lv_label_set_text(timezoneTextLabel, LOC_TIMEZONE);
        timezone_button = lv_btn_create(parent, NULL);
        lv_obj_align(timezone_button, timezoneTextLabel, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
        lv_obj_add_style(timezone_button, LV_OBJ_PART_MAIN, &buttonStyle);
        timezone_label = lv_label_create(timezone_button, NULL);
        lv_label_set_text_fmt(timezone_label, "%s", get_timezone_string(timezone_id).c_str());
        lv_obj_set_event_cb(timezone_button, timezone_button_callback);
        lv_obj_set_width(timezone_button, 90);

        time_24hour_check = lv_switch_create(parent, NULL);
        // BS: Need to change the following to align it with some object, instead of an absolute position
        lv_obj_set_pos(time_24hour_check, 4, 114);
        time_24hour_label = lv_label_create(parent, NULL);
        lv_label_set_text(time_24hour_label, LOC_24HOUR_TIME);
        lv_obj_align(time_24hour_label, time_24hour_check, LV_ALIGN_OUT_RIGHT_MID, 4, 0);
        lv_obj_set_event_cb(time_24hour_check, TimeSettings::time_24hour_callback);

        if (time_24hour_format)
        {
            lv_switch_on(time_24hour_check, LV_ANIM_OFF);
        }

        sk_sync_check = lv_switch_create(parent, NULL);
        lv_obj_align(sk_sync_check, time_24hour_check, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
        sk_sync_label = lv_label_create(parent, NULL);
        lv_label_set_text(sk_sync_label, LOC_SIGNALK_SYNC_TIME);
        lv_obj_align(sk_sync_label, sk_sync_check, LV_ALIGN_OUT_RIGHT_MID, 4, 0);
        lv_obj_set_event_cb(sk_sync_check, TimeSettings::sync_with_sk_callback);

        if (ws_socket->get_sync_time_with_server())
        {
            lv_switch_on(sk_sync_check, LV_ANIM_OFF);
        }

        hourButton->user_data = this;
        minuteButton->user_data = this;
        timeModeButton->user_data = this;
        dateButton->user_data = this;
        sk_sync_check->user_data = this;
        time_24hour_check->user_data = this;
        timezone_button->user_data = this;

        update_time_date_display();
    }

    virtual bool hide_internal() override
    {
        if (time_changed || date_changed)
        {
            save_time_settings();
        }
        if (sk_settings_changed)
        {
            save_sk_settings();
        }

        return true;
    }

private:
    TTGOClass *watch;
    SignalKSocket *ws_socket;
    lv_obj_t *timeLabel;
    lv_obj_t *hourButton;
    lv_obj_t *hourLabel;
    lv_obj_t *minuteButton;
    lv_obj_t *minuteLabel;
    lv_obj_t *timeModeButton;
    lv_obj_t *timeModeLabel;
    lv_obj_t *dateLabel;
    lv_obj_t *dateButton;
    lv_obj_t *timezoneTextLabel;
    lv_obj_t *sk_sync_check;
    lv_obj_t *sk_sync_label;
    lv_obj_t *time_24hour_check;
    lv_obj_t *time_24hour_label;
    lv_obj_t *timezone_button;
    lv_obj_t *timezone_label;
    int hours = 0;
    int minutes = 0;
    int day = 0;
    int month = 0;
    int year = 0;
    bool time_24hour_format = false;
    char *months[13] = LOC_MONTHS;
    int8_t timezone_id = 12; // only until it's changed the first time
    std::array<String, 25> timezone_strings = {"GMT-12:00", "GMT-11:00", "GMT-10:00",
        "GMT-9:00", "GMT-8:00", "GMT-7:00", "GMT-6:00", "GMT-5:00", "GMT-4:00",
        "GMT-3:00", "GMT-2:00", "GMT-1:00","GMT0", "GMT+1:00", "GMT+2:00", "GMT+3:00",
        "GMT+4:00", "GMT+5:00", "GMT+6:00", "GMT+7:00", "GMT+8:00", "GMT+9:00",
        "GMT+10:00", "GMT11:00", "GMT+12:00"};
    bool time_changed = false;
    bool date_changed = false;
    bool sk_settings_changed = false;
    bool is_time_am = true;

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
                    if (settings->get_24hour_format())
                    {
                        if (hour >= 0 && hour <= 23)
                        {
                            settings->update_hours(hour);
                        }
                    }
                    else
                    {
                        if (hour >= 1 && hour <= 12)
                        {
                            settings->update_hours(hour);
                        }
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

    static void toggle_am_pm_callback(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_CLICKED)
        {
            TimeSettings *settings = (TimeSettings *)obj->user_data;
            settings->toggle_am_pm();
        }
    }

    static void date_button_callback(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_CLICKED)
        {
            auto timeSettings = (TimeSettings *)obj->user_data;
            auto datePicker = new DatePicker(LOC_SELECT_DATE);
            datePicker->on_close([datePicker, timeSettings]() {
                if (datePicker->is_success())
                {
                    auto date = datePicker->get_date();
                    timeSettings->update_date(date.year, date.month, date.day);
                }

                delete datePicker;
            });

            datePicker->show(lv_scr_act());
        }
    }

    static void timezone_button_callback(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_CLICKED)
        {
            auto timeSettings = (TimeSettings*)obj->user_data;
            auto roller = new Roller(LOC_INPUT_TIMEZONE, RollerType_t::Timezone, timeSettings->timezone_id);
            roller->on_close([roller, timeSettings]() 
            {
                if (roller->is_success())
                {
                    auto selected_id = roller->get_selected_id();
                    timeSettings->update_timezone_id(selected_id);
                }
            });
            roller->show(lv_scr_act());
        }
    }

    static void sync_with_sk_callback(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_VALUE_CHANGED)
        {
            TimeSettings *settings = (TimeSettings *)obj->user_data;
            bool state = lv_switch_get_state(obj);
            ESP_LOGI(SETTINGS_TAG, "User changed sync with SK to %s", state ? "on" : "off");
            settings->sk_settings_changed = true;
        }
    }

    static void time_24hour_callback(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_VALUE_CHANGED)
        {
            TimeSettings *settings = (TimeSettings *)obj->user_data;
            bool state = lv_switch_get_state(obj);
            ESP_LOGI(SETTINGS_TAG, "User changed 24 hour format to %s", state ? "on" : "off");
            if(!state) // state of the switch has changed
            {
                if(settings->hours > 12)
                {
                    settings->hours = (settings->hours % 12);
                    settings->is_time_am = false;
                }
                else
                {
                    if(settings->hours == 0)
                    {
                        settings->hours = 12;
                    }
                    settings->is_time_am = true;
                }
            }
            else
            {
                if(!settings->is_time_am)
                {
                    settings->hours = (settings->hours + 12) % 24;
                }
                else if(settings->hours == 12)
                {
                    settings->hours = 0;
                }
            }

            settings->time_24hour_format = state;
            settings->update_time_date_display();
        }
    }

    void update_time_date_display()
    {
        lv_label_set_text_fmt(hourLabel, "%d", hours);
        lv_label_set_text_fmt(minuteLabel, "%.2d", minutes);

        if (!time_24hour_format)
        {
            lv_label_set_text(timeModeLabel, is_time_am ? "am" : "pm");
            lv_obj_set_hidden(timeModeButton, false);
        }
        else
        {
            lv_obj_set_hidden(timeModeButton, true);
        }

        lv_label_set_text_fmt(dateLabel, LOC_DATE_FORMAT, day, months[month], year);
    }

    void save_time_settings()
    {
        ESP_LOGI(SETTINGS_TAG, "Saving time %d:%d %d-%d-%d into RTC...", hours, minutes, year, month, day);
        auto rtc = watch->rtc;

        auto currentTime = rtc->getDateTime();
        if (time_24hour_format)
        {
            currentTime.hour = hours;
        }
        else
        {
            currentTime.hour = hours + (is_time_am ? 0 : 12);
        }
        currentTime.minute = minutes;
        currentTime.second = 0;
        currentTime.year = year;
        currentTime.month = month;
        currentTime.day = day;
        rtc->setDateTime(currentTime);
        rtc->syncToSystem();

        ESP_LOGI(SETTINGS_TAG, "Updated RTC time is %s", rtc->formatDateTime(PCF_TIMEFORMAT_YYYY_MM_DD_H_M_S));
    }

    void save_sk_settings()
    {
        ws_socket->set_sync_time_with_server(lv_switch_get_state(sk_sync_check));
        ws_socket->save();
        ESP_LOGI(SETTINGS_TAG, "Updated SK websocket time sync to %d", ws_socket->get_sync_time_with_server());
    }
};