#pragma once
#include <ArduinoJson.h>
#include "../config.h"

class Component
{
    public:
        void virtual load(const JsonObject &json);
        void virtual update(const JsonVariant &update);
        void virtual destroy();
        lv_obj_t* get_obj()
        {
            return obj_;
        }
    protected:
        Component(lv_obj_t*parent)
        {
            parent_ = parent;
        }

        lv_obj_t*obj_ = NULL;
        lv_obj_t*parent_ = NULL;
};