#include "Hardware/Touch.h"
//Took from code by sharandac, uri: https://github.com/sharandac/My-TTGO-Watch/blob/master/src/hardware/touch.cpp
#define TOUCH_TAG "TOUCH"
static SemaphoreHandle_t xTouchSemaphore = NULL;
lv_indev_t *touch_dev = NULL;

static bool DRAM_ATTR low_power_mode = false;
static bool touch_down = false;
//Touch IRQ flags and Mutex
volatile bool DRAM_ATTR touch_irq_flag = false;
portMUX_TYPE DRAM_ATTR Touch_IRQ_Mux = portMUX_INITIALIZER_UNLOCKED;
EventGroupHandle_t watch_isr_group = NULL;

#define WATCH_FLAG_SLEEP_MODE _BV(1) // in sleep mode
#define WATCH_FLAG_SLEEP_EXIT _BV(2) // leaving sleep mode because of any kind of interrupt
#define WATCH_FLAG_TOUCH_IRQ _BV(5)  // leaving sleep mode because of touch

bool touch_lock_take( void ) {
    return xSemaphoreTake( xTouchSemaphore, portMAX_DELAY ) == pdTRUE;
}
void touch_lock_give( void ) {
    xSemaphoreGive( xTouchSemaphore );
}

static bool touch_getXY(int16_t &x, int16_t &y)
{

    TTGOClass *ttgo = TTGOClass::getWatch();
    static bool touch_press = false;

    if (!low_power_mode)
    {
        /*
     * get touchstate from touchcontroller if not taken
     * by other task/thread
     */
        bool getTouchResult = false;
        if (touch_lock_take())
        {
            getTouchResult = ttgo->getTouch(x, y);
            touch_lock_give();
        }
        /*
     * if touched?
     */
        if (!getTouchResult)
        {
            touch_press = false;
            return (false);
        }

        /*
     * issue https://github.com/sharandac/My-TTGO-Watch/issues/18 fix
     */
        float temp_x = (x - (lv_disp_get_hor_res(NULL) / 2)) * 1.15;
        float temp_y = (y - (lv_disp_get_ver_res(NULL) / 2)) * 1.0;
        x = temp_x + (lv_disp_get_hor_res(NULL) / 2);
        y = temp_y + (lv_disp_get_ver_res(NULL) / 2);

        return true;
    }
    else
    {
        return false;
    }
}

static bool touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    /*
     * We use two flags, one changes in the interrupt handler
     * the other controls whether we poll the sensor,
     * and gets cleared when the level is no longer low,
     * meaning the touch has finished
     */
    portENTER_CRITICAL(&Touch_IRQ_Mux);
    bool temp_touch_irq_flag = touch_irq_flag;
    touch_irq_flag = false;
    portEXIT_CRITICAL(&Touch_IRQ_Mux);
    touch_down |= temp_touch_irq_flag;
    /*
     * check for an touch interrupt
     */
    if (touch_down)
    {
        data->state = touch_getXY(data->point.x, data->point.y) ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
        touch_down = digitalRead(TOUCH_INT) == LOW;
        if (!touch_down)
        {
            /*
             * Save power by switching to monitor mode now instead of waiting for 30 seconds.
             */
            if (touch_lock_take())
            {
                TTGOClass::getWatch()->touchToMonitor();
                touch_lock_give();
            }
        }
        else
        {
            ESP_LOGI(TOUCH_TAG, "Touch=%d,%d", data->point.x, data->point.y);
        }
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
    return false;
}

void IRAM_ATTR touch_irq(void)
{
    /*
     * enter critical section and set interrupt flag
     */
    portENTER_CRITICAL_ISR(&Touch_IRQ_Mux);
    touch_irq_flag = true;
    /*
     * leave critical section
     */
    portEXIT_CRITICAL_ISR(&Touch_IRQ_Mux);

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    EventBits_t bits = xEventGroupGetBitsFromISR(watch_isr_group);
    if (bits & WATCH_FLAG_SLEEP_MODE)
    {
        //! For quick wake up, use the group flag
        xEventGroupSetBitsFromISR(watch_isr_group, WATCH_FLAG_SLEEP_EXIT | WATCH_FLAG_TOUCH_IRQ, &xHigherPriorityTaskWoken);
    }
}

void Touch::set_low_power(bool low_power)
{
    low_power_mode = low_power;
}

bool Touch::initialize(EventGroupHandle_t wakeupEvents)
{
    TTGOClass *ttgo = TTGOClass::getWatch();

#if defined(LILYGO_WATCH_2020_V2) || defined(LILYGO_WATCH_2020_V3)
    ttgo->touchWakup();
#endif
    ESP_LOGI(TOUCH_TAG, "irq disable");

    //we will handle TOUCH interupts on our own
    ttgo->touch->disableINT();
    ttgo->disableTouchIRQ();
    ESP_LOGI(TOUCH_TAG, "touch monitoring config");
    //configure touch monitoring time
    ttgo->touch->setMonitorTime(0x01);
    ttgo->touch->setMonitorPeriod(125);
    ESP_LOGI(TOUCH_TAG, "driver init");
    xTouchSemaphore = xSemaphoreCreateMutex();
    touch_dev = lv_indev_get_next(NULL);
    touch_dev->driver.read_cb = touch_read;
    ESP_LOGI(TOUCH_TAG, "interup attach");
    attachInterrupt(TOUCH_INT, &touch_irq, FALLING);
    ESP_LOGI(TOUCH_TAG, "ok");

    watch_isr_group = wakeupEvents;

    return true;
}