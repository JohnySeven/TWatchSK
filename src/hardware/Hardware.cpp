#include "Hardware.h"
#include "system/events.h"

EventGroupHandle_t isr_group = NULL;

#define WATCH_FLAG_SLEEP_MODE _BV(1) // in sleep mode
#define WATCH_FLAG_SLEEP_EXIT _BV(2) // leaving sleep mode because of any kind of interrupt
#define WATCH_FLAG_BMA_IRQ _BV(3)    // leaving sleep mode because of double tap or tilt
#define WATCH_FLAG_AXP_IRQ _BV(4)    // leaving sleep mode because of external button press or any other power management interrupt
#define WATCH_FLAG_TOUCH_IRQ _BV(5)  // leaving sleep mode because of touch (not yet implemented)

#define MOTOR_CHANNEL 1
#define MOTOR_FREQUENCY 12000

const char *HW_TAG = "HW";

Hardware::Hardware() : Configurable("/config/hardware")
{
    load();
    //setup vibrating motor configuration
    ledcSetup(MOTOR_CHANNEL, MOTOR_FREQUENCY, 8);
}

void Hardware::load_config_from_file(const JsonObject &json)
{
    double_tap_wakeup_ = json["doubletapwakeup"].as<bool>();
    tilt_wakeup_ = json["tiltwakeup"].as<bool>();
    touch_wakeup_ = json["touchwakeup"].as<bool>();

    ESP_LOGI(HW_TAG, "Loaded hardware settings: double tap wakeup=%d, tilt wakeup=%d, touch wakeup=%d", double_tap_wakeup_, tilt_wakeup_, touch_wakeup_);
}

void Hardware::save_config_to_file(JsonObject &json)
{
    json["doubletapwakeup"] = double_tap_wakeup_;
    json["tiltwakeup"] = tilt_wakeup_;
    json["touchwakeup"] = touch_wakeup_;
}

/**
 * Initializes hardware required for power managements operation and configures wake up source
 * 
 */
void Hardware::initialize(TTGOClass *watch)
{
    isr_group = xEventGroupCreate();
    watch_ = watch;

    // Turn on the IRQ used
    watch->power->adc1Enable(AXP202_BATT_VOL_ADC1 | AXP202_BATT_CUR_ADC1 | AXP202_VBUS_VOL_ADC1 | AXP202_VBUS_CUR_ADC1, AXP202_ON);
    watch->power->enableIRQ(AXP202_VBUS_REMOVED_IRQ | AXP202_VBUS_CONNECT_IRQ | AXP202_CHARGING_FINISHED_IRQ, AXP202_ON);
    watch->power->clearIRQ();
    watch->power->setChgLEDMode(axp_chgled_mode_t::AXP20X_LED_OFF);

    // Turn off unused power
    watch->power->setPowerOutPut(AXP202_EXTEN, AXP202_OFF);
    watch->power->setPowerOutPut(AXP202_DCDC2, AXP202_OFF);
    watch->power->setPowerOutPut(AXP202_LDO3, AXP202_OFF);
    watch->power->setPowerOutPut(AXP202_LDO4, AXP202_OFF);

    ESP_LOGI(HW_TAG, "Watch power initialized!");

    // Enable BMA423 interrupt ，
    // The default interrupt configuration,
    // you need to set the acceleration parameters, please refer to the BMA423_Accel example
    watch->bma->attachInterrupt();
    watch_->bma->enableTiltInterrupt(this->tilt_wakeup_); // set according to the saved setting
    watch_->bma->enableWakeupInterrupt(true);             // needs to be on for double-tap theme switching whenever watch is awake
    //TODO: touch interrupt breaks touch as it uses the interrupt also
    /*pinMode(TOUCH_INT, INPUT);
    attachInterrupt(
        TOUCH_INT, [] {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            EventBits_t bits = xEventGroupGetBitsFromISR(isr_group);
            if (bits & WATCH_FLAG_SLEEP_MODE)
            {
                //! For quick wake up, use the group flag
                xEventGroupSetBitsFromISR(isr_group, WATCH_FLAG_SLEEP_EXIT | WATCH_FLAG_TOUCH_IRQ, &xHigherPriorityTaskWoken);
            }

            if (xHigherPriorityTaskWoken)
            {
                portYIELD_FROM_ISR();
            }
        },
        RISING);*/
    //Accelerometer interupt
    pinMode(BMA423_INT1, INPUT);
    attachInterrupt(
        BMA423_INT1, []
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            EventBits_t bits = xEventGroupGetBitsFromISR(isr_group);
            if (bits & WATCH_FLAG_SLEEP_MODE)
            {
                //! For quick wake up, use the group flag
                xEventGroupSetBitsFromISR(isr_group, WATCH_FLAG_SLEEP_EXIT | WATCH_FLAG_BMA_IRQ, &xHigherPriorityTaskWoken);
            }
            else
            {
                uint8_t data = ApplicationEvents_T::Q_EVENT_BMA_INT;
                xQueueSendFromISR(g_event_queue_handle, &data, &xHigherPriorityTaskWoken);
            }

            if (xHigherPriorityTaskWoken)
            {
                portYIELD_FROM_ISR();
            }
        },
        RISING);
    //Power management interupt, connection interrupted to the specified pin
    pinMode(AXP202_INT, INPUT);
    attachInterrupt(
        AXP202_INT, []
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            EventBits_t bits = xEventGroupGetBitsFromISR(isr_group);
            if (bits & WATCH_FLAG_SLEEP_MODE)
            {
                //! For quick wake up, use the group flag
                xEventGroupSetBitsFromISR(isr_group, WATCH_FLAG_SLEEP_EXIT | WATCH_FLAG_AXP_IRQ, &xHigherPriorityTaskWoken);
            }
            else
            {
                uint8_t data = ApplicationEvents_T::Q_EVENT_AXP_INT;
                xQueueSendFromISR(g_event_queue_handle, &data, &xHigherPriorityTaskWoken);
            }
            if (xHigherPriorityTaskWoken)
            {
                portYIELD_FROM_ISR();
            }
        },
        FALLING);

    //initialize sound player
    player_ = new SoundPlayer();
    /*touch_ = new Touch();

    touch_->initialize();*/
}

