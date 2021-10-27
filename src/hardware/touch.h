#pragma once
#include "../config.h"
#include "FreeRTOS.h"

typedef struct {
        bool touched;
        int16_t x_coor;
        int16_t y_coor;
    } touch_point;


class Touch
{
    public:
        bool initialize(EventGroupHandle_t wakeupEvents);
        void set_low_power(bool low_power);
        void allow_touch_wakeup(bool value);

};