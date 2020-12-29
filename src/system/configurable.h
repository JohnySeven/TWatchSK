#pragma once
#include "SPIFFS.h"
#include <ArduinoJson.h>
#include "json.h"

class Configurable
{
    public:
        Configurable(String path);
        void load();
        void save();
    protected:
        virtual void load_config_from_file(const JsonObject& json);
        virtual void save_config_to_file(JsonObject& json);
    private:
        String file_path;
};