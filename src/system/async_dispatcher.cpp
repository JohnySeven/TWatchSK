#include "async_dispatcher.h"
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <string.h>

QueueHandle_t dispatcher_queue_handle = NULL;
TaskHandle_t dispatcher_task = NULL;
const char *ASYNC_TAG = "ASYNC";

/** This function is receiving tasks from queue and executing them one by one
 */
void dispatcher_task_func(void *pvParameter)
{
    AsyncTask_t currentTask;

    ESP_LOGI(ASYNC_TAG, "Async task dispatcher started!");

    while (true)
    {
        if (xQueueReceive(dispatcher_queue_handle, &currentTask, portMAX_DELAY))
        {
            ESP_LOGI(ASYNC_TAG, "Starting task %s", currentTask.name);
            currentTask.callback();
            ESP_LOGI(ASYNC_TAG, "Task finished %s", currentTask.name);
        }
    }
}

/** Intialize async call dispatcher that allows serialized async method running
 */
void twatchsk::initialize_async()
{
    ESP_LOGI(ASYNC_TAG, "Initializing async task dispatcher...");
    dispatcher_queue_handle = xQueueCreate(32, sizeof(AsyncTask_t));
    xTaskCreate(dispatcher_task_func, "async", CONFIG_MAIN_TASK_STACK_SIZE, NULL, 5, &dispatcher_task);
}

/** This method adds new task in async dispatcher queue, it will be run when all queued tasks prior this call are completed
 * @param name - name of the task (will be visible in log - start / stop)
 * @param function - lambda or std::binded function that will be executed on 1 code of ESP32
 */
void twatchsk::run_async(const char *name, std::function<void(void)> function)
{
    AsyncTask_t task;
    strcpy(task.name, name);
    task.callback = function;
    xQueueSend(dispatcher_queue_handle, &task, portMAX_DELAY);
}