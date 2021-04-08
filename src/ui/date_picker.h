#include "settings_view.h"
#include "config.h"

LV_FONT_DECLARE(lv_font_montserrat_14);

class DatePicker : public SettingsView
{
public:
    DatePicker(char *title) : SettingsView(title)
    {
    }

    virtual void show_internal(lv_obj_t *parent) override
    {
        lv_cont_set_layout(parent, LV_LAYOUT_OFF);
        calendar = lv_calendar_create(parent, NULL);
        lv_obj_set_size(calendar, 200, 200);
        lv_obj_align(calendar, parent, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_local_text_font(calendar, LV_CALENDAR_PART_DATE, LV_STATE_DEFAULT, &lv_font_montserrat_14);

        auto today = TTGOClass::getWatch()->rtc->getDateTime();
        lv_calendar_date_t date;
        date.year = today.year;
        date.month = today.month;
        date.day = today.day;
        selected_date = date;
        lv_calendar_set_today_date(calendar, &date);
        lv_calendar_set_showed_date(calendar, &date);
        calendar->user_data = this;
        lv_obj_set_event_cb(calendar, DatePicker::calendar_callback);
    }

    bool is_success() { return success; }
    lv_calendar_date_t get_date() { return selected_date; }

private:
    lv_obj_t *calendar;
    lv_calendar_date_t selected_date;
    bool success = false;

    void date_selected(lv_calendar_date_t date)
    {
        success = true;
        selected_date = date;
        ESP_LOGI(SETTINGS_TAG, "User selected %d-%d-%d date in picker.", date.year, date.month, date.day);
        this->hide();
    }

    static void calendar_callback(lv_obj_t*obj, lv_event_t event)
    {
        if(event == LV_EVENT_VALUE_CHANGED)
        {
            auto selectedDate = lv_calendar_get_pressed_date(obj);
            if(selectedDate)
            {
                
                DatePicker*picker = ((DatePicker*)obj->user_data);
                lv_calendar_date_t passDate;
                passDate.year = selectedDate->year;
                passDate.month = selectedDate->month;
                passDate.day = selectedDate->day;
                picker->date_selected(passDate);
            }
        }
    }
};