#include "dynamic_label.h"
#include "localization.h"
#include "data_adapter.h"

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
    factory->register_constructor("label", [factory](JsonObject &json, lv_obj_t *parent) -> Component *
                                  {
                                      auto label = new DynamicLabel(parent);
                                      label->load(json);
                                      return label; });
}

void DynamicLabel::load(const JsonObject &json)
{
    lv_obj_t *label = lv_label_create(parent_, NULL);

    this->obj_ = label;

    if (json.containsKey("color"))
    {
        auto color = DynamicHelpers::get_color(json["color"]);
        lv_obj_set_style_local_text_color(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color);
    }

    auto textSet = false;

    if (json.containsKey("text"))
    {
        textSet = true;
        lv_label_set_text(label, json["text"].as<String>().c_str());
    }

    if (json.containsKey("binding"))
    {
        has_binding_ = true;
        JsonObject binding = json["binding"].as<JsonObject>();
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

        if (binding.containsKey("format"))
        {
            auto jsonFormating = binding["format"].as<char *>();
            // allocate memory for format string + 1 for \0 char at the end
            formating.string_format = (char *)malloc(strlen(jsonFormating) + 1);
            strcpy(formating.string_format, jsonFormating);
        }
        // register dataadapter that will connect SK receiver and this label
        new DataAdapter(binding["path"].as<String>(), period, this);

        if (!textSet)
        {
            lv_label_set_text(label, "--");
        }
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
    DynamicHelpers::set_layout(label, parent_, json);
}

void DynamicLabel::update(const JsonVariant &value)
{
    String stringValue;

    if (value.is<String>())
    {
        stringValue = value.as<String>();
    }
    else if (value.is<int>())
    {
        stringValue = String(value.as<int>());
    }
    else if (value.is<float>())
    {
        stringValue = String((value.as<float>() * formating.multiply) + formating.offset, formating.decimal_places);
    }
    else if (value.is<bool>())
    {
        stringValue = String(value.as<bool>() ? LOC_TRUE : LOC_FALSE);
    }
    else
    {
        String json;
        serializeJson(value, json);
        stringValue = json;
    }

    if (formating.string_format != NULL)
    {
        String text = String(formating.string_format);
        text.replace("$$", stringValue.c_str());
        lv_label_set_text(obj_, text.c_str());
    }
    else
    {
        lv_label_set_text(obj_, stringValue.c_str());
    }
}

void DynamicLabel::on_offline()
{
    if (has_binding_)
    {
        String stringValue = "--";

        if (formating.string_format != NULL)
        {
            String text = String(formating.string_format);
            text.replace("$$", stringValue.c_str());
            lv_label_set_text(obj_, text.c_str());
        }
        else
        {
            lv_label_set_text(obj_, stringValue.c_str());
        }
    }
}

void DynamicLabel::destroy()
{
    if (obj_ != NULL)
    {
        lv_obj_del(obj_);
        obj_ = NULL;
    }
}