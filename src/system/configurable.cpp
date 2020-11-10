#include "configurable.h"

Configurable::Configurable(String path)
{
    this->file_path = path;
    load();
}

void Configurable::load()
{
    ESP_LOGI("CONFIG", "Loading config %s", file_path.c_str());

    SpiRamJsonDocument json(1024);
    if (SPIFFS.exists(file_path))
    {
        auto file = SPIFFS.open(file_path);
        deserializeJson(json, file);
        set_config(json.as<JsonObject>());
        file.close();
    }
    else
    {
        json.clear();
    }
}

void Configurable::save()
{
    ESP_LOGI("CONFIG", "Saving config %s", file_path.c_str());

    DynamicJsonDocument json(1024);
    auto file = SPIFFS.open(file_path, "w");
    get_config(json.as<JsonObject>());
    serializeJson(json, file);
    file.close();
}

void Configurable::get_config(const JsonObject& json)
{

}

void Configurable::set_config(const JsonObject& json)
{

}