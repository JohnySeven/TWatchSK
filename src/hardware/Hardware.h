#pragma once
#include "../config.h"
#include "system/configurable.h"
#include "ui/themes.h"
#include <functional>
#include <vector>
#include "system/async_dispatcher.h"
#include "sounds/sound_player.h"
#include "Hardware/Touch.h"

enum PowerCode_t
{
    POWER_ENTER_LOW_POWER,
    POWER_LEAVE_LOW_POWER,
    POWER_CHARGING_ON,
    POWER_CHARGING_OFF,
    POWER_CHARGING_DONE,
    WALK_STEP_COUNTER_UPDATED,
    POWER_LOW_TICK,
    DOUBLE_TAP_DETECTED
};

enum WakeupSource_t
{
    WAKEUP_BUTTON,
    WAKEUP_ACCELEROMETER,
    WAKEUP_TOUCH
};

extern void IRAM_ATTR wakeup_from_isr();

typedef std::function<void(PowerCode_t, uint32_t)> low_power_callback;
/**
 * @brief Hardware class purpose is to handle all hardware features of the watch, power managment setup,
 * power event callbacks and BMA interrupts, sound & vibrate stuff (coming soon)
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
    void vibrate(int duration);
    void vibrate(int pattern[], int repeat = 1);
    void intialize_touch();
    
    SoundPlayer*get_player()
    {
        return player_;
    }
private:
    std::vector<low_power_callback> power_callbacks_;
    std::function<uint32_t(void)> get_screen_timeout_;
    bool double_tap_wakeup_ = false;
    bool tilt_wakeup_ = false;
    TTGOClass *watch_;
    bool lenergy_ = false;
    bool is_vibrating_ = false;
    void low_energy();
    void invoke_power_callbacks(PowerCode_t code, uint32_t arg);
    void update_bma_wakeup();
    void vibrate(bool status);
    SoundPlayer*player_ = NULL;
    Touch*touch_ = NULL;
};