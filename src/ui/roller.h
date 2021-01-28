#pragma once
#include <functional>
#include "localization.h"
#include "config.h"
#include "settings_view.h"

/*****************************************************************
 *
 *          Roller Class
 *
 */

enum RollerType_t
{
    Timezone
};

class Roller : public SettingsView
{
public:
    Roller(char *title, RollerType_t type, uint16_t starting_id) : SettingsView(title)
    {
        roller_type = type;
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
        roller_widget = lv_roller_create(parent, NULL);
        lv_obj_align(roller_widget, parent, LV_ALIGN_IN_TOP_MID, 10, 10);
        lv_obj_add_style(roller_widget, LV_OBJ_PART_MAIN, &rollerStyle);
        
        if (roller_type == RollerType_t::Timezone)
        {
            lv_roller_set_options(roller_widget, timezone_option_list, 0 /*LV_ROLLER_MODE_INFINITE*/); // BS: compiler can't find LV_ROLLER_MODE_INFINITE
        }
        // add more RollerType_t checking here as they are added      
        
        lv_roller_set_selected(roller_widget, starting_id_, 1); // BS: 0 should be LV_ANIM_ON or LV_ANIM_OFF
        lv_roller_set_visible_row_count(roller_widget, 5); // number of rows to make visible in the widget
        lv_obj_set_event_cb(roller_widget, roller_widget_cb);
        roller_widget->user_data = this;
    }

    virtual bool hide_internal() override
    {
        return true;
    }

    bool is_success()
    {
        return success;
    }
    
    uint16_t get_selected_id()
    {
        return selected_id;
    }
    
    void set_selected_id(uint16_t new_selected_id)
    {
        selected_id = new_selected_id;
    }

private:
    lv_obj_t *roller_widget;
    RollerType_t roller_type;
    uint16_t selected_id;
    bool success = false;
    const char* timezone_option_list =
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
            roller_ptr->selected_id = sel_id;
            roller_ptr->success = true;
        }

    }

};
     

