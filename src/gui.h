#include "hardware/Wifi.h"
#include "networking/signalk_socket.h"
#include "system/configurable.h"
#include "system/events.h"
#include "ui/statusbar.h"
#include "ui/dynamic_gui.h"
#include "ui/themes.h"
#include "hardware/hardware.h"
#include <list>

#ifndef __GUI_H
#define __GUI_H

#define ARROWS_DISAPPEAR_TIME 4000

enum MsgTopic_t
{
    Wifi_Problem
};

class Gui : public Configurable
{
public:
    Gui() : Configurable("/config/gui")
    {
        //set default name
        strcpy(watch_name, "TWatchSK");
        load();
    }
    void setup_gui(WifiManager *wifi, SignalKSocket *socket, Hardware *hardware);
    void update_step_counter(uint32_t counter);
    void update_battery_level();
    void hide_status_bar(bool hidden);
    void hide_main_bar(bool hidden);
    void load_config_from_file(const JsonObject &json) override;
    void save_config_to_file(JsonObject &json) override;
    WifiManager *get_wifi_manager() { return wifiManager; }
    SignalKSocket *get_sk_socket() { return ws_socket; }
    Hardware *get_hardware() { return hardware_; }
    bool get_time_24hour_format() { return time_24hour_format; }
    void set_time_24hour_format(bool value) { time_24hour_format = value; }
    int get_screen_timeout() { return screen_timeout; }
    void set_screen_timeout(int value) { screen_timeout = value; }
    void set_temporary_screen_timeout(int value);
    void clear_temporary_screen_timeout();
    int get_wakeup_count() { return wakeup_count; }
    void increment_wakeup_count() { wakeup_count++; }
    uint8_t get_display_brightness() { return display_brightness; }
    uint8_t get_adjusted_display_brightness();
    void set_display_brightness(uint8_t value) { display_brightness = value; }
    void on_power_event(PowerCode_t code, uint32_t arg);
    int8_t get_timezone_id() { return timezone_id; }
    void set_timezone_id(int8_t new_tz_id) { timezone_id = new_tz_id; }
    StatusBar *get_bar()
    {
        return bar;
    }
    void theme_changed();
    const char* get_watch_name() { return watch_name; }
    void set_watch_name(const char *new_name) { strcpy(watch_name, new_name); }
    bool is_active_view_dynamic() { return is_active_view_dynamic_; }
    void set_is_active_view_dynamic(bool new_value);
    bool get_gui_needs_saved() { return gui_needs_saved; }
    void set_gui_needs_saved(bool new_value) { gui_needs_saved = new_value; }
    void set_display_next_pending_message(bool value) { display_next_pending_message_ = value; }
    void display_next_message(bool delete_first_message);
    void update_pending_messages();
    void handle_gui_queue();
    void show_home();
    void show_settings();
    void toggle_wifi();
private:
    static void lv_update_task(struct _lv_task_t *);
    static void lv_battery_task(struct _lv_task_t *);
    static void lv_mainbar_callback(lv_obj_t*obj, lv_event_t event);
    void update_time();
    void update_tiles_valid_points(int count);
    char *message_from_code(GuiMessageCode_t code);
    void update_gui();
    String current_time();
    void update_arrows_visibility(bool left, bool right);
    static void msg_box_callback(lv_obj_t * obj, lv_event_t event);
    static void hide_arrows_task_cb(lv_task_t * task);

    WifiManager *wifiManager = NULL;
    SignalKSocket *ws_socket = NULL;
    Hardware *hardware_ = NULL;
    lv_obj_t *mainBar = NULL;
    lv_obj_t *timeLabel = NULL;
    lv_obj_t *timeSuffixLabel = NULL;
    lv_obj_t *secondsLabel = NULL;
    lv_obj_t *menuBtn = NULL;
    lv_obj_t *watch_face = NULL;
    lv_obj_t *dayDateLabel = NULL;
    lv_obj_t *watchNameLabel = NULL;
    lv_obj_t *msgBox = NULL;
    lv_obj_t *arrow_left = NULL;
    lv_obj_t *arrow_right = NULL;
    StatusBar *bar = NULL;
    DynamicGui*dynamic_gui = NULL;

    struct PendingMsg_t
    {
        String msg_text;
        MsgTopic_t msg_topic;
        String msg_time;;
        int msg_count = 0;
    };

    std::list<PendingMsg_t> pending_messages_;

    bool time_24hour_format = false;
    int screen_timeout = 30; // only until it's first changed
    int saved_screen_timeout = 30;
    bool screen_timeout_is_temporary = false;
    int8_t timezone_id = 0; // Index of the array of timezones in the timezone Roller selector widget
    int wakeup_count = 0; // restarts at zero at each startup
    uint8_t display_brightness = 5; // only until it's first changed
    lv_point_t*tile_valid_points = NULL; //this is for tile navigation matrix to allow user navigation in multiple directions
    int tile_valid_points_count = 0; //number of matrix points
    char watch_name[16] = "";
    bool is_active_view_dynamic_ = false;
    bool gui_needs_saved = false;
    bool display_next_pending_message_ = true;
    long arrows_hide_at_time = -1;

    StatusBarIcon* batteryPercent_;
    StatusBarIcon* stepCounterIcon_;
    StatusBarIcon* stepCounterSteps_;
    StatusBarIcon* WifiIcon_;
    StatusBarIcon* SKIcon_;
    StatusBarIcon* pendingMessagesIcon_;
};
#endif /*__GUI_H */