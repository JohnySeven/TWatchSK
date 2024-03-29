#pragma once
#include "../config.h"
#include <Arduino.h>
#include "ArduinoJson.h"

class DynamicHelpers
{
public:
    static void set_layout(lv_obj_t *obj, lv_obj_t *parent, const JsonObject&json);
    static void set_location(lv_obj_t*obj, const JsonObject&json);
    static void set_size(lv_obj_t*obj, const JsonObject&json);
    static lv_color_t get_color(const String&value);
    static void set_container_layout(lv_obj_t*obj, String&value);
    static void set_font(lv_obj_t*obj, String&fontname);
private:
    DynamicHelpers();
    static uint8_t get_alignment(String value);   
};