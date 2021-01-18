#pragma once
#include "esp_timer.h"
#include "settings_view.h"
#include "localization.h"
#include "ui_ticker.h"

/**
 * @brief Shows version, author, uptime.
 * Future: wake-ups, number of handled SK deltas, more?
 **/

class WatchInfo : public SettingsView
{
public:
    WatchInfo() : SettingsView(LOC_WATCH_INFO) { }

protected:
    virtual void show_internal(lv_obj_t *parent) override
    {
        lv_cont_set_layout(parent, LV_LAYOUT_COLUMN_LEFT);

        author_ = lv_label_create(parent, NULL);
        lv_label_set_text(author_, LOC_AUTHOR);

        version_ = lv_label_create(parent, NULL);
        lv_label_set_text(version_, LOC_WATCH_VERSION);

        uptime_ = lv_label_create(parent, NULL);
        update_uptime();
        
        uptimeTicker_ = new UITicker(1000, [this]() {
            this->update_uptime();
        });
    }

    virtual bool hide_internal() override
    {
        delete uptimeTicker_;
        uptimeTicker_ = NULL;
        return true;
    }

    void update_uptime()
    {
        int32_t elapsed_seconds = esp_timer_get_time() / 1000000;
        int hours = elapsed_seconds/3600;
	    elapsed_seconds = elapsed_seconds%3600;
	    int minutes = elapsed_seconds/60;
	    elapsed_seconds = elapsed_seconds%60;
	    int seconds = elapsed_seconds;

        lv_label_set_text_fmt(uptime_, LOC_UPTIME, hours, minutes, seconds);
    }

private:
    lv_obj_t* version_;
    lv_obj_t* author_;
    lv_obj_t* uptime_;
    UITicker* uptimeTicker_;

};