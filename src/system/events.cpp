#include "events.h"

static void initialize_events()
{
    //Create a program that allows the required message objects and group flags
    g_event_queue_handle = xQueueCreate(20, sizeof(uint8_t));
    g_event_group = xEventGroupCreate();
    isr_group = xEventGroupCreate();
}

static void post_event(ApplicationEvents_T event)
{
    xQueueSend(g_event_queue_handle, &event, 10 / portTICK_PERIOD_MS);
}