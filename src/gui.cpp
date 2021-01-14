#include "config.h"
#include <Arduino.h>
#include <time.h>
#include "gui.h"
#include "ui/keyboard.h"
#include "ui/message.h"
#include "ui/loader.h"
#include "ui/wifilist.h"
#include "ui/navigationview.h"
#include <WiFi.h>
#include "string.h"
#include <Ticker.h>
#include "FS.h"
#include "SD.h"
#include "hardware/Wifi.h"
#include "system/systemobject.h"
#include "system/observable.h"
#include "system/events.h"
#include "ui/settings_view.h"
#include "ui/wifisettings.h"
#include "ui/signalk_settings.h"
#include "ui/time_settings.h"
#include "ui/watch_info.h"

#define RTC_TIME_ZONE "CET-1CEST,M3.5.0,M10.5.0/3"

LV_FONT_DECLARE(Geometr);
LV_FONT_DECLARE(Ubuntu);
LV_FONT_DECLARE(roboto80);
LV_IMG_DECLARE(bg_default);
LV_IMG_DECLARE(sk_status);
LV_IMG_DECLARE(signalk_48px);
LV_IMG_DECLARE(menu);
LV_IMG_DECLARE(wifi_48px);
LV_IMG_DECLARE(info_48px);
LV_IMG_DECLARE(time_48px);

LV_IMG_DECLARE(setting);
LV_IMG_DECLARE(on);
LV_IMG_DECLARE(off);
LV_IMG_DECLARE(iexit);

static lv_style_t settingStyle;
const char *GUI_TAG = "GUI";

