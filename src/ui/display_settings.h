#pragma once
#include "settings_view.h"
#include "localization.h"
#include "keyboard.h"

/**
 * @brief Used to set display brightness, wake-up sources (watch flip, touch,
 * double tap (accelerometer driven), and auto screen timeout (sleep time).
 **/

class DisplaySettings : public SettingsView
{
public:
    DisplaySettings(TTGOClass *watch) : SettingsView(LOC_DISPLAY_SETTINGS) 
    {
        watch_ = watch;
    }

    void update_timeout(int timeout_seconds) // for when user changes the screen timeout value
    {
        screen_timeout_ = timeout_seconds;
        lv_label_set_text_fmt(timeoutLabel_, "%d", screen_timeout_);
        ESP_LOGI(SETTINGS_TAG, "User set screen timeout to %d", screen_timeout_);
    }

    void update_brightness(uint8_t new_brightness_level) // for when user changes display brightness value
    {
        display_brightness_ = new_brightness_level;
        lv_label_set_text_fmt(brightnessLabel_, "%d", display_brightness_);
        uint8_t adjusted_brightness = display_brightness_;
        if (adjusted_brightness == 1)
        {
            adjusted_brightness = 10; // minimum readable level in bright light
        }
        else
        {
            adjusted_brightness = (adjusted_brightness - 1) * 63;
        }
        watch_->bl->adjust(adjusted_brightness);
        ESP_LOGI(SETTINGS_TAG, "User set display brightness to %d", display_brightness_);
    }

    int get_screen_timeout() { return screen_timeout_; }
    void set_screen_timeout(int value)
    {
        screen_timeout_ = value; 
    }

    int get_display_brightness() { return display_brightness_; }
    void set_display_brightness(uint8_t value)
    {
        display_brightness_ = value; 
    }

protected:
    virtual void show_internal(lv_obj_t *parent) override
    {
        lv_cont_set_layout(parent, LV_LAYOUT_PRETTY_TOP);

        static lv_style_t buttonStyle;
        lv_style_init(&buttonStyle);
        lv_style_set_radius(&buttonStyle, LV_STATE_DEFAULT, 0);
        lv_style_set_radius(&buttonStyle, LV_STATE_PRESSED, 0);
        lv_style_set_radius(&buttonStyle, LV_STATE_DISABLED, 0);
        
        screenTimeoutLabel_ = lv_label_create(parent, NULL);
        lv_label_set_text(screenTimeoutLabel_, LOC_SCREEN_TIMEOUT);
        timeoutButton_ = lv_btn_create(parent, NULL);
        lv_obj_add_style(timeoutButton_, LV_OBJ_PART_MAIN, &buttonStyle);
        timeoutLabel_ = lv_label_create(timeoutButton_, NULL);
        lv_label_set_text_fmt(timeoutLabel_, "%d", screen_timeout_);
        lv_obj_set_event_cb(timeoutButton_, timeout_button_callback);
        lv_obj_set_width(timeoutButton_, 50);

        displayBrightnessLabel_ = lv_label_create(parent, NULL);
        lv_label_set_text(displayBrightnessLabel_, LOC_DISPLAY_BRIGHTNESS);
        brightnessButton_ = lv_btn_create(parent, NULL);
        lv_obj_add_style(brightnessButton_, LV_OBJ_PART_MAIN, &buttonStyle);
        brightnessLabel_ = lv_label_create(brightnessButton_, NULL);
        lv_label_set_text_fmt(brightnessLabel_, "%d", display_brightness_);
        lv_obj_set_event_cb(brightnessButton_, brightness_button_callback);
        lv_obj_set_width(brightnessButton_, 50);

        timeoutButton_->user_data = this;
        brightnessButton_->user_data = this;
    }

    virtual bool hide_internal() override
    {
        return true;
    }

private:
    TTGOClass* watch_;
    lv_obj_t* screenTimeoutLabel_;
    lv_obj_t* timeoutButton_;
    lv_obj_t* timeoutLabel_;
    int screen_timeout_ = 10; // multiply by 1000 in main.cpp to make this "seconds"
    lv_obj_t* displayBrightnessLabel_;
    lv_obj_t* brightnessButton_;
    lv_obj_t* brightnessLabel_;
    uint8_t display_brightness_;

    static void timeout_button_callback(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_CLICKED)
        {
            DisplaySettings* settings = (DisplaySettings* )obj->user_data;
            auto keyboard = new Keyboard(LOC_INPUT_SCREEN_TIMEOUT, KeyboardType_t::Number, 2);
            keyboard->on_close([keyboard, settings]() {
                if (keyboard->is_success())
                {
                    const char *text = keyboard->get_text();

                    int timeout = atoi(text);
                    if (timeout >= 5)
                    {
                        settings->update_timeout(timeout);
                    }
                }
            });
            keyboard->show(lv_scr_act());
        }
    }

    static void brightness_button_callback(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_CLICKED)
        {
            DisplaySettings* settings = (DisplaySettings* )obj->user_data;
            uint8_t max_digits = 1;
            auto keyboard = new Keyboard(LOC_INPUT_DISPLAY_BRIGHTNESS, KeyboardType_t::Brightness, max_digits);
            keyboard->on_close([keyboard, settings]() 
            {
                if (keyboard->is_success())
                {
                    const char *text = keyboard->get_text();
                    settings->update_brightness(atoi(text));
                }
            });
            keyboard->show(lv_scr_act());
        }
    }
};