///Invokes power callback to all listeners
void Hardware::invoke_power_callbacks(PowerCode_t code, uint32_t arg)
{
    ESP_LOGI(HW_TAG, "invoke_power_callbacks(PowerCode_t %d, arg %d)", (int)code, arg);
    for (auto callback : power_callbacks_)
    {
        callback(code, arg);
    }
}

/**
 * This function drives low_energy states.
 * If watch is in normal power mode, lenergy_ == false, it will enter low power mode and invoke_power_callback with POWER_ENTER_LOW_POWER.
 * If watch is in low power mode it waits until event is received via isr_group, if event is received it will wakeup watch
 * and invoke_power_callback with POWER_LEAVE_LOW_POWER.
 */
void Hardware::low_energy()
{
    EventBits_t isr_bits = xEventGroupGetBits(isr_group);
    EventBits_t app_bits = xEventGroupGetBits(g_app_state);

    if (!lenergy_) // watch is not in low power mode, so set it to low power mode now
    {
        watch_->closeBL();
        watch_->stopLvglTick();
        //disable step count interrupt to save battery (counter will still work)
        watch_->bma->enableStepCountInterrupt(false);
        watch_->bma->enableWakeupInterrupt(double_tap_wakeup_); // enable or disable double_tap_wakeup depending on the switch setting
        watch_->displaySleep();
        touch_->set_low_power(true);
        //set event bits in events.cpp
        xEventGroupSetBits(isr_group, WATCH_FLAG_SLEEP_MODE);
        set_low_power(true);
        //notify every one we are going low power!
        invoke_power_callbacks(PowerCode_t::POWER_ENTER_LOW_POWER, 0);
        lenergy_ = true;

        ESP_LOGI(HW_TAG, "Entering light sleep.");
        uint counter = 0;
        while (!(isr_bits & WATCH_FLAG_SLEEP_EXIT) && !(app_bits & G_APP_STATE_WAKE_UP))
        {
            delay(500);
            counter++;
            isr_bits = xEventGroupGetBits(isr_group);
            app_bits = xEventGroupGetBits(g_app_state);
            //Signal every 5000 ms low power actions
            if (counter % 10 == 0)
            {
                invoke_power_callbacks(PowerCode_t::POWER_LOW_TICK, counter / 2);
            }
        }

        xEventGroupClearBits(g_app_state, G_APP_STATE_WAKE_UP);
        if (app_bits & G_APP_STATE_WAKE_UP)
        {
            xEventGroupSetBits(isr_group, WATCH_FLAG_SLEEP_EXIT);
        }

        ESP_LOGI(HW_TAG, "Wakeup request from sleep. System isr=%d,app=%d", isr_bits, app_bits);
        //}
    }
    else // watch is in low power mode, so wake it up now
    {
        //set events in events.cpp
        set_low_power(false);
        lenergy_ = false;
        touch_->set_low_power(false);
        //start LVGL ticking
        watch_->startLvglTick();
        //wakeup display
        watch_->displayWakeup();
        watch_->touch->setPowerMode(PowerMode_t::FOCALTECH_PMODE_ACTIVE);
        //update system time from RTC
        watch_->rtc->syncToSystem();
        // always enable double_tap_wakeup when awake - necessary for double-tap theme change to work
        watch_->bma->enableWakeupInterrupt(true);
        //enable display backlight
        watch_->openBL();
        WakeupSource_t source = (isr_bits & WATCH_FLAG_BMA_IRQ ? WAKEUP_ACCELEROMETER : WAKEUP_BUTTON);
        //notify everyone we are leaving low power mode
        invoke_power_callbacks(PowerCode_t::POWER_LEAVE_LOW_POWER, source);
        watch_->bma->enableStepCountInterrupt();
    }
}

