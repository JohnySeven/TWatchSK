#pragma once
#include "settings_view.h"
#include "localization.h"
#include "keyboard.h"
#include "themes.h"
#include "ui/loader.h"
#include "networking/http_request.h"
#include "networking/signalk_socket.h"
#include "system/async_dispatcher.h"
#include "ui_ticker.h"

/**
 * @brief Used to set display brightness, auto screen timeout (sleep time), and
 * turn the Dark Theme on or off.
 **/

class DisplaySettings : public SettingsView
{
public:
    DisplaySettings(TTGOClass *watch, SignalKSocket *socket) : SettingsView(LOC_DISPLAY_SETTINGS)
    {
        watch_ = watch;
        socket_ = socket;
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

    void theme_changed() override
    {
        twatchsk::update_imgbtn_color(back);
        update_brightness(twatchsk::dark_theme_enabled ? 1 : 5);
        if (twatchsk::dark_theme_enabled)
        {
            lv_switch_on(dark_switch_, LV_ANIM_OFF);
        }
        else
        {
            lv_switch_off(dark_switch_, LV_ANIM_OFF);
        }
    }

protected:
    virtual void show_internal(lv_obj_t *parent) override
    {
        lv_cont_set_layout(parent, LV_LAYOUT_OFF);

        static lv_style_t buttonStyle;
        lv_style_init(&buttonStyle);
        lv_style_set_radius(&buttonStyle, LV_STATE_DEFAULT, 10);

        screenTimeoutLabel_ = lv_label_create(parent, NULL);
        lv_obj_set_pos(screenTimeoutLabel_, 4, 10);
        lv_label_set_text(screenTimeoutLabel_, LOC_SCREEN_TIMEOUT);
        timeoutButton_ = lv_btn_create(parent, NULL);
        lv_obj_add_style(timeoutButton_, LV_OBJ_PART_MAIN, &buttonStyle);
        lv_obj_align(timeoutButton_, screenTimeoutLabel_, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
        timeoutLabel_ = lv_label_create(timeoutButton_, NULL);
        lv_label_set_text_fmt(timeoutLabel_, "%d", screen_timeout_);
        lv_obj_set_event_cb(timeoutButton_, timeout_button_callback);
        lv_obj_set_width(timeoutButton_, 50);

        displayBrightnessLabel_ = lv_label_create(parent, NULL);
        lv_obj_align(displayBrightnessLabel_, screenTimeoutLabel_, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 16);
        lv_label_set_text(displayBrightnessLabel_, LOC_DISPLAY_BRIGHTNESS);
        brightnessButton_ = lv_btn_create(parent, NULL);
        lv_obj_align(brightnessButton_, displayBrightnessLabel_, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
        lv_obj_add_style(brightnessButton_, LV_OBJ_PART_MAIN, &buttonStyle);
        brightnessLabel_ = lv_label_create(brightnessButton_, NULL);
        lv_label_set_text_fmt(brightnessLabel_, "%d", display_brightness_);
        lv_obj_set_event_cb(brightnessButton_, brightness_button_callback);
        lv_obj_set_width(brightnessButton_, 50);

        dark_switch_ = lv_switch_create(parent, NULL);
        lv_obj_align(dark_switch_, displayBrightnessLabel_, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 16);
        if (lv_theme_get_flags() & LV_THEME_MATERIAL_FLAG_DARK) // is the theme's "dark" version currently on?
        {
            lv_switch_on(dark_switch_, LV_ANIM_OFF); // set the switch widget to "ON"
        }
        dark_switch_label_ = lv_label_create(parent, NULL);
        lv_obj_align(dark_switch_label_, dark_switch_, LV_ALIGN_OUT_RIGHT_MID, 4, 0);
        lv_label_set_text(dark_switch_label_, LOC_DARK_SWITCH_LABEL);
        if (twatchsk::dark_theme_enabled)
        {
            lv_switch_on(dark_switch_, LV_ANIM_OFF);
            uint32_t flag = LV_THEME_MATERIAL_FLAG_DARK;
            LV_THEME_DEFAULT_INIT(lv_theme_get_color_primary(), lv_theme_get_color_secondary(), flag,
                                  lv_theme_get_font_small(), lv_theme_get_font_normal(), lv_theme_get_font_subtitle(),
                                  lv_theme_get_font_title());
        }
        lv_obj_set_event_cb(dark_switch_, dark_switch_cb);

        download_ui_button_ = lv_btn_create(parent, NULL);
        lv_obj_t *downloadLabel = lv_label_create(download_ui_button_, NULL);
        lv_label_set_text(downloadLabel, LOC_DISPLAY_DOWNLOAD_UI);
        lv_obj_set_event_cb(download_ui_button_, download_button_cb);
        lv_obj_align(download_ui_button_, dark_switch_, LV_ALIGN_OUT_BOTTOM_LEFT, 4, 0);

        download_ui_button_->user_data = this;
        timeoutButton_->user_data = this;
        brightnessButton_->user_data = this;
        dark_switch_->user_data = this;
    }

    virtual bool hide_internal() override
    {
        return true;
    }

private:
    TTGOClass *watch_;
    SignalKSocket *socket_;
    lv_obj_t *screenTimeoutLabel_;
    lv_obj_t *timeoutButton_;
    lv_obj_t *timeoutLabel_;
    int screen_timeout_ = 10; // multiply by 1000 in main.cpp to make this "seconds"
    lv_obj_t *displayBrightnessLabel_;
    lv_obj_t *brightnessButton_;
    lv_obj_t *brightnessLabel_;
    uint8_t display_brightness_;
    lv_obj_t *dark_switch_;
    lv_obj_t *dark_switch_label_;
    lv_obj_t *download_ui_button_;

    static void timeout_button_callback(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_CLICKED)
        {
            DisplaySettings *settings = (DisplaySettings *)obj->user_data;
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
                delete keyboard;
            });
            keyboard->show(lv_scr_act());
        }
    }

