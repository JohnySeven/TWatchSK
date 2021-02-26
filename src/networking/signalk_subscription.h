#pragma once
#include "Arduino.h"

class SignalKSubscription
{
    public:
        SignalKSubscription(String subscription_path, uint subscription_period = 1000, bool subscription_low_power = false)
        {
            path_ = subscription_path;
            period_ = subscription_period;
            is_low_power_ = subscription_low_power;
        }
        bool get_low_power() { return is_low_power_; }
        String get_path() { return path_; }
        uint get_period() { return period_; }
        bool get_active() { return is_active_; }
        void set_active(bool value) { is_active_ = value; }
    private:
        String path_ = "";
        uint period_ = 1000;
        bool is_low_power_ = false;
        bool is_active_ = false;
};