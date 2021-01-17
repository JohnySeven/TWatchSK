#include "dynamic_label.h"
LV_FONT_DECLARE(Geometr);
LV_FONT_DECLARE(Ubuntu);
LV_FONT_DECLARE(roboto80);
LV_FONT_DECLARE(roboto60);
LV_FONT_DECLARE(roboto40);

static lv_style_t ubuntu_font_style;
static lv_style_t roboto_80_style;
static lv_style_t roboto_60_style;
static lv_style_t roboto_40_style;

void DynamicLabelBuilder::initializeStyles()
{
    lv_style_init(&ubuntu_font_style);
    lv_style_set_value_font(&ubuntu_font_style, LV_STATE_DEFAULT, &Ubuntu);

    lv_style_init(&roboto_80_style);
    lv_style_set_value_font(&roboto_80_style, LV_STATE_DEFAULT, &roboto80);

    lv_style_init(&roboto_60_style);
    lv_style_set_value_font(&roboto_80_style, LV_STATE_DEFAULT, &roboto60);

    lv_style_init(&roboto_40_style);
    lv_style_set_value_font(&roboto_80_style, LV_STATE_DEFAULT, &roboto40);
}

void DynamicLabelBuilder::initialize(ComponentFactory *factory)
{
    factory->registerConstructor("label", [](JsonObject &json, lv_obj_t *parent) -> lv_obj_t * {
        lv_obj_t *label = lv_label_create(parent, NULL);
        DynamicHelpers::set_layout(label, parent, json);
        DynamicHelpers::set_location(label, json);

        if (json.containsKey("color"))
        {
            auto color = DynamicHelpers::get_color(json["color"]);
            lv_obj_set_style_local_text_color(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color);
        }

        if(json.containsKey("text"))
        {
            lv_label_set_text(label, json["text"].as<String>().c_str());
        }
        else if(json.containsKey("path"))
        {

        }

        if(json.containsKey("style"))
        {
            auto styleName = json["style"].as<String>();
            lv_style_t* style = NULL;
            if(styleName == "ubuntu50")
            {
                style = &ubuntu_font_style;
            }
            else if(styleName == "roboto40")
            {
                style = &roboto_40_style;
            }
            else if(styleName == "roboto60")
            {
                style = &roboto_60_style;
            }
            else if(styleName == "roboto80")
            {
                style = &roboto_80_style;
            }
            else
            {
                ESP_LOGW("LABEL", "Style %s not found!", styleName.c_str());
            }

            if(style != NULL)
            {
                lv_obj_add_style(label, LV_LABEL_PART_MAIN, style);
            }
        }

        return label;
    });
}