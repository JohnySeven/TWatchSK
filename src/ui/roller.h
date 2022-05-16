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
    Timezone,
    Year,
    Month,
    Day28,
    Day30,
    Day31,
    Hours24,
    Hours12,
    Minutes
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

        /*Create a roller and apply the styles*/
        roller_widget_ = lv_roller_create(parent, NULL);
        lv_obj_align(roller_widget_, parent, LV_ALIGN_IN_TOP_MID, 10, 10);
        const char * options = NULL;

        if (roller_type_ == RollerType_t::Timezone)
        {
            options = timezone_option_list_;

        }
        else if(roller_type_ == RollerType_t::Year)
        {
            options = year_option_list_;
        }
        else if(roller_type_ == RollerType_t::Month)
        {
            options =  LOC_MONTHS_FULL;
        }
        else if(roller_type_ == RollerType_t::Day31)
        {
            options =  day31_option_list_;
        }
        else if(roller_type_ == RollerType_t::Day30)
        {
            options =  day30_option_list_;
        }
        else if(roller_type_ == RollerType_t::Day28)
        {
            options =  day28_option_list_;
        }
        else if(roller_type_ == RollerType_t::Hours12)
        {
            options =  hours12_option_list_;
        }
        else if(roller_type_ == RollerType_t::Hours24)
        {
            options = hours24_option_list_;
        }
        else if(roller_type_ == RollerType_t::Minutes)
        {
            options = minutes_option_list_;
        }

        lv_roller_set_options(roller_widget_, options, LV_ROLLER_MODE_INIFINITE);
        // add more RollerType_t checking here as they are added      
        lv_roller_set_selected(roller_widget_, starting_id_, LV_ANIM_OFF);
        //set selected id to starting_id_
        selected_id_ = starting_id_;
        lv_roller_set_visible_row_count(roller_widget_, 5); // number of rows to make visible in the widget
        lv_obj_set_event_cb(roller_widget_, roller_widget_cb);
        //add OK button
        ok_button_ = lv_btn_create(topBar, NULL);
        lv_obj_set_style_local_border_width(ok_button_, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 0);
        lv_obj_set_style_local_border_width(ok_button_, LV_BTN_PART_MAIN, LV_STATE_FOCUSED, 0);
        lv_obj_set_style_local_border_width(ok_button_, LV_BTN_PART_MAIN, LV_STATE_PRESSED, 0);
        lv_obj_set_style_local_radius(ok_button_, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 0);
        lv_obj_set_style_local_radius(ok_button_, LV_BTN_PART_MAIN, LV_STATE_FOCUSED, 0);
        lv_obj_set_style_local_radius(ok_button_, LV_BTN_PART_MAIN, LV_STATE_PRESSED, 0);
        lv_obj_set_size(ok_button_, 40, lv_obj_get_height(topBar));
        ok_button_->user_data = this;
        lv_obj_set_event_cb(ok_button_, ok_button_cb);
        //OK label on button
        auto lbl = lv_label_create(ok_button_, NULL);
        lv_label_set_text(lbl, LOC_MESSAGEBOX_OK);

        lv_obj_align(ok_button_, topBar, LV_ALIGN_IN_TOP_RIGHT, 0, 0);        

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

    void theme_changed() override
    {
        twatchsk::update_imgbtn_color(back); // make the "Back" button the correct color depending on the theme

    }

private:
    lv_obj_t *roller_widget_;
    lv_obj_t *ok_button_;
    RollerType_t roller_type_;
    uint16_t selected_id_;
    bool success_ = false;
    const char* timezone_option_list_ =
        "GMT-12:00\n" "GMT-11:00\n" "GMT-10:00\n" "GMT-9:00\n" "GMT-8:00\n" "GMT-7:00\n" "GMT-6:00\n"
        "GMT-5:00\n" "GMT-4:00\n" "GMT-3:00\n" "GMT-2:00\n" "GMT-1:00\n" "GMT0\n" "GMT+1:00\n" "GMT+2:00\n"
        "GMT+3:00\n" "GMT+4:00\n" "GMT+5:00\n" "GMT+6:00\n" "GMT+7:00\n" "GMT+8:00\n" "GMT+9:00\n"
        "GMT+10:00\n" "GMT+11:00\n" "GMT+12:00";
    uint16_t starting_id_ = 12; // equates to GMT0

    const char* year_option_list_ =
        "2022\n" "2023\n" "2024\n"
        "2025\n" "2026\n" "2027\n"
        "2028\n" "2029\n" "2030";    

    const char * day31_option_list_ = "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31";
    const char * day30_option_list_ = "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30";
    const char * day28_option_list_ = "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29";
    const char * minutes_option_list_ = "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31\n32\n33\n34\n35\n36\n37\n38\n39\n40\n41\n42\n43\n44\n45\n46\n47\n48\n49\n50\n51\n52\n53\n54\n55\n56\n57\n58\n59";
    const char * hours24_option_list_ = "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23";
    const char * hours12_option_list_ = "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12";

    static void roller_widget_cb(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_VALUE_CHANGED)
        {
            const uint16_t sel_id = lv_roller_get_selected(obj);
            Roller* roller_ptr = ((Roller*)obj->user_data);
            roller_ptr->selected_id_ = sel_id;

            ESP_LOGI("ROLLER", "SelectedId=%d", sel_id);
        }
    }

    static void ok_button_cb(lv_obj_t *obj, lv_event_t event)
    {
        ESP_LOGI("ROLLER", "OK_BUTTON event=%d", event);

        if (event == LV_EVENT_CLICKED)
        {
            Roller* roller_ptr = ((Roller*)obj->user_data);
            roller_ptr->success_ = true;
            roller_ptr->hide();
        }
    }
};
     

