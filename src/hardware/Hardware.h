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

enum WakeupSource_t
{
    WAKEUP_BUTTON,
    WAKEUP_ACCELEROMETER,
    WAKEUP_TOUCH
};

typedef std::function<void(PowerCode_t, uint32_t)> low_power_callback;
/**
 * @brief Hardware class purpose is to handle all hardware features of the watch, power managment setup, power event callbacks and BMA interuptions, sound & vibrate stuff (comming soon)
 **/
class Hardware : public Configurable
{
public:
    Hardware();

    void load_config_from_file(const JsonObject &json) override;
    void save_config_to_file(JsonObject &json) override;

    bool get_tilt_wakeup()
    {
        return tilt_wakeup_;
    }
    void set_tilt_wakeup(bool value)
    {
        tilt_wakeup_ = value;
        update_bma_wakeup();
    }
    bool get_double_tap_wakeup()
    {
        return double_tap_wakeup_;
    }
    void set_double_tap_wakeup(bool value)
    {
        double_tap_wakeup_ = value;
        update_bma_wakeup();
    }

    void initialize(TTGOClass *watch);
    void attach_power_callback(low_power_callback callback)
    {
        power_callbacks_.push_back(callback);
    }

    void loop();
    void set_screen_timeout_func(std::function<uint32_t(void)> func)
    {
        get_screen_timeout_ = func;
    }
    void vibrate(bool status);
private:
    std::vector<low_power_callback> power_callbacks_;
    std::function<uint32_t(void)> get_screen_timeout_;
    bool double_tap_wakeup_ = false;
    bool tilt_wakeup_ = false;
    TTGOClass *watch_;
    bool lenergy_ = false;
    bool light_sleep_ = false;
    bool is_vibrating_ = false;
    void low_energy();
    void invoke_power_callbacks(PowerCode_t code, uint32_t arg);
    void update_bma_wakeup();
};