#pragma once
#include "../config.h"
#include "map"
#include "vector"
#include "ArduinoJson.h"
#include "functional"
#include "data_adapter.h"

class ComponentFactory
{
    public:
        lv_obj_t*create_component(JsonObject& componentJson, lv_obj_t*parent);
        void layout_component(String layoutType, lv_obj_t* parent, lv_obj_t*obj);
        void register_constructor(String name, std::function<lv_obj_t*(JsonObject&,lv_obj_t*)> factoryFunc);
        void add_data_adapter(String path, int subscription_period, Data_formating_t formating, adapter_callback_t callback);
        std::vector<DataAdapter*>& get_adapters() { return adapters; }
    private:
        std::map<String, std::function<lv_obj_t*(JsonObject&,lv_obj_t*)>> componentConstructors;
        std::vector<DataAdapter*> adapters;
};