/*
Copyright (c) 2019 lewis he
This is just a demonstration. Most of the functions are not implemented.
The main implementation is low-power standby.
The off-screen standby (not deep sleep) current is about 4mA.
Select standard motherboard and standard backplane for testing.
Created by Lewis he on October 10, 2019.
*/

// Please select the model you want to use in config.h
#include "config.h"
#include <Arduino.h>
#include <time.h>
#include "gui.h"
#include "ui/statusbar.h"
#include "ui/menubar.h"
#include "ui/keyboard.h"
#include "ui/message.h"
#include "ui/loader.h"
#include "ui/wifilist.h"
#include <WiFi.h>
#include "string.h"
#include <Ticker.h>
#include "FS.h"
#include "SD.h"
#include "hardware/Wifi.h"
#include "system/systemobject.h"
#include "system/observable.h"
#include "ui/settings_view.h"
#include "ui/wifisettings.h"

#define RTC_TIME_ZONE "CET-1CEST,M3.5.0,M10.5.0/3"

LV_FONT_DECLARE(Geometr);
LV_FONT_DECLARE(Ubuntu);
LV_FONT_DECLARE(roboto80);
LV_IMG_DECLARE(bg_default);
LV_IMG_DECLARE(sk_status);
//LV_IMG_DECLARE(WALLPAPER_1_IMG);
//LV_IMG_DECLARE(WALLPAPER_2_IMG);
//LV_IMG_DECLARE(WALLPAPER_3_IMG);
LV_IMG_DECLARE(menu);

LV_IMG_DECLARE(wifi);
//LV_IMG_DECLARE(light);
//LV_IMG_DECLARE(bluetooth);
//LV_IMG_DECLARE(sd);
LV_IMG_DECLARE(setting);
LV_IMG_DECLARE(on);
LV_IMG_DECLARE(off);
LV_IMG_DECLARE(level1);
LV_IMG_DECLARE(level2);
LV_IMG_DECLARE(level3);
LV_IMG_DECLARE(iexit);
LV_IMG_DECLARE(modules);
LV_IMG_DECLARE(CAMERA_PNG);

extern EventGroupHandle_t g_event_group;
extern QueueHandle_t g_event_queue_handle;

static lv_style_t settingStyle;
static lv_obj_t *mainBar = nullptr;
static lv_obj_t *timeLabel = nullptr;
static lv_obj_t *menuBtn = nullptr;

static uint8_t globalIndex = 0;

static WifiManager *wifiManager;

static void lv_update_task(struct _lv_task_t *);
static void lv_battery_task(struct _lv_task_t *);
static void updateTime();
static void view_event_handler(lv_obj_t *obj, lv_event_t event);

static void wifi_settings_event_cb();
static void setting_event_cb();
static void wifi_destory();

MenuBar menuBars;
StatusBar bar;
SettingsView* testView;

#define SETTINGS_MENU_ITEMS_COUNT 2
// Settings menu config
MenuBar::lv_menu_config_t settings_menu_cfg[SETTINGS_MENU_ITEMS_COUNT] = {
    {.name = "WiFi", .img = (void *)&wifi, .event_cb = wifi_settings_event_cb},
    //{.name = "Bluetooth",  .img = (void *) &bluetooth, /*.event_cb = bluetooth_event_cb*/},
    //{.name = "SD Card",  .img = (void *) &sd,  /*.event_cb =sd_event_cb*/},
    //{.name = "Light",  .img = (void *) &light, /*.event_cb = light_event_cb*/},
    {.name = "Setting", .img = (void *)&setting, .event_cb = setting_event_cb },
    //{.name = "Modules",  .img = (void *) &modules, /*.event_cb = modules_event_cb */},
    //{.name = "Camera",  .img = (void *) &CAMERA_PNG, /*.event_cb = camera_event_cb*/ }
};

