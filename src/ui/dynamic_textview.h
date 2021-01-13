#pragma once
#include "component_factory.h"
#include "dynamic_helpers.h"

class DynamicTextViewBuilder
{
public:
    static void initialize(ComponentFactory *factory)
    {
        factory->registerConstructor("textview", [](JsonObject& json, lv_obj_t *parent) -> lv_obj_t * {
            lv_obj_t* label = lv_label_create(parent, NULL);
            DynamicHelpers::set_layout(label, parent, json["layout"].as<String>());

            return label;
        });
    }

private:
    DynamicTextViewBuilder() {}
};