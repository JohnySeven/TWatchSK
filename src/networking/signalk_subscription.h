#pragma once
#include "Arduino.h"

class SignalKSubscription
{
    public:
        SignalKSubscription(String subscription_path, uint subscription_period = 1000, bool subscription_low_power = false)
        {
            path = subscription_path;
            period = subscription_period;
            is_low_power = subscription_low_power;
        }
        bool get_low_power() { return is_low_power; }
        String get_path() { return path; }
        uint get_period() { return period; }
    private:
        String path = "";
        uint period = 1000;
        bool is_low_power = false;
};