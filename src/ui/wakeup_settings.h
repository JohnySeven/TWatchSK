#pragma once
#include "gui.h"
#include "settings_view.h"
#include "localization.h"
#include "hardware/Hardware.h"

/**
 * @brief Allows user to configure wake up sources (double tap, watch tilt).
 * Future: Signal K notifications?
 **/

class WakeupSettings : public SettingsView
{
public:
    WakeupSettings(Gui *gui, Hardware *hardware) : SettingsView(LOC_WAKEUP_SETTINGS_MENU)
    {
        gui_ = gui;
        hardware_ = hardware;
    }

protected:
    virtual void show_internal(lv_obj_t *parent) override
    {
        const lv_coord_t padding = 8;
        lv_cont_set_layout(parent, LV_LAYOUT_OFF);

        title_ = lv_label_create(parent, NULL);
        lv_obj_set_pos(title_, padding, padding);
        lv_label_set_text(title_, LOC_WAKEUP_TITLE);

        double_tap_switch_ = lv_switch_create(parent, NULL);
        double_tap_wakeup_ = hardware_->get_double_tap_wakeup();
        if (double_tap_wakeup_)
        {
            lv_switch_on(double_tap_switch_, LV_ANIM_OFF);
        }
        lv_obj_align(double_tap_switch_, title_, LV_ALIGN_OUT_BOTTOM_LEFT, 0, padding);
        double_tap_label_ = lv_label_create(parent, NULL);
        lv_label_set_text(double_tap_label_, LOC_WAKEUP_DOUBLE_TAP);
        lv_obj_align(double_tap_label_, double_tap_switch_, LV_ALIGN_OUT_RIGHT_MID, padding, 0);
        double_tap_switch_->user_data = this;
        lv_obj_set_event_cb(double_tap_switch_, WakeupSettings::double_tap_switch_callback);

        tilt_switch_ = lv_switch_create(parent, NULL);
        tilt_wakeup_ = hardware_->get_tilt_wakeup();
        if (tilt_wakeup_)
        {
            lv_switch_on(tilt_switch_, LV_ANIM_OFF);
        }
        lv_obj_align(tilt_switch_, double_tap_switch_, LV_ALIGN_OUT_BOTTOM_LEFT, 0, padding);
        tilt_label_ = lv_label_create(parent, NULL);
        lv_label_set_text(tilt_label_, LOC_WAKEUP_TILT);
        lv_obj_align(tilt_label_, tilt_switch_, LV_ALIGN_OUT_RIGHT_MID, padding, 0);
        tilt_switch_->user_data = this;
        lv_obj_set_event_cb(tilt_switch_, WakeupSettings::double_tap_switch_callback);
    }

    virtual bool hide_internal() override
    {
        if (update_hardware_)
        {
            ESP_LOGI(SETTINGS_TAG, "Updating double tap=%d, tilt=%d", double_tap_wakeup_, tilt_wakeup_);
            hardware_->set_double_tap_wakeup(double_tap_wakeup_);
            hardware_->set_tilt_wakeup(tilt_wakeup_);
            hardware_->save();
        }
        return true;
    }

private:
    Gui *gui_;
    Hardware *hardware_;
    bool double_tap_wakeup_;
    bool tilt_wakeup_;
    bool update_hardware_ = false;
    lv_obj_t *title_;
    lv_obj_t *double_tap_switch_;
    lv_obj_t *double_tap_label_;
    lv_obj_t *tilt_switch_;
    lv_obj_t *tilt_label_;

    static void double_tap_switch_callback(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_VALUE_CHANGED)
        {
            auto settings = (WakeupSettings *)obj->user_data;
            auto value = lv_switch_get_state(obj);
            settings->double_tap_wakeup_ = value;
            settings->update_hardware_ = true;
        }
    }

    static void tilt_switch_callback(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_VALUE_CHANGED)
        {
            auto settings = (WakeupSettings *)obj->user_data;
            auto value = lv_switch_get_state(obj);
            settings->tilt_wakeup_ = value;
            settings->update_hardware_ = true;
        }
    }
};