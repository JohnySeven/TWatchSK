#pragma once
#include "../config.h"

namespace twatchsk
{

/** This file defines variables and functions for manipulating graphic elements of
 * the MATERIAL theme that's used for TWatchSK. They are declared in the twatchsk
 * namespace so that they can be used anwywhere throught the TWatchSK project.
 **/

    static bool dark_theme_enabled = false;

    static lv_color_t get_text_color()
    {
        if(dark_theme_enabled)
        {
            return LV_COLOR_WHITE;
        }
        else
        {
            return LV_COLOR_BLACK;
        }
    }

    static void update_imgbtn_color(lv_obj_t *button)
    {
        if (dark_theme_enabled)
        {
            lv_obj_set_style_local_image_recolor_opa(button, LV_OBJ_PART_MAIN, LV_BTN_STATE_PRESSED, LV_OPA_COVER);
            lv_obj_set_style_local_image_recolor_opa(button, LV_OBJ_PART_MAIN, LV_BTN_STATE_RELEASED, LV_OPA_COVER);
            lv_obj_set_style_local_image_recolor(button, LV_OBJ_PART_MAIN, LV_BTN_STATE_PRESSED, LV_COLOR_WHITE);
            lv_obj_set_style_local_image_recolor(button, LV_OBJ_PART_MAIN, LV_BTN_STATE_RELEASED, LV_COLOR_WHITE);
        }
        else
        {
            lv_obj_set_style_local_image_recolor_opa(button, LV_OBJ_PART_MAIN, LV_BTN_STATE_PRESSED, LV_OPA_COVER);
            lv_obj_set_style_local_image_recolor_opa(button, LV_OBJ_PART_MAIN, LV_BTN_STATE_RELEASED, LV_OPA_COVER);
            lv_obj_set_style_local_image_recolor(button, LV_OBJ_PART_MAIN, LV_BTN_STATE_PRESSED, LV_COLOR_BLACK);
            lv_obj_set_style_local_image_recolor(button, LV_OBJ_PART_MAIN, LV_BTN_STATE_RELEASED, LV_COLOR_BLACK);
        }
    }

} // namespace twatchsk