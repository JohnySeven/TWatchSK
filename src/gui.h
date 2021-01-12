/*
Copyright (c) 2019 lewis he
This is just a demonstration. Most of the functions are not implemented.
The main implementation is low-power standby. 
The off-screen standby (not deep sleep) current is about 4mA.
Select standard motherboard and standard backplane for testing.
Created by Lewis he on October 10, 2019.
*/
#include "hardware/Wifi.h"
#include "networking/signalk_socket.h"
#include "system/configurable.h"
#include "system/events.h"

#ifndef __GUI_H
#define __GUI_H

typedef enum
{
    LV_ICON_BAT_EMPTY,
    LV_ICON_BAT_1,
    LV_ICON_BAT_2,
    LV_ICON_BAT_3,
    LV_ICON_BAT_FULL,
    LV_ICON_CHARGE,
    LV_ICON_CALCULATION
} lv_icon_battery_t;

typedef enum
{
    LV_STATUS_BAR_BATTERY_LEVEL = 0,
    LV_STATUS_BAR_BATTERY_ICON = 1,
    LV_STATUS_BAR_WIFI = 2,
    LV_STATUS_BAR_SIGNALK = 3
} lv_icon_status_bar_t;

class Gui : public Configurable
{
public:
    Gui() : Configurable("gui") { }
    void setup_gui(WifiManager *wifi, SignalKSocket *socket);
    void updateStepCounter(uint32_t counter);
    void updateBatteryIcon(lv_icon_battery_t index);
    void updateBatteryLevel();
    void toggleStatusBar(bool hidden);
    void load_config_from_file(const JsonObject &json) override;
    void save_config_to_file(JsonObject &json) override;
private:
    static void lv_update_task(struct _lv_task_t *);
    static void lv_battery_task(struct _lv_task_t *);
    static void updateTime();
    char * message_from_code(GuiEventCode_t code);

    bool is24hourFormat = false;
    int screenTimeout = 5;
    String timeZone = "";
    int brightness = 128;
};
#endif /*__GUI_H */