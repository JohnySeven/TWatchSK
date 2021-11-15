#include "dynamic_gui.h"
#include "SPIFFS.h"
#include "ArduinoJson.h"
#include "default_view_json.h"
#include "json.h"
#include "networking/signalk_socket.h"
#include "networking/signalk_subscription.h"
#include "data_adapter.h"

#include "dynamic_label.h"
#include "dynamic_gauge.h"
#include "dynamic_switch.h"

const char *DGUI_TAG = "DGUI";

DynamicGui::DynamicGui()
{
    factory = new ComponentFactory();
}

void DynamicGui::initialize()
{
    DynamicLabelBuilder::initialize(factory);
    DynamicGaugeBuilder::initialize(factory);
    DynamicSwitchBuilder::initialize(factory);
}

bool DynamicGui::load_file(String path, lv_obj_t *parent, SignalKSocket *socket, int &count)
{
    bool ret = false;

    SpiRamJsonDocument uiJson(20240); //allocate 20 kB in SPI RAM for JSON parsing
    DeserializationError result;

    if (SPIFFS.exists(path))
    {
        auto file = SPIFFS.open(path);
        result = deserializeJson(uiJson, file);
    }
    else
    {
        ESP_LOGW(DGUI_TAG, "Dynamic GUI file %s definition not found, loading default views!", path.c_str());
        result = deserializeJson(uiJson, JSON_default_view);
    }

    if (result == DeserializationError::Ok)
    {
        JsonArray views = uiJson["views"].as<JsonArray>();
        int x = 0;

        for (JsonObject viewJson : views)
        {
            x++;
            DynamicView *newView = new DynamicView();
            newView->load(parent, viewJson, factory);
            this->views.push_back(newView);
            lv_obj_set_pos(newView->get_obj(), x * LV_HOR_RES, 0);
            lv_tileview_add_element(parent, newView->get_obj());
        }

        count = x;
        ESP_LOGI(DGUI_TAG, "Loaded %d views.", count);

        for (auto adapter : DataAdapter::get_adapters())
        {
            adapter->initialize(socket);
        }
        
        ret = true;
    }
    else
    {
        ESP_LOGW(DGUI_TAG, "Failed to load dynamic GUI file %s, error = %s!", path.c_str(), result.c_str());
    }

    return ret;
}

void DynamicGui::handle_signalk_update(const String& path, const JsonVariant &value)
{
    for (auto adapter : DataAdapter::get_adapters())
    {
        if (adapter->get_path() == path)
        {
            adapter->on_updated(value);
        }
    }
}