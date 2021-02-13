#pragma once
#include "config.h"
#include "ui/settings_view.h"
#include "localization.h"

LV_FONT_DECLARE(lv_font_montserrat_28);

class WifiList : public SettingsView
{
public:
    WifiList() : SettingsView(LOC_WIFI_SELECT) {}

    ~WifiList()
    {
        if (wifi_list_ != NULL)
        {
            lv_obj_del(wifi_list_);
            wifi_list_ = NULL;
        }
    }

    virtual void show_internal(lv_obj_t *parent) override
    {
        lv_cont_set_layout(parent, LV_LAYOUT_OFF);

        wifi_list_ = lv_list_create(parent, NULL);
        lv_obj_set_pos(wifi_list_, 0, 0);
        lv_obj_set_size(wifi_list_, lv_obj_get_width(parent), lv_obj_get_height(parent));
        lv_obj_set_style_local_scale_border_width(wifi_list_, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
        lv_obj_set_style_local_radius(wifi_list_, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
        lv_obj_set_style_local_text_font(wifi_list_, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_28);
    }

    virtual bool hide_internal() override
    {
        lv_obj_del(wifi_list_);
        wifi_list_ = NULL;
        return true;
    }

    void add_ssid(const char *txt, void *imgsrc = (void *)LV_SYMBOL_WIFI)
    {
        lv_obj_t *btn = lv_list_add_btn(wifi_list_, imgsrc, txt);
        btn->user_data = this;
        lv_obj_set_event_cb(btn, __list_event_cb);
    }

    static void __list_event_cb(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_SHORT_CLICKED)
        {
            auto list = (WifiList *)obj->user_data;
            const char *ssid = lv_list_get_btn_text(obj);
            list->wifi_selected(ssid);
        }
    }

    const char *selected_ssid()
    {
        return selected_ssid_;
    }

private:
    lv_obj_t *wifi_list_ = NULL;
    const char *selected_ssid_ = NULL;

    void wifi_selected(const char *ssid)
    {
        selected_ssid_ = ssid;
        hide();
    }
};