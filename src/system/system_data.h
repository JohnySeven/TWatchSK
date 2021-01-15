#pragma once
#include "system/configurable.h"

/** 
 * @brief Used to store and retrieve various system-wide bits of info.
 **/  

class SystemData : public Configurable, public SystemObject
{
    public:
        SystemData();
        int get_screen_timeout() { return screen_timeout_; }
        void set_screen_timeout(int new_value) { screen_timeout_ = new_value; }
        
    private:
        int screen_timeout_ = 10; // multiply by 1000 to make this "seconds" 
        void load_config_from_file(const JsonObject &json) override;
        void save_config_to_file(JsonObject &json) override;
};