#include "hardware/Wifi.h"
#include "networking/signalk_socket.h"
#include "system/configurable.h"
#include "system/events.h"
#include "ui/statusbar.h"
#include "ui/menubar.h"

#ifndef __GUI_H
#define __GUI_H

class Gui : public Configurable
{
public:
    Gui() : Configurable("/config/gui")
    {
        load();
    }
    void setup_gui(WifiManager *wifi, SignalKSocket *socket);
    void update_step_counter(uint32_t counter);
    void update_battery_icon(lv_icon_battery_t index);
    void update_battery_level();
    void toggle_status_bar(bool hidden);
    void toggle_main_bar(bool hidden);
    void load_config_from_file(const JsonObject &json) override;
    void save_config_to_file(JsonObject &json) override;
    WifiManager *get_wifi_manager() { return wifiManager; }
    SignalKSocket *get_sk_socket() { return ws_socket; }
    bool get_time_24hour_format() { return time_24hour_format; }
    void set_time_24hour_format(bool value) { time_24hour_format = value; }
    void toggle_status_bar_icon(lv_icon_status_bar_t icon, bool hidden);
    int get_screen_timeout() { return screen_timeout; }
    void set_screen_timeout(int value) { screen_timeout = value; }

private:
    static void lv_update_task(struct _lv_task_t *);
    static void lv_battery_task(struct _lv_task_t *);
    void update_time();
    char *message_from_code(GuiEventCode_t code);

    WifiManager *wifiManager = NULL;
    SignalKSocket *ws_socket = NULL;
    lv_obj_t *mainBar = NULL;
    lv_obj_t *timeLabel = NULL;
    lv_obj_t *timeSuffixLabel = NULL;
    lv_obj_t *menuBtn = NULL;
    MenuBar *menuBars = NULL;
    StatusBar *bar = NULL;

    bool time_24hour_format = false;
    int screen_timeout = 10;
    String time_zone = "";
    int display_brightness = 128;
};
#endif /*__GUI_H */