void Hardware::loop()
{
    bool result;
    uint8_t data;
    //! Fast response wake-up interrupt
    EventBits_t bits = xEventGroupGetBits(isr_group);
    if (bits & WATCH_FLAG_SLEEP_EXIT)
    {
        low_energy();

        if (bits & WATCH_FLAG_BMA_IRQ)
        {
            do
            {
                result = watch_->bma->readInterrupt();
            } while (!result);
            xEventGroupClearBits(isr_group, WATCH_FLAG_BMA_IRQ);
        }
        if (bits & WATCH_FLAG_AXP_IRQ)
        {
            watch_->power->readIRQ();
            watch_->power->clearIRQ();

            xEventGroupClearBits(isr_group, WATCH_FLAG_AXP_IRQ);
        }
        if (bits & WATCH_FLAG_TOUCH_IRQ)
        {
            xEventGroupClearBits(isr_group, WATCH_FLAG_TOUCH_IRQ);
            ESP_LOGD(HW_TAG, "Touch interrupt!");
        }

        xEventGroupClearBits(isr_group, WATCH_FLAG_SLEEP_EXIT);
        xEventGroupClearBits(isr_group, WATCH_FLAG_SLEEP_MODE);
    }
    if ((bits & WATCH_FLAG_SLEEP_MODE))
    {
        //! No event processing after entering the information screen
        return;
    }

    //! Normal polling
    if (xQueueReceive(g_event_queue_handle, &data, 5 / portTICK_RATE_MS) == pdPASS)
    {
        switch (data)
        {
        case ApplicationEvents_T::Q_EVENT_BMA_INT:
            do
            {
                result = watch_->bma->readInterrupt();
            } while (!result);

            // step counter
            if (watch_->bma->isStepCounter())
            {
                invoke_power_callbacks(WALK_STEP_COUNTER_UPDATED, watch_->bma->getCounter());
            }

            // double tap
            if (!lenergy_ && watch_->bma->isDoubleClick())
            {
                invoke_power_callbacks(DOUBLE_TAP_DETECTED, 0);
            }

            break;

        case ApplicationEvents_T::Q_EVENT_AXP_INT:
            watch_->power->readIRQ();

            if (watch_->power->isVbusPlugInIRQ())
            {
                invoke_power_callbacks(PowerCode_t::POWER_CHARGING_ON, 0);
            }

            if (watch_->power->isVbusRemoveIRQ())
            {
                invoke_power_callbacks(PowerCode_t::POWER_CHARGING_OFF, 0);
            }

            if (watch_->power->isChargingDoneIRQ())
            {
                invoke_power_callbacks(PowerCode_t::POWER_CHARGING_DONE, 0);
            }

            if (watch_->power->isPEKShortPressIRQ())
            {
                watch_->power->clearIRQ();
                low_energy();
                return;
            }

            watch_->power->clearIRQ();
            break;
        default:
            break;
        }
    }

    if (!lenergy_)
    {
        if (lv_disp_get_inactive_time(NULL) < (get_screen_timeout_() * 1000))
        {
            auto sleep = lv_task_handler();
            if (sleep > 250)
            {
                sleep = 250;
            }

            delay(sleep);
        }
        else
        {
            low_energy();
        }
    }
    else
    {
        ESP_LOGI(HW_TAG, "Low power");
        delay(250);
    }
}

void Hardware::update_bma_wakeup()
{
    watch_->bma->enableTiltInterrupt(this->tilt_wakeup_);
    watch_->bma->enableWakeupInterrupt(this->double_tap_wakeup_);
}

void Hardware::vibrate(bool status)
{
    if (status != is_vibrating_)
    {
        is_vibrating_ = status;

        if (status)
        {
            //attach pin to PWM channel 1
            ledcAttachPin(MOTOR_PIN, 1);
            //change channel 1 duty to 128 (50%)
            ledcWrite(MOTOR_CHANNEL, 128);
        }
        else
        {
            //change channel 1 duty to 0
            ledcWrite(MOTOR_CHANNEL, 0);
            //detach the PIN from PWM
            ledcDetachPin(MOTOR_PIN);
        }
    }
}

void Hardware::vibrate(int duration)
{
    if (duration < 150)
    {
        duration = 150;
    }

    twatchsk::run_async("vibrate", [this, duration]()
                        {
                            int count = duration / 100;

                            for (int i = 0; i < count; i++)
                            {
                                this->vibrate(true);
                                delay(100);
                                this->vibrate(false);
                                delay(100);
                            }

                            this->vibrate(false);
                        });
}

void Hardware::vibrate(int pattern[], int repeat)
{
    if (repeat > 0)
    {
        twatchsk::run_async("vibrate_pattern", [this, pattern, repeat]()
                            {
                                for (int i = 0; i < repeat; i++)
                                {
                                    int index = 0;
                                    int value = pattern[i];

                                    while (value != 0)
                                    {
                                        if (index % 2 == 0)
                                        {
                                            this->vibrate(true);
                                        }
                                        else
                                        {
                                            this->vibrate(false);
                                        }
                                        delay(value);
                                        index++;
                                        value = pattern[index];
                                    }
                                }

                                this->vibrate(false);
                            });
    }
}

void Hardware::intialize_touch()
{
    touch_ = new Touch();
    touch_->initialize(isr_group);
    touch_->allow_touch_wakeup(touch_wakeup_);
}