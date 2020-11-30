#pragma once
#include "config.h"

class Loader
{
public:
    Loader(String text)
    {
        static lv_style_t plStyle;
        lv_style_init(&plStyle);
        lv_style_set_radius(&plStyle, LV_OBJ_PART_MAIN, 0);
        lv_style_set_border_width(&plStyle, LV_OBJ_PART_MAIN, 0);
        lv_style_set_bg_color(&plStyle, LV_OBJ_PART_MAIN, LV_COLOR_GRAY);
        lv_style_set_text_color(&plStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);
        lv_style_set_image_recolor(&plStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);

        static lv_style_t style;
        lv_style_init(&style);
        lv_style_set_radius(&style, LV_OBJ_PART_MAIN, 0);
        lv_style_set_border_width(&style, LV_OBJ_PART_MAIN, 0);
        lv_style_set_text_color(&style, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);
        lv_style_set_image_recolor(&style, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);

        loaderContainer = lv_cont_create(lv_scr_act(), NULL);
        lv_obj_set_pos(loaderContainer, 0, 0);
        lv_obj_set_size(loaderContainer, LV_HOR_RES, LV_VER_RES);
        lv_obj_add_style(loaderContainer, LV_OBJ_PART_MAIN, &plStyle);
        lv_cont_set_layout(loaderContainer, LV_LAYOUT_COLUMN_MID);

        lv_obj_t *preload = lv_spinner_create(loaderContainer, NULL);
        lv_obj_set_size(preload, lv_obj_get_width(loaderContainer) / 2, lv_obj_get_height(loaderContainer) / 2);
        lv_obj_add_style(preload, LV_OBJ_PART_MAIN, &style);
        
        lv_obj_t *label = lv_label_create(loaderContainer, NULL);
        lv_label_set_text(label, text.c_str());
        lv_obj_add_style(preload, LV_OBJ_PART_MAIN, &style);
        ESP_LOGI("LOADER", "Loader loaded %s", text.c_str());
    }

    void update_status(String status)
    {
        lv_label_set_text(label, status.c_str());
    }

    ~Loader()
    {
        if (loaderContainer != NULL)
        {
            lv_obj_del(loaderContainer);
            loaderContainer = NULL;
            label = NULL;
            ESP_LOGI("LOADER", "Loader destroyed");
        }
    }

private:
    lv_obj_t *loaderContainer = NULL;
    lv_obj_t *label = NULL;
};