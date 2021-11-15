#include "dynamic_gauge.h"
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
    lv_obj_t *gauge = lv_gauge_create(parent_, NULL);

    this->obj_ = gauge;

    lv_color_t colors[1];
    colors[0] = lv_theme_get_color_primary();

    if (json.containsKey("color"))
    {
        colors[0] = DynamicHelpers::get_color(json["color"]);
    }

    lv_gauge_set_needle_count(gauge, 1, colors);

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

        if(binding.containsKey("minimum") && binding.containsKey("maximum"))
        {
            lv_gauge_set_range(gauge, binding["minimum"].as<int>(), binding["maximum"].as<int>());
        }

        if(binding.containsKey("critical"))
        {
            lv_gauge_set_critical_value(gauge, binding["critical"].as<int>());
        }

        //register dataadapter that will connect SK receiver and this label
        new DataAdapter(binding["path"].as<String>(), period, this);
    }

    lv_obj_set_click(gauge, false);

    DynamicHelpers::set_location(gauge, json);
    DynamicHelpers::set_size(gauge, json);
    DynamicHelpers::set_layout(gauge, parent_, json);
}

void DynamicGauge::update(const JsonVariant &json)
{
    int value = 0;

    if (json.is<int>())
    {
        value = json.as<int>();
    }
    else if (json.is<float>())
    {
        value = (int)((json.as<float>() + formating.offset) * formating.multiply);
    }
    else if (json.is<bool>())
    {
        value = json.as<bool>() ? 0 : 1;
    }
    else
    {
        value = 0;
    }

    lv_gauge_set_value(obj_, 0, value);
}

void DynamicGauge::destroy()
{
    if (obj_ != NULL)
    {
        lv_obj_del(obj_);
        obj_ = NULL;
    }
}