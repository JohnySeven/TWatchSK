#include "component_factory.h"

lv_obj_t * ComponentFactory::create_component(JsonObject &componentJson, lv_obj_t *parent)
{
    lv_obj_t *ret = NULL;
    String type = componentJson["type"].as<String>();

    auto it = componentConstructors.find(type);

    if (it != componentConstructors.end())
    {
        ret = it->second(componentJson, parent);
        if (componentJson.containsKey("layout"))
        {
            layout_component(componentJson["layout"].as<String>(), parent, ret);
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

void ComponentFactory::register_constructor(String name, std::function<lv_obj_t *(JsonObject &, lv_obj_t *)> factoryFunc)
{
    componentConstructors[name] = factoryFunc;
}

void ComponentFactory::add_data_adapter(String path, int subscription_period, Data_formating_t formating, adapter_callback_t callback)
{
    auto adapter = new DataAdapter(path, subscription_period, formating, callback);
    adapters.push_back(adapter);
}