static void main_menu_event_cb(lv_obj_t *obj, lv_event_t event)
{
    Gui *gui = (Gui *)obj->user_data;
    if (event == LV_EVENT_SHORT_CLICKED)
    { //!  Event callback Is in here
        gui->toggle_main_bar(true);
        NavigationView *setupMenu = NULL;
        setupMenu = new NavigationView(LOC_SETTINGS_MENU, [setupMenu, gui]() {
            delete setupMenu;
            gui->toggle_main_bar(false);
        });

        setupMenu->add_tile("Clock", &time_48px, [setupMenu, gui]() {
            auto timeSetting = new TimeSettings(TTGOClass::getWatch(), gui->get_sk_socket());
            timeSetting->set_24hour_format(gui->get_time_24hour_format());
            timeSetting->on_close([timeSetting, gui]() {
                if (gui->get_time_24hour_format() != timeSetting->get_24hour_format())
                {
                    gui->set_time_24hour_format(timeSetting->get_24hour_format());
                    gui->save();
                }

                delete timeSetting;
            });
            
            setupMenu->add_tile("Wifi", &wifi_48px, [setupMenu]() {
                auto wifiSettings = new WifiSettings(wifiManager);
                wifiSettings->on_close([wifiSettings]() {
                    delete wifiSettings;
                });
                wifiSettings->show(lv_scr_act());
            });
            
            setupMenu->add_tile("Signal K", &signalk_48px, [setupMenu]() {
                auto skSettings = new SignalKSettings(ws_socket);
                skSettings->on_close([skSettings]() {
                    delete skSettings;
                });
                skSettings->show(lv_scr_act());
            });
            
            setupMenu->add_tile("Watch info", &info_48px, [setupMenu]() {
                ESP_LOGI("GUI", "Show watch info!");
                auto watchInfo = new WatchInfo();
                watchInfo->on_close([watchInfo]() {
                    delete watchInfo;
                });
                watchInfo->show(lv_scr_act());
            });
            
            setupMenu->show(lv_scr_act());
        }
    }
}

void Gui::setup_gui(WifiManager *wifi, SignalKSocket *socket)
{
    wifiManager = wifi;
    ws_socket = socket;
    lv_style_init(&settingStyle);
    lv_style_set_radius(&settingStyle, LV_OBJ_PART_MAIN, 0);
    lv_style_set_bg_color(&settingStyle, LV_OBJ_PART_MAIN, LV_COLOR_GRAY);
    lv_style_set_bg_opa(&settingStyle, LV_OBJ_PART_MAIN, LV_OPA_0);
    lv_style_set_border_width(&settingStyle, LV_OBJ_PART_MAIN, 0);
    lv_style_set_text_color(&settingStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);
    lv_style_set_image_recolor(&settingStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);

    //Create wallpaper
    lv_obj_t *scr = lv_scr_act();
    lv_obj_t *img_bin = lv_img_create(scr, NULL); /*Create an image object*/
    lv_img_set_src(img_bin, &bg_default);
    lv_obj_align(img_bin, NULL, LV_ALIGN_CENTER, 0, 0);

    bar = new StatusBar();
    //! bar
    bar->createIcons(scr);
    //battery
    update_battery_level();
    lv_icon_battery_t icon = LV_ICON_CALCULATION;

    TTGOClass *ttgo = TTGOClass::getWatch();

    if (ttgo->power->isChargeing())
    {
        icon = LV_ICON_CHARGE;
    }

    update_battery_icon(icon);
    //! main
    static lv_style_t mainStyle;
    lv_style_init(&mainStyle);
    lv_style_set_radius(&mainStyle, LV_OBJ_PART_MAIN, 0);
    lv_style_set_bg_color(&mainStyle, LV_OBJ_PART_MAIN, LV_COLOR_GRAY);
    lv_style_set_bg_opa(&mainStyle, LV_OBJ_PART_MAIN, LV_OPA_0);
    lv_style_set_border_width(&mainStyle, LV_OBJ_PART_MAIN, 0);
    lv_style_set_text_color(&mainStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);
    lv_style_set_image_recolor(&mainStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);

    mainBar = lv_cont_create(scr, NULL);
    lv_obj_set_size(mainBar, LV_HOR_RES, LV_VER_RES - bar->height());
    lv_obj_add_style(mainBar, LV_OBJ_PART_MAIN, &mainStyle);
    lv_obj_align(mainBar, bar->self(), LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

    //! Time
    static lv_style_t timeStyle;
    lv_style_copy(&timeStyle, &mainStyle);
    lv_style_set_text_font(&timeStyle, LV_STATE_DEFAULT, &roboto80);

    timeLabel = lv_label_create(mainBar, NULL);
    lv_obj_add_style(timeLabel, LV_OBJ_PART_MAIN, &timeStyle);
    update_time();

    //! menu
    static lv_style_t style_pr;

    lv_style_init(&style_pr);
    lv_style_set_image_recolor(&style_pr, LV_OBJ_PART_MAIN, LV_COLOR_BLACK);
    lv_style_set_text_color(&style_pr, LV_OBJ_PART_MAIN, lv_color_hex3(0xaaa));

    menuBtn = lv_imgbtn_create(mainBar, NULL);

    lv_imgbtn_set_src(menuBtn, LV_BTN_STATE_RELEASED, &menu);
    lv_imgbtn_set_src(menuBtn, LV_BTN_STATE_PRESSED, &menu);
    lv_imgbtn_set_src(menuBtn, LV_BTN_STATE_CHECKED_RELEASED, &menu);
    lv_imgbtn_set_src(menuBtn, LV_BTN_STATE_CHECKED_PRESSED, &menu);
    lv_obj_add_style(menuBtn, LV_OBJ_PART_MAIN, &style_pr);

    lv_obj_align(menuBtn, mainBar, LV_ALIGN_OUT_BOTTOM_MID, 0, -70);
    menuBtn->user_data = this;
    lv_obj_set_event_cb(menuBtn, main_menu_event_cb);

    auto update_task = lv_task_create(lv_update_task, 1000, LV_TASK_PRIO_LOWEST, NULL);
    auto batery_update_task = lv_task_create(lv_battery_task, 30000, LV_TASK_PRIO_LOWEST, NULL);
    update_task->user_data = this;
    batery_update_task->user_data = this;
}

void Gui::update_step_counter(uint32_t counter)
{
    bar->setStepCounter(counter);
}

void Gui::update_time()
{
    time_t now;
    struct tm info;
    char buf[64];
    time(&now);
    localtime_r(&now, &info);
    if (is24hourFormat)
    {
        strftime(buf, sizeof(buf), "%R", &info);
    }
    else
    {
        strftime(buf, sizeof(buf), "%r", &info);
    }

    lv_label_set_text(this->timeLabel, buf);
    lv_obj_align(timeLabel, NULL, LV_ALIGN_IN_TOP_MID, 0, 20);
}

void Gui::update_battery_level()
{
    TTGOClass *ttgo = TTGOClass::getWatch();
    int p = ttgo->power->getBattPercentage();
    bar->updateLevel(p);
}

void Gui::update_battery_icon(lv_icon_battery_t icon)
{
    if (icon >= LV_ICON_CALCULATION)
    {
        TTGOClass *ttgo = TTGOClass::getWatch();
        int level = ttgo->power->getBattPercentage();
        if (level > 95)
            icon = LV_ICON_BAT_FULL;
        else if (level > 80)
            icon = LV_ICON_BAT_3;
        else if (level > 45)
            icon = LV_ICON_BAT_2;
        else if (level > 20)
            icon = LV_ICON_BAT_1;
        else
            icon = LV_ICON_BAT_EMPTY;
    }
    bar->updateBatteryIcon(icon);
}

char *Gui::message_from_code(GuiEventCode_t code)
{
    switch (code)
    {
    case GuiEventCode_t::GUI_WARN_SK_REJECTED:
        return LOC_SIGNALK_REQUEST_REJECTED;
    default:
        return NULL;
    };
}

void Gui::toggle_status_bar_icon(lv_icon_status_bar_t icon, bool hidden)
{
    
}

void Gui::lv_update_task(struct _lv_task_t *data)
{
    Gui *gui = (Gui *)data->user_data;
    gui->update_time();

    if (gui->wifiManager->get_status() == WifiState_t::Wifi_Off)
    {
        gui->bar->hidden(lv_icon_status_bar_t::LV_STATUS_BAR_WIFI);
    }
    else
    {
        gui->bar->show(lv_icon_status_bar_t::LV_STATUS_BAR_WIFI);
    }

    if (gui->ws_socket->get_state() == WebsocketState_t::WS_Connected)
    {
        gui->bar->show(lv_icon_status_bar_t::LV_STATUS_BAR_SIGNALK);
    }
    else
    {
        gui->bar->hidden(lv_icon_status_bar_t::LV_STATUS_BAR_SIGNALK);
    }

    GuiEvent_t event;

    if (read_gui_update(event))
    {
        if (event.event == GuiEventType_t::GUI_SHOW_MESSAGE || event.event == GuiEventType_t::GUI_SHOW_WARNING)
        {
            char *message = (char *)event.argument;
            if (message == NULL)
            {
                message = gui->message_from_code(event.eventCode);
            }

            if (message != NULL)
            {
                static const char *btns[] = {LOC_MESSAGEBOX_OK, ""};
                lv_obj_t *mbox1 = lv_msgbox_create(lv_scr_act(), NULL);
                lv_msgbox_set_text(mbox1, message);
                lv_msgbox_add_btns(mbox1, btns);
                lv_obj_set_width(mbox1, 200);
                lv_obj_align(mbox1, NULL, LV_ALIGN_CENTER, 0, 0);
            }

            ESP_LOGI(GUI_TAG, "Show message %s, event=%d, code=%d!", (char *)event.argument, event.event, event.eventCode);
        }
        else if (event.event == GuiEventType_t::GUI_SIGNALK_UPDATE)
        {
            ESP_LOGI(GUI_TAG, "Update SK view %s", (char *)event.argument);
        }

        if (event.argument != NULL)
        {
            free(event.argument);
        }
    }
}

void Gui::lv_battery_task(struct _lv_task_t *data)
{
    Gui *gui = (Gui *)data->user_data;
    gui->update_battery_level();
}

void Gui::toggle_status_bar(bool hidden)
{
    bar->set_hidden(hidden);
}

void Gui::toggle_main_bar(bool hidden)
{
    lv_obj_set_hidden(mainBar, hidden);
}

void Gui::load_config_from_file(const JsonObject &json)
{
    is24hourFormat = json["24hourformat"].as<bool>();
    screenTimeout = json["screentimeout"].as<int>();
    timeZone = json["timezone"].as<String>();
}

void Gui::save_config_to_file(JsonObject &json)
{
    json["24hourformat"] = is24hourFormat;
    json["screentimeout"] = screenTimeout;
    json["timezone"] = timeZone;
}