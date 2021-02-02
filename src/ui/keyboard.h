#pragma once
#include <functional>
#include "localization.h"
#include "config.h"
#include "settings_view.h"

/*****************************************************************
 *
 *          ! Keyboard Class
 *
 */

enum KeyboardType_t
{
    Normal,
    Number,
    Brightness
};

class Keyboard : public SettingsView
{
public:
    Keyboard(char *title, KeyboardType_t type = KeyboardType_t::Normal, int maxTextLength = 0) : SettingsView(title)
    {
        keyboardType = type;
        maxLength = maxTextLength;
    };

    ~Keyboard(){

    };

    virtual void show_internal(lv_obj_t *parent) override
    {
        lv_cont_set_layout(parent, LV_LAYOUT_OFF);

        static lv_style_t kbStyle;

        lv_style_init(&kbStyle);
        lv_style_set_radius(&kbStyle, LV_OBJ_PART_MAIN, 0);
        lv_style_set_bg_color(&kbStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);
        lv_style_set_bg_opa(&kbStyle, LV_OBJ_PART_MAIN, LV_OPA_100);
        lv_style_set_border_width(&kbStyle, LV_OBJ_PART_MAIN, 0);
        lv_style_set_text_color(&kbStyle, LV_OBJ_PART_MAIN, LV_COLOR_BLACK);
        lv_style_set_image_recolor(&kbStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);

        lv_obj_t *ta = lv_textarea_create(parent, NULL);
        lv_obj_set_height(ta, 40);
        lv_textarea_set_one_line(ta, true);
        lv_textarea_set_pwd_mode(ta, false);
        lv_textarea_set_text(ta, "");
        lv_textarea_set_max_length(ta, 128);

        lv_obj_align(ta, parent, LV_ALIGN_IN_TOP_MID, 10, 10);
        //lv_obj_add_style(ta, LV_OBJ_PART_MAIN, &kbStyle);
        /*Create a keyboard and apply the styles*/
        lv_obj_t *kb = lv_keyboard_create(parent, NULL);
        lv_keyboard_set_cursor_manage(kb, true);
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_set_pos(kb, 0, 50);
        lv_obj_set_size(kb, LV_HOR_RES, lv_obj_get_height(parent) - 50);
        if (keyboardType == KeyboardType_t::Normal)
        {
            lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_LOWER, btnm_mapplus[0]);
        }
        else if (keyboardType == KeyboardType_t::Number)
        {
            lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_LOWER, btnm_numeric[0]);
            lv_textarea_set_accepted_chars(ta, "0123456789");
        }
        else // type == Brightness)
        {
            lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_LOWER, btnm_brightness[0]);
            lv_textarea_set_accepted_chars(ta, "12345");
        }
        if (maxLength != 0)
        {
            lv_textarea_set_max_length(ta, maxLength);
        }
        kb->user_data = this;
        lv_obj_set_event_cb(kb, __kb_event_cb);
    }

    virtual bool hide_internal() override
    {
        return true;
    }

    static void __kb_event_cb(lv_obj_t *kb, lv_event_t event)
    {
        ESP_LOGI("KEYBOARD", "EVENT=%d", event);

        if (event == LV_EVENT_VALUE_CHANGED || event == LV_EVENT_LONG_PRESSED_REPEAT)
        {
            lv_keyboard_ext_t *ext = (lv_keyboard_ext_t *)lv_obj_get_ext_attr(kb);
            Keyboard *keyboard = (Keyboard *)kb->user_data;
            const char *txt = lv_btnmatrix_get_active_btn_text(kb);
            if (txt != NULL)
            {
                ESP_LOGI("KEYBOARD", "Keypress=%s", txt);
                static int index = 0;
                if (strcmp(txt, LV_SYMBOL_OK) == 0)
                {
                    strcpy(keyboard->__buf, lv_textarea_get_text(ext->ta));
                    keyboard->close_and_result(true);
                    return;
                }
                else if (strcmp(txt, LV_SYMBOL_LEFT) == 0)
                {
                    index = index - 1 < 0 ? 9 : index - 1;
                    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_LOWER, btnm_mapplus[index]);
                    return;
                }
                else if (strcmp(txt, LV_SYMBOL_RIGHT) == 0)
                {
                    index = index + 1 >= sizeof(btnm_mapplus) / sizeof(btnm_mapplus[0]) ? 0 : index + 1;
                    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_LOWER, btnm_mapplus[index]);
                    return;
                }
                else if (strcmp(txt, LOC_KEYBOARD_DEL) == 0)
                {
                    lv_textarea_del_char(ext->ta);
                }
                else
                {
                    lv_textarea_add_text(ext->ta, txt);
                }
            }
        }
    }
    
    const char *get_text()
    {
        return (const char *)__buf;
    }

    bool is_success()
    {
        return _isSuccess;
    }

