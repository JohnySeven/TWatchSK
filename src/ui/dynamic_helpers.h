#pragma once
#include "../config.h"
#include <Arduino.h>
#include "ArduinoJson.h"

class DynamicHelpers
{
public:
    static void set_layout(lv_obj_t *obj, lv_obj_t *parent, JsonObject&json);
    static void set_location(lv_obj_t*obj, JsonObject&json);
    static lv_color_t get_color(const String&value);
private:
    DynamicHelpers();
    static uint8_t get_alignment(String value);   
};