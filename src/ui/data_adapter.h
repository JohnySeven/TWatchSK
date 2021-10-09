#pragma once
#include "ArduinoJson.h"
#include "config.h"
#include "functional"
#include "component.h"
#include <vector>

struct Data_formating_t
{
    float multiply = 1.0;
    float offset = 0.0;
    int decimal_places = 1;
    char *string_format = NULL;
};

class DataAdapter
{
public:
    DataAdapter(String sk_path, int sk_subscription_period, Component*target);
    const String& get_path() { return path; }
    int get_subscription_period() { return subscription_period; }
    void on_updated(const JsonVariant &value)
    {
        targetObject_->update(value);
    }
    static std::vector<DataAdapter *> & get_adapters();
protected:
    int subscription_period = 0;
    Data_formating_t formating_options_;
    String path = "";
    Component* targetObject_ = NULL;
};
