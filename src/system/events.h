#pragma once
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>
#include "Arduino.h"

#define G_EVENT_VBUS_PLUGIN _BV(0)
#define G_EVENT_VBUS_REMOVE _BV(1)
#define G_EVENT_CHARGE_DONE _BV(2)
#define G_EVENT_TOUCH _BV(3)

#define G_APP_STATE_LOW_POWER _BV(0)
#define G_APP_STATE_WAKE_UP _BV(1)

enum ApplicationEvents_T
{
    Q_EVENT_BMA_INT,
    Q_EVENT_AXP_INT,
    Q_EVENT_UI_MESSAGE
};

enum GuiEventType_t
{
    GUI_SHOW_MESSAGE,
    GUI_SHOW_WARNING,
    GUI_SIGNALK_UPDATE
};

enum GuiMessageCode_t
{
    NONE,
    GUI_WARN_SK_REJECTED,
    GUI_WARN_WIFI_DISCONNECTED
};

struct GuiEvent_t
{
    GuiEventType_t event_type;
    void*argument;
    GuiMessageCode_t message_code;
};

extern QueueHandle_t g_event_queue_handle;
extern EventGroupHandle_t g_app_state;

void initialize_events();
void post_event(ApplicationEvents_T event);
void post_gui_signalk_update(const String& json);
void post_gui_warning(GuiMessageCode_t message);
void post_gui_warning(const String& message);
bool read_gui_update(GuiEvent_t& event);
bool is_low_power();
void set_low_power(bool low_power);