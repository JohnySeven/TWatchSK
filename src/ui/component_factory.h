#pragma once
#include "../config.h"
#include "map"
#include "ArduinoJson.h"
#include "functional"

class ComponentFactory
{
    public:
        lv_obj_t*createComponent(JsonObject& componentJson, lv_obj_t*parent);

        void layoutComponent(String layoutType, lv_obj_t* parent, lv_obj_t*obj);

        void registerConstructor(String name, std::function<lv_obj_t*(JsonObject&,lv_obj_t*)> factoryFunc);
    private:
        std::map<String, std::function<lv_obj_t*(JsonObject&,lv_obj_t*)>> componentConstructors;
};