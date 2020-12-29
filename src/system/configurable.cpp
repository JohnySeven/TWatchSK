#include "configurable.h"
const char TAG[] = "CONFIG";

Configurable::Configurable(String path)
{
    this->file_path = path;
}

void Configurable::load()
{
    StaticJsonDocument<512> doc;
    auto exists = SPIFFS.exists(file_path);
    ESP_LOGI(TAG, "Loading config %s (exists=%d)", file_path.c_str(), exists);

    if (exists)
    {
        auto file = SPIFFS.open(file_path);
        ESP_LOGI(TAG, "Config %s size=%d", file_path.c_str(), file.available());
        auto error = deserializeJson(doc, file);
        if(error == DeserializationError::Ok)
        {
            load_config_from_file(doc.as<JsonObject>());
        }
        else
        {
            ESP_LOGI(TAG, "Failed to deserialize config %s! Error=%d", file_path.c_str(), (int)error.code());            
        }
        
        file.close();
    }
}

void Configurable::save()
{
    StaticJsonDocument<512> doc;
    auto file = SPIFFS.open(file_path, "w");
    JsonObject obj = doc.createNestedObject("root");
    save_config_to_file(obj);
    serializeJson(obj, file);
    file.flush();
    file.close();
}

void Configurable::load_config_from_file(const JsonObject &json)
{
    ESP_LOGW(TAG, "load_config_from_file not overriden for %s", file_path.c_str());
}

void Configurable::save_config_to_file(JsonObject &json)
{
    ESP_LOGW(TAG, "save_config_to_file not overriden for %s", file_path.c_str());
}