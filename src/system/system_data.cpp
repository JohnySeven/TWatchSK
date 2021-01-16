#include "system_data.h"
#include "system/systemobject.h"
#include "ui/localization.h"

static const char *SD_TAG = "SD";

SystemData::SystemData() : Configurable("/config/system_data")
{
    load();
}

void SystemData::load_config_from_file(const JsonObject &json)
{
    screen_timeout_ = json["screen_timeout"].as<int>();
    ESP_LOGI(SD_TAG, "Loaded config with screen timeout: %d seconds", screen_timeout_);
}

void SystemData::save_config_to_file(JsonObject &json)
{
    json["screen_timeout"] = screen_timeout_;
    ESP_LOGI(SD_TAG, "Saved screen timeout: %d seconds", screen_timeout_);
}