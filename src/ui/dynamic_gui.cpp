#include "dynamic_gui.h"
#include "SPIFFS.h"
#include "ArduinoJson.h"
#include "default_view_json.h"

const char *DGUI_TAG = "DGUI";

DynamicGui::DynamicGui()
{
    factory = new ComponentFactory();
}

void DynamicGui::initialize_builders()
{
    DynamicLabelBuilder::initialize(factory);
}

bool DynamicGui::load_file(String path, lv_obj_t *parent, int &count)
{
    bool ret = false;

    DynamicJsonDocument uiJson(10240);
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
            newView->Load(parent, viewJson, factory);
            this->views.push_back(newView);
            lv_obj_set_pos(newView->get_obj(), x * LV_HOR_RES, 0);
            lv_tileview_add_element(parent, newView->get_obj());
        }

        count = x;
        ESP_LOGI(DGUI_TAG, "Loaded %d views.", count);
        ret = true;
    }
    else
    {
        ESP_LOGW(DGUI_TAG, "Failed to load dynamic GUI file %s, error = %s!", path.c_str(), result.c_str());
    }

    return ret;
}