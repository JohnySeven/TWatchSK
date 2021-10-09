#include "component_factory.h"

Component * ComponentFactory::create_component(JsonObject &componentJson, lv_obj_t *parent)
{
    Component *ret = NULL;
    String type = componentJson["type"].as<String>();

    auto it = componentConstructors.find(type);

    if (it != componentConstructors.end())
    {
        ret = it->second(componentJson, parent);
        if (componentJson.containsKey("layout"))
        {
            layout_component(componentJson["layout"].as<String>(), parent, ret->get_obj());
        }
    }
    else
    {
        ESP_LOGI("UI", "Unknown component type %s!", type.c_str());
    }

    return ret;
}

void ComponentFactory::layout_component(String layoutType, lv_obj_t *parent, lv_obj_t *obj)
{
    auto layout = LV_LAYOUT_CENTER;

    lv_obj_align(obj, parent, layout, 0, 0);
}

void ComponentFactory::register_constructor(String name, std::function<Component *(JsonObject &, lv_obj_t*)> factoryFunc)
{
    componentConstructors[name] = factoryFunc;
}