private:
    static const char *btnm_mapplus[10][23];
    static const char *btnm_numeric[1][23];
    static const char *btnm_brightness[1][23];
    char __buf[128];
    bool _isSuccess = false;
    KeyboardType_t keyboardType;
    int maxLength;

    void close_and_result(bool is_success)
    {
        _isSuccess = is_success;
        hide();
    }
};

const char *Keyboard::btnm_numeric[1][23] = {
    {"1", "2", "3", "\n",
     "4", "5", "6", "\n",
     "7", "8", "9", "\n",
     "0", LV_SYMBOL_OK, LOC_KEYBOARD_DEL, "", ""}};

const char *Keyboard::btnm_brightness[1][23] = {
    {"1", "2", "3", "\n",
     "4", "5", LV_SYMBOL_OK, LOC_KEYBOARD_DEL, "", ""}};     

const char *Keyboard::btnm_mapplus[10][23] = {
    {"a", "b", "c", "\n",
     "d", "e", "f", "\n",
     "g", "h", "i", "\n",
     LV_SYMBOL_OK, LOC_KEYBOARD_DEL, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, ""},
    {"j", "k", "l", "\n",
     "n", "m", "o", "\n",
     "p", "q", "r", "\n",
     LV_SYMBOL_OK, LOC_KEYBOARD_DEL, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, ""},
    {"s", "t", "u", "\n",
     "v", "w", "x", "\n",
     "y", "z", " ", "\n",
     LV_SYMBOL_OK, LOC_KEYBOARD_DEL, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, ""},
    {"A", "B", "C", "\n",
     "D", "E", "F", "\n",
     "G", "H", "I", "\n",
     LV_SYMBOL_OK, LOC_KEYBOARD_DEL, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, ""},
    {"J", "K", "L", "\n",
     "N", "M", "O", "\n",
     "P", "Q", "R", "\n",
     LV_SYMBOL_OK, LOC_KEYBOARD_DEL, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, ""},
    {"S", "T", "U", "\n",
     "V", "W", "X", "\n",
     "Y", "Z", " ", "\n",
     LV_SYMBOL_OK, LOC_KEYBOARD_DEL, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, ""},
    {"1", "2", "3", "\n",
     "4", "5", "6", "\n",
     "7", "8", "9", "\n",
     LV_SYMBOL_OK, LOC_KEYBOARD_DEL, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, ""},
    {"0", "+", "-", "\n",
     "/", "*", "=", "\n",
     "!", "?", "#", "\n",
     LV_SYMBOL_OK, LOC_KEYBOARD_DEL, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, ""},
    {"<", ">", "@", "\n",
     "%", "$", "(", "\n",
     ")", "{", "}", "\n",
     LV_SYMBOL_OK, LOC_KEYBOARD_DEL, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, ""},
    {"[", "]", ";", "\n",
     "\"", "'", ".", "\n",
     ",", ":", " ", "\n",
     LV_SYMBOL_OK, LOC_KEYBOARD_DEL, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, ""}};
