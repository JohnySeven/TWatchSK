#include "freertos/queue.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "freertos/timers.h"

#define G_EVENT_VBUS_PLUGIN _BV(0)
#define G_EVENT_VBUS_REMOVE _BV(1)
#define G_EVENT_CHARGE_DONE _BV(2)
#define G_EVENT_TOUCH _BV(3)

enum ApplicationEvents_T
{
    Q_EVENT_BMA_INT,
    Q_EVENT_AXP_INT,
    Q_EVENT_UI_MESSAGE
};


#define WATCH_FLAG_SLEEP_MODE _BV(1)
#define WATCH_FLAG_SLEEP_EXIT _BV(2)
#define WATCH_FLAG_BMA_IRQ _BV(3)
#define WATCH_FLAG_AXP_IRQ _BV(4)
#define WATCH_FLAT_TOUCH_IRQ _BV(5)
#define WATCH_FLAG_UI_EVENT _BV(6)

static QueueHandle_t g_event_queue_handle = NULL;
static EventGroupHandle_t g_event_group = NULL;
static EventGroupHandle_t isr_group = NULL;

static void initialize_events();
static void post_event(ApplicationEvents_T event);