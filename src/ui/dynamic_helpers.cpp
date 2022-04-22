#include "dynamic_helpers.h"
LV_FONT_DECLARE(Geometr);
LV_FONT_DECLARE(Ubuntu);
LV_FONT_DECLARE(roboto80);
LV_FONT_DECLARE(roboto60);
LV_FONT_DECLARE(roboto40);
LV_FONT_DECLARE(lv_font_montserrat_14)
LV_FONT_DECLARE(lv_font_montserrat_28)
LV_FONT_DECLARE(lv_font_montserrat_32)

static String alignments[9] = {
    "center",
    "top-left",
    "top-midle",
    "top_right",
    "bottom_left",
    "bottom_mid",
    "bottom_right",
    "left_mid",
    "right_mid"};

static String layouts[12] = {
    "off",
    "center",
    "column_left",   /**< column left align*/
    "column_mid",    /**< column middle align*/
    "column_right",  /**< column right align*/
    "row_top",       /**< row top align*/
    "row_mid",       /**< row middle align*/
    "row_bottom",    /**< row bottom align*/
    "pretty_top",    /**< row top align*/
    "pretty_mid",    /**< row middle align*/
    "pretty_bottom", /**< row bottom align*/
    "grid"};

uint8_t DynamicHelpers::get_alignment(String value)
{
    for (int i = 0; i < 9; i++)
    {
        if (alignments[i].equals(value))
        {
            return (uint8_t)i;
        }
    }

    return LV_ALIGN_CENTER;
}

void DynamicHelpers::set_container_layout(lv_obj_t *obj, String &value)
{
    lv_layout_t layout = LV_LAYOUT_OFF;
    for (int i = 0; i < 12; i++)
    {
        if (layouts[i].equals(value))
        {
            layout = (lv_layout_t)i;
        }
    }
    lv_cont_set_layout(obj, layout);
}

void DynamicHelpers::set_layout(lv_obj_t *obj, lv_obj_t *parent, const JsonObject &json)
{
    if (json.containsKey("layout"))
    {
        String layout = json["layout"].as<String>();
        auto alignment = DynamicHelpers::get_alignment(layout);
        lv_obj_align(obj, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
        ESP_LOGI("UI", "Object layout %s=%d", layout.c_str(), alignment);
    }
}

void DynamicHelpers::set_location(lv_obj_t *obj, const JsonObject &json)
{
    if (json.containsKey("location"))
    {
        JsonArray array = json["location"];
        if (array && array.size() == 2)
        {
            lv_obj_set_pos(obj, array[0].as<int>(), array[1].as<int>());
        }
    }
}

void DynamicHelpers::set_size(lv_obj_t *obj, const JsonObject &json)
{
    if (json.containsKey("size"))
    {
        JsonArray array = json["size"];
        if (array && array.size() == 2)
        {
            lv_obj_set_size(obj, array[0].as<int>(), array[1].as<int>());
        }
    }
}

lv_color_t DynamicHelpers::get_color(const String &value)
{
    lv_color_t ret = LV_COLOR_BLACK;

    if (value.startsWith("#"))
    {
        ret = lv_color_hex(strtol(value.substring(1).c_str(), NULL, 16));
    }
    else if (value == "white")
    {
        ret = LV_COLOR_WHITE;
    }
    else if (value == "black")
    {
        ret = LV_COLOR_BLACK;
    }
    else if (value == "blue")
    {
        ret = LV_COLOR_BLUE;
    }
    else if (value == "red")
    {
        ret = LV_COLOR_RED;
    }
    else if (value == "green")
    {
        ret = LV_COLOR_GREEN;
    }
    else if (value == "gray")
    {
        ret = LV_COLOR_GRAY;
    }
    else if (value == "primary")
    {
        ret = LV_THEME_DEFAULT_COLOR_PRIMARY;
    }
    else if (value == "secondary")
    {
        ret = LV_THEME_DEFAULT_COLOR_SECONDARY;
    }
    else
    {
        ESP_LOGW("HELPERS", "Color %s not found!", value.c_str());
    }

    return ret;
}

void DynamicHelpers::set_font(lv_obj_t *obj, String &styleName)
{
    if (styleName == "montserrat14")
    {
        lv_obj_set_style_local_text_font(obj, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_14);
    }
    else if (styleName == "montserrat28")
    {
        lv_obj_set_style_local_text_font(obj, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_28);
    }
    else if (styleName == "montserrat32")
    {
        lv_obj_set_style_local_text_font(obj, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_32);
    }
    else if (styleName == "ubuntu50")
    {
        lv_obj_set_style_local_text_font(obj, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &Ubuntu);
    }
    else if (styleName == "roboto40")
    {
        lv_obj_set_style_local_text_font(obj, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &roboto40);
    }
    else if (styleName == "roboto60")
    {
        lv_obj_set_style_local_text_font(obj, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &roboto60);
    }
    else if (styleName == "roboto80")
    {
        lv_obj_set_style_local_text_font(obj, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &roboto80);
    }
    else
    {
        ESP_LOGW("LABEL", "Font %s not found!", styleName.c_str());
    }
}