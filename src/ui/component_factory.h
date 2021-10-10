#pragma once
#include "../config.h"
#include "map"
#include "vector"
#include "ArduinoJson.h"
#include "functional"
#include "data_adapter.h"
#include "component.h"

class ComponentFactory
{
    public:
        Component*create_component(JsonObject& componentJson, lv_obj_t*parent);
        void layout_component(String layoutType, lv_obj_t* parent, lv_obj_t*obj);
        void register_constructor(String name, std::function<Component*(JsonObject&,lv_obj_t*)> factoryFunc);
    private:
        std::map<String, std::function<Component*(JsonObject&,lv_obj_t*)>> componentConstructors;
};