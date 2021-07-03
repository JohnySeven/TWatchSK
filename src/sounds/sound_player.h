#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>

class SoundPlayer
{
    public:
        SoundPlayer();
        void play_raw_from_const(const char*name, const unsigned char*raw, int size, int repeat = 1);
        ~SoundPlayer();
    private:
        QueueHandle_t player_queue_handle_ = NULL;
        TaskHandle_t player_task_ = NULL;
        static void player_task_func(void *pvParameter);
};