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
        virtual void get_config(const JsonObject& json);
        virtual void set_config(const JsonObject& json);
    private:
        String file_path;
};