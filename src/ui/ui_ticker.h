#pragma once
#include <functional>
#include "config.h"
//Uses LVGL timers
class UITicker
{
    public:
        UITicker(uint32_t period_ms, std::function<void()> callback)
        {
            tickHandler = callback;
            task = lv_task_create(__task_handler, period_ms, LV_TASK_PRIO_LOW, this);
        }
        ~UITicker()
        {
            if(task != nullptr)
            {
                lv_task_del(task);
                task = nullptr;
            }
        }
    private:
        lv_task_t*task = nullptr;
        std::function<void()> tickHandler;
        static void __task_handler(lv_task_t * task)
        {
            auto ticker = (UITicker*)task->user_data;
            ticker->tickHandler();
        }
};