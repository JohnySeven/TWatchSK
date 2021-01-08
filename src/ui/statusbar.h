#pragma once
#include "config.h"
#include "system/observable.h"
#include "system/systemobject.h"
const char *SB_TAG = "StatusBar";

LV_IMG_DECLARE(step);
LV_IMG_DECLARE(sk_statusbar_icon);
class StatusBar : public Observer<WifiState_t>, public Observer<WebsocketState_t>
{
    typedef struct
    {
        bool vaild;
        lv_obj_t *icon;
    } lv_status_bar_t;

public:
    StatusBar()
    {
        memset(_icons, 0, sizeof(_icons));
    }

    void createIcons(lv_obj_t *par)
    {
        _par = par;

        static lv_style_t barStyle;

        lv_style_init(&barStyle);
        lv_style_set_radius(&barStyle, LV_OBJ_PART_MAIN, 0);
        lv_style_set_bg_color(&barStyle, LV_OBJ_PART_MAIN, LV_COLOR_GRAY);
        lv_style_set_bg_opa(&barStyle, LV_OBJ_PART_MAIN, LV_OPA_20);
        lv_style_set_border_width(&barStyle, LV_OBJ_PART_MAIN, 0);
        lv_style_set_text_color(&barStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);
        lv_style_set_image_recolor(&barStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);

        _bar = lv_cont_create(_par, NULL);
        lv_obj_set_size(_bar, LV_HOR_RES, _barHeight);
        lv_obj_add_style(_bar, LV_OBJ_PART_MAIN, &barStyle);

        _icons[0].icon = lv_label_create(_bar, NULL);
        lv_label_set_text(_icons[0].icon, "100%");

        _icons[1].icon = lv_img_create(_bar, NULL);
        lv_img_set_src(_icons[1].icon, LV_SYMBOL_BATTERY_FULL);

        _icons[2].icon = lv_img_create(_bar, NULL);
        lv_img_set_src(_icons[2].icon, LV_SYMBOL_WIFI);
        lv_obj_set_hidden(_icons[2].icon, true);
        //web socket
        _icons[3].icon = lv_img_create(_bar, NULL);
        lv_img_set_src(_icons[3].icon, &sk_statusbar_icon);
        //step counter
        _icons[4].icon = lv_img_create(_bar, NULL);
        lv_img_set_src(_icons[4].icon, &step);
        lv_obj_align(_icons[4].icon, _bar, LV_ALIGN_IN_LEFT_MID, 10, 0);

        _icons[5].icon = lv_label_create(_bar, NULL);
        lv_label_set_text(_icons[5].icon, "0");
        lv_obj_align(_icons[5].icon, _icons[4].icon, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

        refresh();
    }

    void setStepCounter(uint32_t counter)
    {
        lv_label_set_text(_icons[5].icon, String(counter).c_str());
        lv_obj_align(_icons[5].icon, _icons[4].icon, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    }

    void updateLevel(int level)
    {
        lv_label_set_text(_icons[0].icon, (String(level) + "%").c_str());
        refresh();
    }

    void updateBatteryIcon(lv_icon_battery_t icon)
    {
        const char *icons[6] = {LV_SYMBOL_BATTERY_EMPTY, LV_SYMBOL_BATTERY_1, LV_SYMBOL_BATTERY_2, LV_SYMBOL_BATTERY_3, LV_SYMBOL_BATTERY_FULL, LV_SYMBOL_CHARGE};
        lv_img_set_src(_icons[1].icon, icons[icon]);
        refresh();
    }

    void show(lv_icon_status_bar_t icon)
    {
        if (lv_obj_get_hidden(_icons[icon].icon) == true)
        {
            lv_obj_set_hidden(_icons[icon].icon, false);
            refresh();
        }
    }

    void hidden(lv_icon_status_bar_t icon)
    {
        if (lv_obj_get_hidden(_icons[icon].icon) == false)
        {
            lv_obj_set_hidden(_icons[icon].icon, true);
            refresh();
        }
    }

    uint8_t height()
    {
        return _barHeight;
    }
    lv_obj_t *self()
    {
        return _bar;
    }

    void set_hidden(bool hidden)
    {
        lv_obj_set_hidden(_bar, hidden);
    }

private:
    void refresh()
    {
        int prev = 0;
        for (int i = 0; i < 5; i++)
        {
            if (!lv_obj_get_hidden(_icons[i].icon))
            {
                if (i == LV_STATUS_BAR_BATTERY_LEVEL)
                {
                    lv_obj_align(_icons[i].icon, NULL, LV_ALIGN_IN_RIGHT_MID, 0, 0);
                }
                else
                {
                    lv_obj_align(_icons[i].icon, _icons[prev].icon, LV_ALIGN_OUT_LEFT_MID, iconOffset, 0);
                }
                prev = i;
            }
        }
    };
    lv_obj_t *_bar = nullptr;
    lv_obj_t *_par = nullptr;
    uint8_t _barHeight = 30;
    lv_status_bar_t _icons[6];
    const int8_t iconOffset = -5;
};
