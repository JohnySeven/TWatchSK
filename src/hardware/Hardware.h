#pragma once
#include "../config.h"
#include "system/configurable.h"
#include <functional>
#include <vector>

enum PowerCode_t
{
    POWER_ENTER_LOW_POWER,
    POWER_LEAVE_LOW_POWER,
    POWER_CHARGING_ON,
    POWER_CHARGING_OFF,
    POWER_CHARGING_DONE,
    WALK_STEP_COUNTER_UPDATED,
};

typedef std::function<void(PowerCode_t, uint32_t)> low_power_callback;
/**
 * @brief Hardware class purpose is to handle all hardware features of the watch, manly BMA interuptions, sound & vibrate stuff (comming soon)
 *  
 * @param title The title at the top of the page. Set in localization.h
 * 
 * @param type What the roller is used to select. To add a type, add it to the 
 * enum just below, and define it similar to the private data member timezone_option_list_.
 * 
 * @param starting_id The internal ID of the item in the list that you want to be displayed
 * when theh roller is created. 0 is the first item in the list, 1 is the second, etc.
 * 
 * All of the code to set, save, and retrieve the timezone uses the internal ID. Only the
 * display of the timezone to the user converts it to human-friendly format. For example,
 * timezone ID 0 is displayed as "GMT-12:00". This is done with a function in time_settings.h
 * called get_timezone_string().
 **/
class Hardware : public Configurable
{
public:
    Hardware();

    void load_config_from_file(const JsonObject &json) override;
    void save_config_to_file(JsonObject &json) override;

    bool get_tilt_wakeup() { return tilt_wakeup_; }
    void set_tilt_wakeup(bool value) { tilt_wakeup_ = value; update_bma_wakeup(); }

    bool get_double_tap_wakeup() { return double_tap_wakeup_; }
    void set_double_tap_wakeup(bool value) { double_tap_wakeup_ = value; update_bma_wakeup(); }


    void initialize(TTGOClass *watch);
    void attach_power_callback(low_power_callback callback) { power_callbacks_.push_back(callback); }
    void loop();
    void set_screen_timeout_func(std::function<uint32_t(void)> func) { get_screen_timeout_ = func; }
private:
    std::vector<low_power_callback> power_callbacks_;
    std::function<uint32_t(void)> get_screen_timeout_;
    bool double_tap_wakeup_ = false;
    bool tilt_wakeup_ = false;
    TTGOClass *watch_;
    bool lenergy_ = false;
    bool light_sleep_ = false;
    void low_energy();
    void invoke_power_callbacks(PowerCode_t code, uint32_t arg);
    void update_bma_wakeup();
};