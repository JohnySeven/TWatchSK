#pragma once
#include "config.h"
#include "ui/settings_view.h"
#include "localization.h"

/*****************************************************************
 *
 *          ! List Class
 *
 */

class WifiList : public SettingsView
{
public:
    WifiList() : SettingsView(LOC_WIFI_SELECT) {}

    ~WifiList()
    {
        if (_listCont == nullptr)
            return;
        lv_obj_del(_listCont);
        _listCont = nullptr;
    }

    virtual void show_internal(lv_obj_t *parent) override
    {
        lv_cont_set_layout(parent, LV_LAYOUT_OFF);
        static lv_style_t listStyle;
        lv_style_init(&listStyle);
        lv_style_set_radius(&listStyle, LV_OBJ_PART_MAIN, 0);
        lv_style_set_bg_color(&listStyle, LV_OBJ_PART_MAIN, LV_COLOR_GRAY);
        lv_style_set_bg_opa(&listStyle, LV_OBJ_PART_MAIN, LV_OPA_0);
        lv_style_set_border_width(&listStyle, LV_OBJ_PART_MAIN, 0);
        lv_style_set_text_color(&listStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);
        lv_style_set_image_recolor(&listStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);

        _listCont = lv_list_create(parent, NULL);
        lv_obj_set_pos(_listCont, 0, 0);
        lv_obj_set_size(_listCont, lv_obj_get_width(parent), lv_obj_get_height(parent));
        lv_obj_add_style(_listCont, LV_OBJ_PART_MAIN, &listStyle);
    }

    virtual bool hide_internal() override
    {
        lv_obj_del(_listCont);
        _listCont = nullptr;
        return true;
    }

    void add_ssid(const char *txt, void *imgsrc = (void *)LV_SYMBOL_WIFI)
    {
        lv_obj_t *btn = lv_list_add_btn(_listCont, imgsrc, txt);
        btn->user_data = this;
        lv_obj_set_event_cb(btn, __list_event_cb);
    }

    static void __list_event_cb(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_SHORT_CLICKED)
        {
            auto list = (WifiList*)obj->user_data;
            const char *ssid = lv_list_get_btn_text(obj);
            list->wifi_selected(ssid);
        }
    }

    const char * selected_ssid()
    {
        return _selected_ssid;
    }
private:
    lv_obj_t *_listCont = nullptr;
    const char* _selected_ssid = nullptr;

    void wifi_selected(const char*ssid)
    {
        _selected_ssid = ssid;
        hide();
    }
};