static void event_handler(lv_obj_t *obj, lv_event_t event)
{
    if (event == LV_EVENT_SHORT_CLICKED)
    { //!  Event callback Is in here
        if (obj == menuBtn)
        {
            lv_obj_set_hidden(mainBar, true);
            if (menuBars.self() == nullptr)
            {
                menuBars.createMenu(settings_menu_cfg, SETTINGS_MENU_ITEMS_COUNT, view_event_handler);
                lv_obj_align(menuBars.self(), bar.self(), LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
            }
            else
            {
                menuBars.hidden(false);
            }
        }
    }
}

void setupGui(WifiManager *wifi, SignalKSocket*socket)
{
    wifiManager = wifi;
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

    //! bar
    bar.createIcons(scr, wifi, socket);
    //battery
    updateBatteryLevel();
    lv_icon_battery_t icon = LV_ICON_CALCULATION;

    TTGOClass *ttgo = TTGOClass::getWatch();

    if (ttgo->power->isChargeing())
    {
        icon = LV_ICON_CHARGE;
    }
    updateBatteryIcon(icon);
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
    lv_obj_set_size(mainBar, LV_HOR_RES, LV_VER_RES - bar.height());
    lv_obj_add_style(mainBar, LV_OBJ_PART_MAIN, &mainStyle);
    lv_obj_align(mainBar, bar.self(), LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

    //! Time
    static lv_style_t timeStyle;
    lv_style_copy(&timeStyle, &mainStyle);
    lv_style_set_text_font(&timeStyle, LV_STATE_DEFAULT, &roboto80);

    timeLabel = lv_label_create(mainBar, NULL);
    lv_obj_add_style(timeLabel, LV_OBJ_PART_MAIN, &timeStyle);
    updateTime();

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
    lv_obj_set_event_cb(menuBtn, event_handler);

    lv_task_create(lv_update_task, 1000, LV_TASK_PRIO_LOWEST, NULL);
    lv_task_create(lv_battery_task, 30000, LV_TASK_PRIO_LOWEST, NULL);
}

void updateStepCounter(uint32_t counter)
{
    bar.setStepCounter(counter);
}

static void updateTime()
{
    time_t now;
    struct tm info;
    char buf[64];
    time(&now);
    localtime_r(&now, &info);
    strftime(buf, sizeof(buf), "%H:%M", &info);
    lv_label_set_text(timeLabel, buf);
    lv_obj_align(timeLabel, NULL, LV_ALIGN_IN_TOP_MID, 0, 20);
    TTGOClass *ttgo = TTGOClass::getWatch();
    ttgo->rtc->syncToRtc();
}

void updateBatteryLevel()
{
    TTGOClass *ttgo = TTGOClass::getWatch();
    int p = ttgo->power->getBattPercentage();
    bar.updateLevel(p);
}

void updateBatteryIcon(lv_icon_battery_t icon)
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
    bar.updateBatteryIcon(icon);
}

static void lv_update_task(struct _lv_task_t *data)
{
    updateTime();
}

static void lv_battery_task(struct _lv_task_t *data)
{
    updateBatteryLevel();
}

static void view_event_handler(lv_obj_t *obj, lv_event_t event)
{
    int size = SETTINGS_MENU_ITEMS_COUNT;
    if (event == LV_EVENT_SHORT_CLICKED)
    {
        if (obj == menuBars.exitBtn())
        {
            menuBars.hidden();
            lv_obj_set_hidden(mainBar, false);
            return;
        }
        for (int i = 0; i < size; i++)
        {
            if (obj == menuBars.obj(i))
            {
                if (settings_menu_cfg[i].event_cb != nullptr)
                {
                    menuBars.hidden();
                    settings_menu_cfg[i].event_cb();
                }
                return;
            }
        }
    }
}


/*****************************************************************
 *
 *          ! Switch Class
 *
 */
class Switch
{
public:
    typedef struct
    {
        const char *name;
        void (*cb)(uint8_t, bool);
    } switch_cfg_t;

    typedef void (*exit_cb)();
    Switch()
    {
        _swCont = nullptr;
    }
    ~Switch()
    {
        if (_swCont)
            lv_obj_del(_swCont);
        _swCont = nullptr;
    }

    void create(switch_cfg_t *cfg, uint8_t count, exit_cb cb, lv_obj_t *parent = nullptr)
    {
        static lv_style_t swlStyle;
        lv_style_init(&swlStyle);
        lv_style_set_radius(&swlStyle, LV_OBJ_PART_MAIN, 0);
        lv_style_set_bg_color(&swlStyle, LV_OBJ_PART_MAIN, LV_COLOR_GRAY);
        lv_style_set_bg_opa(&swlStyle, LV_OBJ_PART_MAIN, LV_OPA_0);
        lv_style_set_border_width(&swlStyle, LV_OBJ_PART_MAIN, 0);
        lv_style_set_border_opa(&swlStyle, LV_OBJ_PART_MAIN, LV_OPA_50);
        lv_style_set_text_color(&swlStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);
        lv_style_set_image_recolor(&swlStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);

        if (parent == nullptr)
        {
            parent = lv_scr_act();
        }
        _exit_cb = cb;

        _swCont = lv_cont_create(parent, NULL);
        lv_obj_set_size(_swCont, LV_HOR_RES, LV_VER_RES - 30);
        lv_obj_align(_swCont, NULL, LV_ALIGN_CENTER, 0, 0);
        lv_obj_add_style(_swCont, LV_OBJ_PART_MAIN, &swlStyle);

        _count = count;
        _sw = new lv_obj_t *[count];
        _cfg = new switch_cfg_t[count];

        memcpy(_cfg, cfg, sizeof(switch_cfg_t) * count);

        lv_obj_t *prev = nullptr;
        for (int i = 0; i < count; i++)
        {
            lv_obj_t *la1 = lv_label_create(_swCont, NULL);
            lv_label_set_text(la1, cfg[i].name);
            i == 0 ? lv_obj_align(la1, NULL, LV_ALIGN_IN_TOP_LEFT, 30, 20) : lv_obj_align(la1, prev, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
            _sw[i] = lv_imgbtn_create(_swCont, NULL);
            lv_imgbtn_set_src(_sw[i], LV_BTN_STATE_RELEASED, &off);
            lv_imgbtn_set_src(_sw[i], LV_BTN_STATE_PRESSED, &off);
            lv_imgbtn_set_src(_sw[i], LV_BTN_STATE_CHECKED_RELEASED, &off);
            lv_imgbtn_set_src(_sw[i], LV_BTN_STATE_CHECKED_PRESSED, &off);
            lv_obj_set_click(_sw[i], true);

            lv_obj_align(_sw[i], la1, LV_ALIGN_OUT_RIGHT_MID, 80, 0);
            lv_obj_set_event_cb(_sw[i], __switch_event_cb);
            prev = la1;
        }

        _exitBtn = lv_imgbtn_create(_swCont, NULL);
        lv_imgbtn_set_src(_exitBtn, LV_BTN_STATE_RELEASED, &iexit);
        lv_imgbtn_set_src(_exitBtn, LV_BTN_STATE_PRESSED, &iexit);
        lv_imgbtn_set_src(_exitBtn, LV_BTN_STATE_CHECKED_RELEASED, &iexit);
        lv_imgbtn_set_src(_exitBtn, LV_BTN_STATE_CHECKED_PRESSED, &iexit);
        lv_obj_set_click(_exitBtn, true);

        lv_obj_align(_exitBtn, _swCont, LV_ALIGN_IN_BOTTOM_MID, 0, -5);
        lv_obj_set_event_cb(_exitBtn, __switch_event_cb);

        _switch = this;
    }

    void align(const lv_obj_t *base, lv_align_t align, lv_coord_t x = 0, lv_coord_t y = 0)
    {
        lv_obj_align(_swCont, base, align, x, y);
    }

    void hidden(bool en = true)
    {
        lv_obj_set_hidden(_swCont, en);
    }

    static void __switch_event_cb(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_SHORT_CLICKED)
        {
            Serial.println("LV_EVENT_SHORT_CLICKED");
            if (obj == _switch->_exitBtn)
            {
                if (_switch->_exit_cb != nullptr)
                {
                    _switch->_exit_cb();
                    return;
                }
            }
        }

        if (event == LV_EVENT_SHORT_CLICKED)
        {
            Serial.println("LV_EVENT_VALUE_CHANGED");
            for (int i = 0; i < _switch->_count; i++)
            {
                lv_obj_t *sw = _switch->_sw[i];
                if (obj == sw)
                {
                    const void *src = lv_imgbtn_get_src(sw, LV_BTN_STATE_RELEASED);
                    const void *dst = src == &off ? &on : &off;
                    bool en = src == &off;
                    lv_imgbtn_set_src(sw, LV_BTN_STATE_RELEASED, dst);
                    lv_imgbtn_set_src(sw, LV_BTN_STATE_PRESSED, dst);
                    lv_imgbtn_set_src(sw, LV_BTN_STATE_CHECKED_RELEASED, dst);
                    lv_imgbtn_set_src(sw, LV_BTN_STATE_CHECKED_PRESSED, dst);
                    if (_switch->_cfg[i].cb != nullptr)
                    {
                        _switch->_cfg[i].cb(i, en);
                    }
                    return;
                }
            }
        }
    }

    void setStatus(uint8_t index, bool en)
    {
        if (index > _count)
            return;
        lv_obj_t *sw = _sw[index];
        const void *dst = en ? &on : &off;
        lv_imgbtn_set_src(sw, LV_BTN_STATE_RELEASED, dst);
        lv_imgbtn_set_src(sw, LV_BTN_STATE_PRESSED, dst);
        lv_imgbtn_set_src(sw, LV_BTN_STATE_CHECKED_RELEASED, dst);
        lv_imgbtn_set_src(sw, LV_BTN_STATE_CHECKED_PRESSED, dst);
    }

private:
    static Switch *_switch;
    lv_obj_t *_swCont = nullptr;
    uint8_t _count;
    lv_obj_t **_sw = nullptr;
    switch_cfg_t *_cfg = nullptr;
    lv_obj_t *_exitBtn = nullptr;
    exit_cb _exit_cb = nullptr;
};

Switch *Switch::_switch = nullptr;

/*****************************************************************
 *
 *          ! Task Class
 *
 */
class Task
{
public:
    Task()
    {
        _handler = nullptr;
        _cb = nullptr;
    }
    ~Task()
    {
        if (_handler == nullptr)
            return;
        Serial.println("Free Task Func");
        lv_task_del(_handler);
        _handler = nullptr;
        _cb = nullptr;
    }

    void create(lv_task_cb_t cb, uint32_t period = 1000, lv_task_prio_t prio = LV_TASK_PRIO_LOW)
    {
        _handler = lv_task_create(cb, period, prio, NULL);
    };

private:
    lv_task_t *_handler = nullptr;
    lv_task_cb_t _cb = nullptr;
};

/*****************************************************************
 *
 *          ! GLOBAL VALUE
 *
 */
static Keyboard *kb = nullptr;
static Loader *pl = nullptr;
static WifiList *list = nullptr;
static Task *task = nullptr;
static Ticker *gTicker = nullptr;
static MBox *mbox = nullptr;

static char ssid[64], password[64];

/*****************************************************************
 *
 *          !WIFI EVENT
 *
 */
void wifi_connect_status(bool result)
{
    if (gTicker != nullptr)
    {
        delete gTicker;
        gTicker = nullptr;
    }
    if (kb != nullptr)
    {
        delete kb;
        kb = nullptr;
    }
    if (pl != nullptr)
    {
        delete pl;
        pl = nullptr;
    }
    /*if (result) {
        bar.show(LV_STATUS_BAR_WIFI);
    } else {
        bar.hidden(LV_STATUS_BAR_WIFI);
    }*/
    menuBars.hidden(false);
}

void wifi_kb_event_cb(Keyboard::kb_event_t event)
{
    if (event == 0)
    {
        kb->hidden();
        Serial.println(kb->getText());
        strlcpy(password, kb->getText(), sizeof(password));
        wifiManager->setup(String(ssid), String(password));
        wifiManager->on();
        gTicker = new Ticker;
        gTicker->once_ms(5 * 1000, []() {
            wifi_connect_status(false);
        });
    }
    else if (event == 1)
    {
        delete kb;
        delete pl;
        pl = nullptr;
        kb = nullptr;
        menuBars.hidden(false);
    }
}

void wifi_sw_event_cb(uint8_t index, bool en)
{
    switch (index)
    {
    case 0:
        if (en)
        {
            wifiManager->on();
        }
        else
        {
            wifiManager->off();
        }
        break;
    case 1:
        
        break;
    case 2:
        if (!WiFi.isConnected())
        {
            //TODO pop-up window
            Serial.println("WiFi is no connect");
            return;
        }
        else
        {
            configTzTime(RTC_TIME_ZONE, "pool.ntp.org");
        }
        break;
    default:
        break;
    }
}

void wifi_list_cb(const char *txt)
{
    strlcpy(ssid, txt, sizeof(ssid));
    delete list;
    list = nullptr;
    kb = new Keyboard;
    kb->create();
    kb->align(bar.self(), LV_ALIGN_OUT_BOTTOM_MID);
    kb->setKeyboardEvent(wifi_kb_event_cb);
}

void wifi_list_add(const char *ssid)
{
    
}

static void wifi_settings_event_cb()
{
    auto wifiSettings = new WifiSettings(wifiManager);
    wifiSettings->on_close([wifiSettings]()
    {
        menuBars.hidden(false);
        delete wifiSettings;
    });
    wifiSettings->show(lv_scr_act());
}

static void wifi_destory()
{
    Serial.printf("globalIndex:%d\n", globalIndex);
    switch (globalIndex)
    {
    //! wifi management main
    case 0:
        menuBars.hidden(false);
        break;
    //! wifi ap list
    case 1:
        if (list != nullptr)
        {
            delete list;
            list = nullptr;
        }
        if (gTicker != nullptr)
        {
            delete gTicker;
            gTicker = nullptr;
        }
        if (kb != nullptr)
        {
            delete kb;
            kb = nullptr;
        }
        if (pl != nullptr)
        {
            delete pl;
            pl = nullptr;
        }
        break;
    //! wifi keyboard
    case 2:
        if (gTicker != nullptr)
        {
            delete gTicker;
            gTicker = nullptr;
        }
        if (kb != nullptr)
        {
            delete kb;
            kb = nullptr;
        }
        if (pl != nullptr)
        {
            delete pl;
            pl = nullptr;
        }
        break;
    case 3:
        break;
    default:
        break;
    }
    globalIndex--;
}

/*****************************************************************
 *
 *          !SETTING EVENT
 *
 */
static void setting_event_cb()
{
    
    testView = new SettingsView("Test view");
    testView->on_close([]() {
        delete testView;
        testView = nullptr;
        menuBars.hidden(false);
    });
    testView->show(lv_scr_act());
}

/*****************************************************************
 *
 *          ! LIGHT EVENT
 *
 */
static void light_sw_event_cb(uint8_t index, bool en)
{
    //Add lights that need to be controlled
}

/*****************************************************************
 *
 *          ! MBOX EVENT
 *
 */
static lv_obj_t *mbox1 = nullptr;

static void create_mbox(const char *txt, lv_event_cb_t event_cb)
{
    if (mbox1 != nullptr)
        return;
    static const char *btns[] = {"Ok", ""};
    mbox1 = lv_msgbox_create(lv_scr_act(), NULL);
    lv_msgbox_set_text(mbox1, txt);
    lv_msgbox_add_btns(mbox1, btns);
    lv_obj_set_width(mbox1, LV_HOR_RES - 40);
    lv_obj_set_event_cb(mbox1, event_cb);
    lv_obj_align(mbox1, NULL, LV_ALIGN_CENTER, 0, 0);
}

static void destory_mbox()
{
    if (pl != nullptr)
    {
        delete pl;
        pl = nullptr;
    }
    if (list != nullptr)
    {
        delete list;
        list = nullptr;
    }
    if (mbox1 != nullptr)
    {
        lv_obj_del(mbox1);
        mbox1 = nullptr;
    }
}

void toggleStatusBar(bool hidden)
{
    bar.set_hidden(hidden);
}

/*****************************************************************
 *
 *          ! SD CARD EVENT
 *
 */

static void sd_event_cb()
{
}

/*****************************************************************
*
 *          ! Modules EVENT
 *
 */
static void modules_event_cb()
{
}

/*****************************************************************
*
 *          ! Camera EVENT
 *
 */

static void camera_event_cb()
{
}
