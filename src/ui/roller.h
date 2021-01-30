#pragma once
#include <functional>
#include "localization.h"
#include "config.h"
#include "settings_view.h"

/**
 * @brief Roller is a class for selecting an item from a scrolling list of items.
 * 
 * Initially used to select the watch's timezone, it's now available for anything else.
 * 
 * @param title The title at the top of the page. Set in localization.h
 * 
 * @param type What the roller is used to select. To add a type, add it to the 
 * enum just below, and define it similar to the private data member timezone_option_list_.
 * 
 * @param starting_id The internal ID of the item in the list that you want to be displayed
 * when theh roller is created. 0 is the first item in the list, 1 is the second, etc.
 * 
 * All of the code to set, save, and retrieve the timezone uses the internal ID. Only the
 * display of the timezone to the user converts it to human-friendly format. For example,
 * timezone ID 0 is displayed as "GMT-12:00". This is done with a function in time_settings.h
 * called get_timezone_string().
 **/

enum RollerType_t
{
    Timezone
};

class Roller : public SettingsView
{
public:
    Roller(char *title, RollerType_t type, uint16_t starting_id) : SettingsView(title)
    {
        roller_type_ = type;
        starting_id_ = starting_id;
     };

    ~Roller() { };

    virtual void show_internal(lv_obj_t *parent) override
    {
        lv_cont_set_layout(parent, LV_LAYOUT_CENTER);

        static lv_style_t rollerStyle;
        lv_style_init(&rollerStyle);
        lv_style_set_radius(&rollerStyle, LV_OBJ_PART_MAIN, 0);
        lv_style_set_bg_color(&rollerStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);
        lv_style_set_bg_opa(&rollerStyle, LV_OBJ_PART_MAIN, LV_OPA_100);
        lv_style_set_border_width(&rollerStyle, LV_OBJ_PART_MAIN, 0);
        lv_style_set_text_color(&rollerStyle, LV_OBJ_PART_MAIN, LV_COLOR_BLACK);
        lv_style_set_image_recolor(&rollerStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);

        /*Create a roller and apply the styles*/
        roller_widget_ = lv_roller_create(parent, NULL);
        lv_obj_align(roller_widget_, parent, LV_ALIGN_IN_TOP_MID, 10, 10);
        lv_obj_add_style(roller_widget_, LV_OBJ_PART_MAIN, &rollerStyle);
        
        if (roller_type_ == RollerType_t::Timezone)
        {
            lv_roller_set_options(roller_widget_, timezone_option_list_, 0 /*LV_ROLLER_MODE_INFINITE*/); // BS: compiler can't find LV_ROLLER_MODE_INFINITE
        }
        // add more RollerType_t checking here as they are added      
        lv_roller_set_selected(roller_widget_, starting_id_, 1); // BS: 0 should be LV_ANIM_ON or LV_ANIM_OFF
        lv_roller_set_visible_row_count(roller_widget_, 5); // number of rows to make visible in the widget
        lv_obj_set_event_cb(roller_widget_, roller_widget_cb);
        roller_widget_->user_data = this;
    }

    virtual bool hide_internal() override
    {
        return true;
    }

    bool is_success()
    {
        return success_;
    }
    
    uint16_t get_selected_id()
    {
        return selected_id_;
    }
    
    void set_selected_id(uint16_t new_selected_id)
    {
        selected_id_ = new_selected_id;
    }

private:
    lv_obj_t *roller_widget_;
    RollerType_t roller_type_;
    uint16_t selected_id_;
    bool success_ = false;
    const char* timezone_option_list_ =
        "GMT-12:00\n" "GMT-11:00\n" "GMT-10:00\n" "GMT-9:00\n" "GMT-8:00\n" "GMT-7:00\n" "GMT-6:00\n"
        "GMT-5:00\n" "GMT-4:00\n" "GMT-3:00\n" "GMT-2:00\n" "GMT-1:00\n" "GMT0\n" "GMT+1:00\n" "GMT+2:00\n"
        "GMT+3:00\n" "GMT+4:00\n" "GMT+5:00\n" "GMT+6:00\n" "GMT+7:00\n" "GMT+8:00\n" "GMT+9:00\n"
        "GMT+10:00\n" "GMT+11:00\n" "GMT+12:00";
    uint16_t starting_id_ = 12; // equates to GMT0

    static void roller_widget_cb(lv_obj_t *obj, lv_event_t event)
    {
        ESP_LOGI("ROLLER", "EVENT=%d", event);

        if (event == LV_EVENT_VALUE_CHANGED)
        {
            const uint16_t sel_id = lv_roller_get_selected(obj);
            Roller* roller_ptr = ((Roller*)obj->user_data);
            roller_ptr->selected_id_ = sel_id;
            roller_ptr->success_ = true;
        }

    }

};
     

