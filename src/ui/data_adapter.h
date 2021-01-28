#pragma once
#include "ArduinoJson.h"
#include "config.h"
#include "functional"

struct Data_formating_t
{
    float multiply = 1.0;
    float offset = 0.0;
    int decimal_places = 1;
};

typedef std::function<void(const JsonVariant&, const Data_formating_t&)> adapter_callback_t;

class DataAdapter
{
public:
    DataAdapter(String sk_path, int sk_subscription_period, Data_formating_t formating_options, adapter_callback_t callback)
    {
        handler = callback;
        path = sk_path;
        subscription_period = sk_subscription_period;
        this->formating_options = formating_options;
    }
    const String& get_path() { return path; }
    int get_subscription_period() { return subscription_period; }
    void on_updated(const JsonVariant &value) { handler(value, formating_options); }
protected:
    int subscription_period = 0;
    Data_formating_t formating_options;
    String path = "";
    lv_obj_t* targetObject = NULL;
    adapter_callback_t handler; 
};