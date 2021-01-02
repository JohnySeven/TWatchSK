#include "events.h"

QueueHandle_t g_event_queue_handle = NULL;
EventGroupHandle_t g_app_state = NULL;
QueueHandle_t gui_queue_handle = NULL;

void initialize_events()
{
    //Create a program that allows the required message objects and group flags
    g_event_queue_handle = xQueueCreate(20, sizeof(uint8_t));
    gui_queue_handle = xQueueCreate(60, sizeof(GuiEvent_t));
    g_app_state = xEventGroupCreate();
}

void post_event(ApplicationEvents_T event)
{
    xQueueSend(g_event_queue_handle, &event, 10);
}

void post_gui_update(GuiEvent_t event)
{
    if (event.event == GuiEventType_t::GUI_SHOW_WARNING && is_low_power())
    {
        xEventGroupSetBits(g_app_state, G_APP_STATE_WAKE_UP);
    }

    ESP_LOGI("GUI", "EVENT %d", event.event);
    xQueueSend(gui_queue_handle, &event, 10);
}

bool read_gui_update(GuiEvent_t &event)
{
    return xQueueReceive(gui_queue_handle, &event, 10);
}

bool is_low_power()
{
    return xEventGroupGetBits(g_app_state) & G_APP_STATE_LOW_POWER;
}

void set_low_power(bool low_power)
{
    if (low_power)
    {
        xEventGroupSetBits(g_app_state, G_APP_STATE_LOW_POWER);
    }
    else
    {
        xEventGroupClearBits(g_app_state, G_APP_STATE_LOW_POWER);
    }
}