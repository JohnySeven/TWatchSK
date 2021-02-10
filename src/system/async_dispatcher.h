#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <functional>

struct AsyncTask_t
{
    char name[32];
    std::function<void(void)> callback;
};

namespace twatchsk
{
    void run_async(const char *name, std::function<void(void)> function);
    void initialize_async();
}