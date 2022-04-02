#include "dynamic_gauge.h"
#include "localization.h"
#include "data_adapter.h"

void DynamicGaugeBuilder::initialize(ComponentFactory *factory)
{
    factory->register_constructor("gauge", [factory](JsonObject &json, lv_obj_t *parent) -> Component *
                                  {
                                      auto gauge = new DynamicGauge(parent);
                                      gauge->load(json);
                                      return gauge;
                                  });
}

void DynamicGauge::load(const JsonObject &json)
{
    lv_obj_t *arc = lv_arc_create(parent_, NULL);

    this->obj_ = arc;
    
    lv_arc_set_range(arc, 0, 300);
    lv_arc_set_bg_angles(arc, 0, 300);
    lv_arc_set_angles(arc, 0, 300);
    lv_arc_set_rotation(arc, 120);
    lv_arc_set_value(arc, 20);
    lv_arc_set_adjustable(arc, false);
    lv_obj_set_click(arc, false);
    //remove border
    lv_obj_set_style_local_pad_all(arc, LV_ARC_PART_BG, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_border_side(arc, LV_ARC_PART_BG, LV_STATE_DEFAULT, LV_BORDER_SIDE_NONE);
    //set opacity to transparent
    lv_obj_set_style_local_bg_opa(arc, LV_ARC_PART_BG, LV_STATE_DEFAULT, LV_OPA_TRANSP);

    if (json.containsKey("color"))
    {
        auto color = DynamicHelpers::get_color(json["color"]);
        lv_obj_set_style_local_line_color(arc, LV_ARC_PART_INDIC, LV_STATE_DEFAULT, color);
    }

    if(json.containsKey("minimum"))
    {
        minimum_ = json["minimum"].as<float>();
    }

    if(json.containsKey("maximum"))
    {
        maximum_ = json["maximum"].as<float>();
    }

    if (json.containsKey("binding"))
    {
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
            //allocate memory for format string + 1 for \0 char at the end
            formating.string_format = (char *)malloc(strlen(jsonFormating) + 1);
            strcpy(formating.string_format, jsonFormating);
        }

        //register dataadapter that will connect SK receiver and this arc
        new DataAdapter(binding["path"].as<String>(), period, this);
    }

    DynamicHelpers::set_location(arc, json);
    DynamicHelpers::set_size(arc, json);
    DynamicHelpers::set_layout(arc, parent_, json);

    label_ = lv_label_create(arc, NULL);
    lv_label_set_text(label_, "--");
    lv_obj_align(label_, arc, LV_ALIGN_CENTER, 0, 0);

    if(json.containsKey("text-color"))
    {
        auto textColor = DynamicHelpers::get_color(json["text-color"]);

        lv_obj_set_style_local_text_color(label_, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, textColor);
    }
}

void DynamicGauge::update(const JsonVariant &json)
{
    float value = 0.0f;

    if (json.is<int>())
    {
        value = ((json.as<int>() + formating.offset) * formating.multiply);
    }
    else if (json.is<float>())
    {
        value = ((json.as<float>() + formating.offset) * formating.multiply);
    }
    else if (json.is<bool>())
    {
        value = json.as<bool>() ? minimum_ : maximum_;
    }
    else
    {
        value = minimum_;
    }

    lv_arc_set_value(obj_, (int)(300.0f * ((value - minimum_) / (maximum_ - minimum_))));

    if (formating.string_format != NULL)
    {
        String labelText = String(formating.string_format);
        labelText.replace("$$", String(value, formating.decimal_places));
        lv_label_set_text(label_, labelText.c_str());
    }
    else
    {
        String labelText = String(value,1);
        lv_label_set_text(obj_, labelText.c_str());
    }
    lv_obj_align(label_, obj_, LV_ALIGN_CENTER, 0, 0);
}

void DynamicGauge::on_offline()
{
    lv_arc_set_value(obj_, 0);
    lv_label_set_text(label_, "--");
    lv_obj_align(label_, obj_, LV_ALIGN_CENTER, 0, 0);
}

void DynamicGauge::destroy()
{
    if (label_ != NULL)
    {
        lv_obj_del(label_);
        label_ = NULL;
    }

    if (obj_ != NULL)
    {
        lv_obj_del(obj_);
        obj_ = NULL;
    }
}