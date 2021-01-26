#include "dynamic_label.h"
#include "localization.h"

LV_FONT_DECLARE(Geometr);
LV_FONT_DECLARE(Ubuntu);
LV_FONT_DECLARE(roboto80);
LV_FONT_DECLARE(roboto60);
LV_FONT_DECLARE(roboto40);
LV_FONT_DECLARE(lv_font_montserrat_14)
LV_FONT_DECLARE(lv_font_montserrat_28)
LV_FONT_DECLARE(lv_font_montserrat_32)

void DynamicLabelBuilder::initialize(ComponentFactory *factory)
{
    factory->register_constructor("label", [factory](JsonObject &json, lv_obj_t *parent) -> lv_obj_t * {
        lv_obj_t *label = lv_label_create(parent, NULL);

        if (json.containsKey("color"))
        {
            auto color = DynamicHelpers::get_color(json["color"]);
            lv_obj_set_style_local_text_color(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color);
        }

        if (json.containsKey("text"))
        {
            lv_label_set_text(label, json["text"].as<String>().c_str());
        }
        else if (json.containsKey("binding"))
        {
            JsonObject binding = json["binding"].as<JsonObject>();
            Data_formating_t formating;
            int period = 1000;
            formating.multiply = 1.0f;
            formating.offset = 0.0f;
            formating.decimal_places = 1;

            if (binding.containsKey("period"))
            {
                period = binding["period"].as<int>();
            }
            if (binding.containsKey("multiply"))
            {
                formating.multiply = binding["multiply"].as<float>();
            }
            if (binding.containsKey("offset"))
            {
                formating.offset = binding["offset"].as<float>();
            }

            if (binding.containsKey("decimals"))
            {
                formating.decimal_places = binding["decimals"].as<int>();
            }

            factory->add_data_adapter(binding["path"].as<String>(), period, formating, [label](const JsonVariant &value, const Data_formating_t &format) {
                if (value.is<String>())
                {
                    lv_label_set_text(label, value.as<String>().c_str());
                }
                else if (value.is<int>())
                {
                    auto intValue = String(value.as<int>());
                    lv_label_set_text(label, intValue.c_str());
                }
                else if (value.is<float>())
                {
                    auto floatValue = String((value.as<float>() + format.offset) * format.multiply, format.decimal_places);

                    lv_label_set_text(label, floatValue.c_str());
                }
                else if (value.is<bool>())
                {
                    lv_label_set_text(label, value.as<bool>() ? LOC_TRUE : LOC_FALSE);
                }
                else
                {
                    String json;
                    serializeJson(value, json);
                    lv_label_set_text(label, json.c_str());
                }
            });
            lv_label_set_text(label, "--");
        }

        if (json.containsKey("font"))
        {
            auto styleName = json["font"].as<String>();
            if (styleName == "montserrat14")
            {
                lv_obj_set_style_local_text_font(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_14);
            }
            else if (styleName == "montserrat28")
            {
                lv_obj_set_style_local_text_font(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_28);
            }
            else if (styleName == "montserrat32")
            {
                lv_obj_set_style_local_text_font(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_32);
            }
            else if (styleName == "ubuntu50")
            {
                lv_obj_set_style_local_text_font(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &Ubuntu);
            }
            else if (styleName == "roboto40")
            {
                lv_obj_set_style_local_text_font(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &roboto40);
            }
            else if (styleName == "roboto60")
            {
                lv_obj_set_style_local_text_font(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &roboto60);
            }
            else if (styleName == "roboto80")
            {
                lv_obj_set_style_local_text_font(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &roboto80);
            }
            else
            {
                ESP_LOGW("LABEL", "Font %s not found!", styleName.c_str());
            }
        }

        DynamicHelpers::set_location(label, json);
        DynamicHelpers::set_size(label, json);
        DynamicHelpers::set_layout(label, parent, json);

        ESP_LOGI("LABEL", "Parent size %d;%d, Label %d;%d", lv_obj_get_width(parent), lv_obj_get_height(parent), lv_obj_get_width(label), lv_obj_get_height(label));

        return label;
    });
}