    static void brightness_button_callback(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_CLICKED)
        {
            DisplaySettings *settings = (DisplaySettings *)obj->user_data;
            uint8_t max_digits = 1;
            auto keyboard = new Keyboard(LOC_INPUT_DISPLAY_BRIGHTNESS, KeyboardType_t::Brightness, max_digits);
            keyboard->on_close([keyboard, settings]() {
                if (keyboard->is_success())
                {
                    const char *text = keyboard->get_text();
                    settings->update_brightness(atoi(text));
                }
                delete keyboard;
            });
            keyboard->show(lv_scr_act());
        }
    }

    static void dark_switch_cb(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_VALUE_CHANGED)
        {
            DisplaySettings* settings = (DisplaySettings* )obj->user_data;
            uint32_t flag = lv_switch_get_state(obj) ? LV_THEME_MATERIAL_FLAG_DARK : LV_THEME_MATERIAL_FLAG_LIGHT; // create theme flag that matches current state of the Dark Theme switch
            twatchsk::dark_theme_enabled = !twatchsk::dark_theme_enabled; // switch value changed, so save the changed value
            LV_THEME_DEFAULT_INIT(lv_theme_get_color_primary(), lv_theme_get_color_secondary(), flag,
                                  lv_theme_get_font_small(), lv_theme_get_font_normal(), lv_theme_get_font_subtitle(),
                                  lv_theme_get_font_title());
            twatchsk::update_imgbtn_color(settings->back);
            uint8_t new_brightness_level = (twatchsk::dark_theme_enabled == true ? 1 : 5);
            settings->update_brightness(new_brightness_level);
        }
    }

    static void download_button_cb(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_CLICKED)
        {
            auto settings = (DisplaySettings *)obj->user_data;

            settings->download_ui_from_server();
        }
    }

    void download_ui_from_server()
    {
        if (socket_->get_token() != "" && socket_->get_state() == WebsocketState_t::WS_Connected)
        {
            ESP_LOGI(SETTINGS_TAG, "About to start downloading Dynamic UI from SK Server...");

            //auto loader = new Loader(LOC_DISPLAY_DOWNLOADING_UI);
            bool done = false;
            /*UITicker* ticker = NULL;
        ticker = new UITicker(1000, [loader, &done, ticker]()
        {
            ESP_LOGI(SETTINGS_TAG, "Downloading status=%d", done);
            if(done)
            {
                delete loader;
                delete ticker;
            }
        });*/
            auto address = socket_->get_server_address();
            int port = socket_->get_server_port();
            auto token = socket_->get_token();

            /*twatchsk::run_async("Dynamic UI download", [address, port, token, &done]()
        {*/
            char requestUri[192];
            ESP_LOGI(SETTINGS_TAG, "Server: %s:%d", address.c_str(), port);
            sprintf(requestUri, "http://%s:%d/signalk/v1/applicationData/global/twatch/1.0/ui/default", address.c_str(), port);
            ESP_LOGI(SETTINGS_TAG, "Will be downloading from URI: %s", requestUri);
            auto http = new JsonHttpRequest(requestUri, token.c_str());
            if (http->downloadFile("/sk_view.json"))
            {
                show_message(LOC_DISPLAY_DOWNLOAD_UI_DONE);
                auto ticker = new UITicker(1000, []() {
                    static int countDown = 5;
                    countDown--;
                    if (countDown < 0)
                    {
                        esp_restart();
                    }
                });
            }
            else
            {
                show_message(LOC_DISPLAY_DOWNLOAD_UI_ERROR);
            }
            //done = true;
            //});
        }
        else
        {
            show_message(LOC_DISPLAY_DOWNLOAD_UI_NO_CONNECTION);
        }
    }
};