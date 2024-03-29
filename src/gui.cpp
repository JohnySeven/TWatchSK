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
#include "ui/display_settings.h"
#include "ui/wakeup_settings.h"
#include "hardware/hardware.h"
#include "system/async_dispatcher.h"
#include <functional>
#include "sounds/beep.h"
#include "sounds/alert.h"
#include "ui/ui_functions.h"
/* In order to use std::bind to attach to method we need to use arguments placeholders that will map arguments to final method.
 * So in function setup_gui on line with attach_power_callback we just type _1 or _2 without whole namespace.
 */
using std::placeholders::_1;
using std::placeholders::_2;

LV_FONT_DECLARE(Geometr);
LV_FONT_DECLARE(Ubuntu);
LV_FONT_DECLARE(roboto70);
LV_FONT_DECLARE(roboto30);
LV_FONT_DECLARE(lv_font_montserrat_16);
LV_FONT_DECLARE(lv_font_montserrat_22);
LV_IMG_DECLARE(sk_status);
LV_IMG_DECLARE(signalk_48px);
LV_IMG_DECLARE(menu);
LV_IMG_DECLARE(wifi_48px);
LV_IMG_DECLARE(info_48px);
LV_IMG_DECLARE(time_48px);
LV_IMG_DECLARE(display_48px);
LV_IMG_DECLARE(wakeup_48px);

LV_IMG_DECLARE(step);
LV_IMG_DECLARE(sk_statusbar_icon);

const char *GUI_TAG = "GUI";

static void main_menu_event_cb(lv_obj_t *obj, lv_event_t event)
{
    Gui *gui = (Gui *)obj->user_data;
    if (event == LV_EVENT_SHORT_CLICKED)
    { //!  Event callback is in here
        gui->show_settings();
    }
}

void Gui::setup_gui(WifiManager *wifi, SignalKSocket *socket, Hardware *hardware)
{
    twatchsk::UI_Functions = new twatchsk::UIFunctions(this);

    wifiManager = wifi;
    ws_socket = socket;

    hardware_ = hardware;
    // attach power events to GUI
    hardware->attach_power_callback(std::bind(&Gui::on_power_event, this, _1, _2));
    // Hardware class needs to know what is the screen timeout, wire it to Gui::get_screen_timeout func
    hardware->set_screen_timeout_func(std::bind(&Gui::get_screen_timeout, this));
    lv_obj_t *scr = lv_scr_act();
    // set the theme
    uint32_t flag = twatchsk::dark_theme_enabled ? LV_THEME_MATERIAL_FLAG_DARK : LV_THEME_MATERIAL_FLAG_LIGHT;
    LV_THEME_DEFAULT_INIT(lv_theme_get_color_primary(), lv_theme_get_color_secondary(), flag, // Initiate the theme
                          lv_theme_get_font_small(), lv_theme_get_font_normal(), lv_theme_get_font_subtitle(),
                          lv_theme_get_font_title());

    bar = new StatusBar();
    bar->setup(scr);
    // intialize status bar icons
    // battery/charge icon and battery percent
    batteryPercent_ = bar->create_text_icon(LV_SYMBOL_BATTERY_EMPTY "0%", StatusBarLocation::Right);
    update_battery_level();
    // step counter icon and number of steps
    stepCounterIcon_ = bar->create_image_icon(&step, StatusBarLocation::Left);
    stepCounterSteps_ = bar->create_text_icon("0", StatusBarLocation::Left);
    // wifi status icon
    WifiIcon_ = bar->create_text_icon(LV_SYMBOL_WIFI, StatusBarLocation::Right, StatusBarIconStatus::Hidden);
    // signal k status icon
    SKIcon_ = bar->create_image_icon(&sk_statusbar_icon, StatusBarLocation::Right, StatusBarIconStatus::Hidden);
    // messages counter
    pendingMessagesIcon_ = bar->create_text_icon(LV_SYMBOL_BELL " 0", StatusBarLocation::Left, StatusBarIconStatus::Hidden);

    //! main
    static lv_style_t mainStyle;
    lv_style_init(&mainStyle);
    lv_style_set_radius(&mainStyle, LV_OBJ_PART_MAIN, 0);
    lv_style_set_border_width(&mainStyle, LV_OBJ_PART_MAIN, 0);

    mainBar = lv_tileview_create(scr, NULL);
    lv_obj_add_style(mainBar, LV_OBJ_PART_MAIN, &mainStyle);
    lv_obj_set_pos(mainBar, 0, bar->height());
    // we need to update is_active_view_dynamic_ field
    mainBar->user_data = this;
    lv_obj_set_event_cb(mainBar, Gui::lv_mainbar_callback);

    watch_face = lv_cont_create(mainBar, NULL);
    lv_obj_add_style(watch_face, LV_OBJ_PART_MAIN, &mainStyle);
    lv_obj_set_pos(watch_face, 0, 0);
    lv_obj_set_size(watch_face, LV_HOR_RES, LV_VER_RES - bar->height());
    lv_tileview_add_element(mainBar, watch_face);

    //! Time
    static lv_style_t timeStyle; // for minutes and hours
    lv_style_copy(&timeStyle, &mainStyle);
    lv_style_set_text_font(&timeStyle, LV_STATE_DEFAULT, &roboto70);

    watchNameLabel = lv_label_create(watch_face, NULL);
    lv_obj_set_style_local_text_font(watchNameLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);

    timeLabel = lv_label_create(watch_face, NULL);
    lv_obj_add_style(timeLabel, LV_OBJ_PART_MAIN, &timeStyle);

    static lv_style_t timeSuffixStyle; // for am/pm and seconds
    lv_style_copy(&timeSuffixStyle, &mainStyle);
    lv_style_set_text_font(&timeSuffixStyle, LV_STATE_DEFAULT, &roboto30);

    timeSuffixLabel = lv_label_create(watch_face, NULL);
    lv_obj_add_style(timeSuffixLabel, LV_OBJ_PART_MAIN, &timeSuffixStyle);

    secondsLabel = lv_label_create(watch_face, NULL);
    lv_obj_add_style(secondsLabel, LV_OBJ_PART_MAIN, &timeSuffixStyle);

    dayDateLabel = lv_label_create(watch_face, NULL);

    update_time();

    menuBtn = lv_imgbtn_create(watch_face, NULL);

    lv_imgbtn_set_src(menuBtn, LV_BTN_STATE_RELEASED, &menu);
    lv_imgbtn_set_src(menuBtn, LV_BTN_STATE_PRESSED, &menu);
    lv_imgbtn_set_src(menuBtn, LV_BTN_STATE_CHECKED_RELEASED, &menu);
    lv_imgbtn_set_src(menuBtn, LV_BTN_STATE_CHECKED_PRESSED, &menu);
    twatchsk::update_imgbtn_color(menuBtn); // make the four little squares be the correct color for the theme

    lv_obj_align(menuBtn, watch_face, LV_ALIGN_OUT_BOTTOM_MID, 0, -100);
    menuBtn->user_data = this;

    lv_obj_set_event_cb(menuBtn, main_menu_event_cb);

    auto update_task = lv_task_create(lv_update_task, 1000, LV_TASK_PRIO_LOWEST, NULL);
    auto batery_update_task = lv_task_create(lv_battery_task, 30000, LV_TASK_PRIO_LOWEST, NULL);

    update_task->user_data = this;
    batery_update_task->user_data = this;

    dynamic_gui = new DynamicGui();

    dynamic_gui->initialize();
    int dynamic_view_count = 0;

    if (!dynamic_gui->load_file("/sk_view.json", mainBar, socket, dynamic_view_count))
    {
        ESP_LOGW(GUI_TAG, "Failed to load dynamic views!");
    }

    dynamic_view_count++;

    update_tiles_valid_points(dynamic_view_count);
    lv_tileview_set_valid_positions(mainBar, tile_valid_points, tile_valid_points_count);
    lv_tileview_set_edge_flash(mainBar, true);

    //setup guide arrows (left/right) to indicate swype direction
    arrow_left = lv_label_create(scr, NULL);
    lv_label_set_text(arrow_left, LV_SYMBOL_LEFT);
    lv_obj_set_pos(arrow_left, 9, bar->height() + 5);
    arrow_right = lv_label_create(scr, NULL);
    lv_obj_set_pos(arrow_right, LV_HOR_RES - 15, bar->height() + 5);
    lv_label_set_text(arrow_right, LV_SYMBOL_RIGHT);
    //show arrows after booting
    update_arrows_visibility(false, tile_valid_points_count > 1);
}

void Gui::update_tiles_valid_points(int count)
{
    if (tile_valid_points != NULL)
    {
        free(tile_valid_points);
        tile_valid_points = NULL;
        tile_valid_points_count = 0;
    }

    tile_valid_points = (lv_point_t *)malloc(sizeof(lv_point_t) * count);
    for (int i = 0; i < count; i++)
    {
        tile_valid_points[i].x = i;
        tile_valid_points[i].y = 0;
    }
    tile_valid_points_count = count;
}

void Gui::update_step_counter(uint32_t counter)
{
    char buf[20];
    sprintf(buf, "%d", counter);
    stepCounterSteps_->set_text(buf);
}

void Gui::update_time()
{
    time_t now;
    struct tm info;
    char buf[64];
    char day_date_buf[32];
    time(&now);
    localtime_r(&now, &info);

    lv_obj_align(watchNameLabel, NULL, LV_ALIGN_IN_TOP_MID, 0, 3);
    lv_label_set_text(watchNameLabel, get_watch_name());

    lv_obj_align(timeLabel, NULL, LV_ALIGN_IN_TOP_MID, -23, 25);

    if (time_24hour_format)
    {
        lv_obj_set_hidden(timeSuffixLabel, true);
        strftime(buf, sizeof(buf), "%H:%M", &info);                          // see http://www.cplusplus.com/reference/ctime/strftime/
        strftime(day_date_buf, sizeof(day_date_buf), "%a %e %b, %G", &info); // day/month format
        lv_obj_set_hidden(timeSuffixLabel, true);                            // hide the suffix label as it's not needed
    }
    else
    {
        lv_obj_set_hidden(timeSuffixLabel, false);
        strftime(buf, sizeof(buf), "%I:%M", &info);
        strftime(day_date_buf, sizeof(day_date_buf), "%a %b %e, %G", &info); // month/day format
        if (info.tm_hour > 12)
        {
            lv_label_set_text(timeSuffixLabel, "pm");
        }
        else
        {
            lv_label_set_text(timeSuffixLabel, "am");
        }
        lv_obj_align(timeSuffixLabel, timeLabel, LV_ALIGN_OUT_RIGHT_MID, 3, -22);
    }
    lv_label_set_text(this->timeLabel, buf);
    lv_obj_align(secondsLabel, timeLabel, LV_ALIGN_OUT_RIGHT_MID, 4, 13);
    lv_label_set_text_fmt(secondsLabel, ":%.2d", info.tm_sec);

    lv_obj_set_style_local_text_font(dayDateLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_22);
    lv_obj_align(dayDateLabel, watch_face, LV_ALIGN_OUT_BOTTOM_MID, 0, -30);
    lv_label_set_text(dayDateLabel, day_date_buf);
}

void Gui::update_battery_level()
{
    TTGOClass *ttgo = TTGOClass::getWatch();
    int level = ttgo->power->getBattPercentage();
    char buff[10];
    auto batterySymbol = LV_SYMBOL_BATTERY_FULL;

    if (ttgo->power->isChargeing())
    {
        batterySymbol = LV_SYMBOL_CHARGE;
    }
    else
    {
        if (level > 80)
        {
            batterySymbol = LV_SYMBOL_BATTERY_3;
        }
        else if (level > 45)
        {
            batterySymbol = LV_SYMBOL_BATTERY_2;
        }
        else if (level > 20)
        {
            batterySymbol = LV_SYMBOL_BATTERY_1;
        }
        else
        {
            batterySymbol = LV_SYMBOL_BATTERY_EMPTY;
        }
    }

    sprintf(buff, "%s %d%%", batterySymbol, level);
    batteryPercent_->set_text(buff);
}

char *Gui::message_from_code(GuiMessageCode_t code)
{
    switch (code)
    {
    case GuiMessageCode_t::GUI_WARN_SK_REJECTED:
        return LOC_SIGNALK_REQUEST_REJECTED;
    case GuiMessageCode_t::GUI_WARN_WIFI_DISCONNECTED:
        return LOC_WIFI_LOST_CONNECTION;
    case GuiMessageCode_t::GUI_WARN_WIFI_CONNECTION_FAILED:
        return LOC_WIFI_ASSOC_FAIL;
    case GuiMessageCode_t::GUI_WARN_SK_LOST_CONNECTION:
        return LOC_SIGNALK_CONNECTION_LOST;
    case GuiMessageCode_t::GUI_INFO_BATTERY_CHARGE_COMPLETE:
        return LOC_POWER_BATTERY_CHARGED;
    default:
        return NULL;
    };
}

void Gui::on_power_event(PowerCode_t code, uint32_t arg)
{
    if (code == PowerCode_t::POWER_LEAVE_LOW_POWER)
    {
        auto ttgo = TTGOClass::getWatch();
        ttgo->bl->adjust(get_adjusted_display_brightness());
        bar->dont_refresh();
        update_step_counter(ttgo->bma->getCounter());
        update_battery_level();
        increment_wakeup_count();
        bar->do_refresh();

        switch (arg)
        {
        case WAKEUP_BUTTON:
            clear_temporary_screen_timeout(); // waking up with a button press - if the last timeout was temporary, clear it
            // hardware_->get_player()->play_raw_from_const("alert", beep_sound, beep_sound_len, 1);
            break;
        case WAKEUP_ACCELEROMETER: // waking up with double tap or tilt
            set_temporary_screen_timeout(2);
            break;
        // case WAKEUP_TOUCH:                   // not yet implemented
        //     set_temporary_screen_timeout(2); // change this when it is implemented
        //     break;
        default:
            clear_temporary_screen_timeout();
        }
        update_gui();
        lv_disp_trig_activity(NULL);
        // update subscriptions if we are on dynamic view
        if (is_active_view_dynamic_)
        {
            ws_socket->update_subscriptions();
        }

        update_arrows_visibility();        
    }
    else if (code == PowerCode_t::POWER_CHARGING_ON)
    {
        update_battery_level();
    }
    else if (code == PowerCode_t::POWER_CHARGING_OFF || code == PowerCode_t::POWER_CHARGING_DONE)
    {
        update_battery_level();
        if (code == PowerCode_t::POWER_CHARGING_DONE) // notify user the watch is fully charged
        {
            post_gui_warning(GuiMessageCode_t::GUI_INFO_BATTERY_CHARGE_COMPLETE);
        }
    }
    else if (code == PowerCode_t::WALK_STEP_COUNTER_UPDATED)
    {
        update_step_counter(arg);
    }
    else if (code == PowerCode_t::DOUBLE_TAP_DETECTED) // while watch is awake - this switches between LIGHT and DARK theme
    {
        if (screen_timeout_is_temporary) // double-tap occurs during a quickie "time check" that resulted from a double-tap or tilt
        {
            clear_temporary_screen_timeout(); // leave the screen on for the normal timeout time, then change the theme
        }
        twatchsk::dark_theme_enabled = !twatchsk::dark_theme_enabled;
        uint32_t flag = twatchsk::dark_theme_enabled ? LV_THEME_MATERIAL_FLAG_DARK : LV_THEME_MATERIAL_FLAG_LIGHT; // Create a theme flag and set it to the new theme
        LV_THEME_DEFAULT_INIT(lv_theme_get_color_primary(), lv_theme_get_color_secondary(), flag,                  // Initiate the theme with the flag
                              lv_theme_get_font_small(), lv_theme_get_font_normal(), lv_theme_get_font_subtitle(),
                              lv_theme_get_font_title());
        set_display_brightness(twatchsk::dark_theme_enabled ? 1 : 5);
        auto ttgo = TTGOClass::getWatch();
        ttgo->bl->adjust(get_adjusted_display_brightness());
        View::invoke_theme_changed();            // calls theme_changed() for every descendant of View class
        this->theme_changed();                   // updates theme for status bar icons (because StatusBar is not a descendant of View class)
        if (View::get_active_views_count() == 0) // we're on the home screen, so save the new theme to SPIFFS immediately
        {
            // JD: commented out for now - causes crash :-/
            /*twatchsk::run_async("GUI Settings save", [this]()
                                {
                                    delay(100);
                                    this->save();
                                });*/
        }
        else
        {
            gui_needs_saved = true; // flag to save the new theme to SPIFFS later, when NavigationView is closed
        }
    }
}

void Gui::lv_update_task(struct _lv_task_t *data)
{
    Gui *gui = (Gui *)data->user_data;
    gui->update_gui();
}

void Gui::update_gui()
{
    update_time();

    auto wifiStatus = wifiManager->get_status();
    if (wifiStatus == WifiState_t::Wifi_Connected || wifiStatus == WifiState_t::Wifi_Connecting)
    {
        WifiIcon_->set_status(StatusBarIconStatus::Visible);
    }
    else if (wifiStatus == WifiState_t::Wifi_Disconnected)
    {
        WifiIcon_->set_status(StatusBarIconStatus::Warning);
    }
    else
    {
        WifiIcon_->set_status(StatusBarIconStatus::Hidden);
    }

    if (ws_socket->get_state() == WebsocketState_t::WS_Connected)
    {
        if (ws_socket->get_token_request_pending())
        {
            SKIcon_->set_status(StatusBarIconStatus::Warning);
        }
        else
        {
            SKIcon_->set_status(StatusBarIconStatus::Visible);
        }

        dynamic_gui->update_online(true);
    }
    else
    {
        SKIcon_->set_status(StatusBarIconStatus::Hidden);
        dynamic_gui->update_online(false);
    }

    handle_gui_queue();

    if(arrows_hide_at_time < millis())
    {
        lv_obj_set_hidden(arrow_left, true);
        lv_obj_set_hidden(arrow_right, true);
    }
}

void Gui::handle_gui_queue()
{
    GuiEvent_t event;

    while (read_gui_update(event))
    {
        if (event.event_type == GuiEventType_t::GUI_SHOW_WARNING)
        {
            ESP_LOGI(GUI_TAG, "Show message %d, event=%d, message code=%d!", (int)event.argument, event.event_type, event.message_code);
            char *message = NULL;
            if (event.message_code != GuiMessageCode_t::NONE) // WIFI or SK connection message
            {
                message = message_from_code(event.message_code);
            }
            else // SK Server alarm/alert/warn/emergency, or MDNS-related error message
            {
                message = (char *)event.argument;
            }

            if (message != NULL)
            {
                // create and populate a new PendingMsg_t
                PendingMsg_t new_message;
                new_message.msg_text = message;
                if (event.message_code == GUI_WARN_WIFI_CONNECTION_FAILED || event.message_code == GUI_WARN_WIFI_DISCONNECTED)
                {
                    new_message.msg_topic = Wifi_Problem;
                }
                new_message.msg_time = current_time();
                new_message.msg_count = 1;

                if (pending_messages_.size() == 0) // list is empty
                {
                    pending_messages_.push_back(new_message); // add it to the list
                    hardware_->vibrate(300);                  // just a very brief vibration for each added message
                    // Run beep
                    hardware_->get_player()->play_raw_from_const("alert", beep_sound, beep_sound_len, 3);
                    ESP_LOGI(GUI_TAG, "pending_messages_ empty, so msg added: %s, %s", new_message.msg_text.c_str(), new_message.msg_time.c_str());
                }
                else
                {
                    bool message_found = false;
                    for (std::list<PendingMsg_t>::iterator it = pending_messages_.begin(); it != pending_messages_.end(); ++it)
                    {
                        if (new_message.msg_text == it->msg_text) // this message is already in the list
                        {
                            // update its time and count
                            ESP_LOGI(GUI_TAG, "Msg is already in pending_messages_: %s, %s", new_message.msg_text.c_str(), new_message.msg_time.c_str());
                            it->msg_time = current_time();
                            it->msg_count++;
                            // update the text on the screen to show the latest time and the new count and the new size of pending_messages
                            it = pending_messages_.begin(); // get back to the first message in the list, which is the one currently being displayed
                            String updated_text =
                                it->msg_time + "\n(" + it->msg_count + "x) " + it->msg_text;
                            lv_msgbox_set_text(msgBox, updated_text.c_str());
                            message_found = true;
                            hardware_->vibrate(300); // just a very brief vibration for each added message
                            hardware_->get_player()->play_raw_from_const("alert", beep_sound, beep_sound_len, alert_sound_default_repeat);
                            break;
                        }
                    }

                    if (!message_found) // it's not already in the list
                    {
                        pending_messages_.push_back(new_message); // add it to the list
                        ESP_LOGI(GUI_TAG, "Msg added to pending_messages_: %s, %s", new_message.msg_text.c_str(), new_message.msg_time.c_str());
                        // update the unread-messages count on any currently-displayed message
                        std::list<PendingMsg_t>::iterator it = pending_messages_.begin(); // get the first message - that's the one that's currently displayed
                        String updated_text =                                             // re-create the message text to reflect the new pending_messages_.size()
                            it->msg_time + "\n(" + it->msg_count + "x) " + it->msg_text + "\n\n(" + (String)(pending_messages_.size() - 1) + LOC_UNREAD_MSGS + ")";
                        lv_msgbox_set_text(msgBox, updated_text.c_str()); // display the updated text on the currently-displayed message
                        hardware_->vibrate(300);                          // vibrate for 300 ms (100 ms on, 100 ms off, 100 ms on)
                    }
                }
                ESP_LOGI(GUI_TAG, "There are %d messages in pending_messages", pending_messages_.size());

                update_pending_messages();

                if (display_next_pending_message_) // if there is not already a message displayed
                {
                    display_next_message(false); // false means "do not delete the first message before displaying the next one"
                }
            }
        }
        else if (event.event_type == GuiEventType_t::GUI_SK_DV_UPDATE)
        {
            StaticJsonDocument<512> update;
            auto result = deserializeJson(update, event.argument);

            if (result == DeserializationError::Ok)
            {
                auto path = update["path"].as<String>();
                auto value = update["value"].as<JsonVariant>();
                dynamic_gui->handle_signalk_update(path, value);
            }
            else
            {
                ESP_LOGI(GUI_TAG, "Unable to parse json, error=%s", result.c_str());
            }
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

void Gui::hide_status_bar(bool hidden)
{
    bar->set_hidden(hidden);
}

void Gui::hide_main_bar(bool hidden)
{
    lv_obj_set_hidden(mainBar, hidden);
}

uint8_t Gui::get_adjusted_display_brightness()
{
    uint8_t adjusted_brightness = get_display_brightness();
    if (adjusted_brightness == 1)
    {
        return 10; // minimum readable level in bright light
    }
    adjusted_brightness = (adjusted_brightness - 1) * 63;
    return adjusted_brightness;
}

void Gui::load_config_from_file(const JsonObject &json)
{
    time_24hour_format = json["24hourformat"].as<bool>();
    screen_timeout = json["screentimeout"].as<int>();
    timezone_id = json["timezone"].as<int>();
    display_brightness = json["brightness"].as<int>();
    twatchsk::dark_theme_enabled = json["darktheme"].as<bool>();
    char watchname[16] = "";
    if (strcmp(watchname, json["watchname"].as<String>().c_str()) != 0) // saved name is not blank
    {
        strcpy(watch_name, json["watchname"].as<String>().c_str());
    }
    else
    {
        strcpy(watch_name, "Set watch name");
    }

    ESP_LOGI(GUI_TAG, "Loaded settings: 24hour=%d, ScreenTimeout=%d, TimezoneID=%d, Brightness=%d, DarkTheme=%d, WatchName=%s",
             time_24hour_format, screen_timeout, timezone_id, display_brightness, twatchsk::dark_theme_enabled, watch_name);
}

void Gui::save_config_to_file(JsonObject &json)
{
    json["24hourformat"] = time_24hour_format;
    json["screentimeout"] = screen_timeout;
    json["timezone"] = timezone_id;
    json["brightness"] = display_brightness;
    json["darktheme"] = twatchsk::dark_theme_enabled;
    json["watchname"] = watch_name;
}

void Gui::theme_changed()
{
    twatchsk::update_imgbtn_color(menuBtn); // make the four little squares be the correct color for the theme
    bar->theme_changed();
}

void Gui::set_temporary_screen_timeout(int value)
{
    if (!screen_timeout_is_temporary)
    {
        saved_screen_timeout = screen_timeout;
        screen_timeout = value;
        screen_timeout_is_temporary = true;
    }
}

void Gui::clear_temporary_screen_timeout()
{
    if (screen_timeout_is_temporary)
    {
        screen_timeout = saved_screen_timeout;
        screen_timeout_is_temporary = false;
    }
}

void Gui::lv_mainbar_callback(lv_obj_t *obj, lv_event_t event)
{
    if (event == LV_EVENT_VALUE_CHANGED)
    {
        Gui *gui = (Gui *)obj->user_data;
        lv_coord_t x, y;
        lv_tileview_get_tile_act(obj, &x, &y);
        ESP_LOGI(GUI_TAG, "Tile view is showing location %d,%d", x, y);
        gui->set_is_active_view_dynamic(x > 0);
        gui->update_arrows_visibility();
    }
}

void Gui::set_is_active_view_dynamic(bool new_value)
{
    if (is_active_view_dynamic_ != new_value)
    {
        // ESP_LOGI(GUI_TAG, "Active view is dynamic=%d", new_value);
        is_active_view_dynamic_ = new_value;
        if (new_value)
        {
            ws_socket->update_subscriptions();
        }
    }
}

String Gui::current_time()
{
    time_t now;
    struct tm info;
    char buf[64];
    time(&now);
    localtime_r(&now, &info);
    if (time_24hour_format)
    {
        strftime(buf, sizeof(buf), "%H:%M", &info);
    }
    else
    {
        strftime(buf, sizeof(buf), "%I:%M", &info);
        if (info.tm_hour > 12)
        {
            strcat(buf, " pm");
        }
        else
        {
            strcat(buf, " am");
        }
    }
    String current_time_string = buf;
    return current_time_string;
}

void Gui::msg_box_callback(lv_obj_t *obj, lv_event_t event)
{
    if (event == LV_EVENT_VALUE_CHANGED)
    {
        Gui *gui = (Gui *)obj->user_data;
        if (lv_msgbox_get_active_btn(obj) == 1) // 0 is the OK button, 1 is the Disable Wifi button
        {
            gui->wifiManager->off();
        }
        gui->set_display_next_pending_message(true); // now that this message is being closed, it's OK to display the next one
        gui->display_next_message(true);             // true means "delete the first message before displaying the next one"
        lv_msgbox_start_auto_close(obj, 0);
        gui->update_pending_messages();
    }
}

void Gui::display_next_message(bool delete_first_message)
{
    if (pending_messages_.size() > 0)
    {
        std::list<PendingMsg_t>::iterator it = pending_messages_.begin();
        if (delete_first_message) // done only when a message was being displayed and has now been cleared from the screen by the user
        {
            ESP_LOGI(GUI_TAG, "Erasing the first message: %s", it->msg_text.c_str());
            pending_messages_.erase(it);
        }
        if (pending_messages_.size() == 0) // it == pending_messages_.end() didn't work for some reason
        {
            ESP_LOGI(GUI_TAG, "Last message has been deleted");
        }
        else // there is at least one message in pending_messages, so display it
        {
            it = pending_messages_.begin(); // doesn't work if "it" is not re-set to the first element like this
            ESP_LOGI(GUI_TAG, "Displaying the first message in pending_messages: %s", it->msg_text.c_str());
            msgBox = lv_msgbox_create(lv_scr_act(), NULL);
            String full_text =
                it->msg_time + "\n(" + it->msg_count + "x) " + it->msg_text + "\n\n(" + (String)(pending_messages_.size() - 1) + LOC_UNREAD_MSGS + ")";
            lv_msgbox_set_text(msgBox, full_text.c_str());
            if (it->msg_topic == Wifi_Problem)
            {
                static const char *btns[] = {LOC_MESSAGEBOX_OK, LOC_MESSAGEBOX_DISABLE_WIFI, ""};
                lv_msgbox_add_btns(msgBox, btns);                                  // Jan: why does this have to be inside the if and the else? If I put it AFTER the else, compiler says btns is undefined.
                lv_btnmatrix_set_btn_width(lv_msgbox_get_btnmatrix(msgBox), 1, 2); // make "Disable Wifi" button (button 1) twice as wide as "OK" button (button 0)
                lv_obj_set_style_local_radius(msgBox, LV_MSGBOX_PART_BTN, LV_STATE_DEFAULT, 10);
            }
            else
            {
                static const char *btns[] = {LOC_MESSAGEBOX_OK, ""};
                lv_msgbox_add_btns(msgBox, btns);
                lv_obj_set_style_local_radius(msgBox, LV_MSGBOX_PART_BTN, LV_STATE_DEFAULT, 10);
            }
            lv_obj_set_size(msgBox, 220, 260);
            lv_obj_align(msgBox, NULL, LV_ALIGN_CENTER, 0, 0);
            msgBox->user_data = this;
            lv_obj_set_event_cb(msgBox, msg_box_callback);
            display_next_pending_message_ = false; // so that only one is displayed at a time
            // trigger activity on main screen to avoid watch going to sleep right away, to ensure the message can be seen and read
            lv_disp_trig_activity(NULL);
        }
    }
    else
    {
        // hide pending messages icon
        ESP_LOGI(GUI_TAG, "No more pending messages to display");
    }

    update_pending_messages();
}

void Gui::update_pending_messages()
{
    if (pending_messages_.size() > 0)
    {
        char buff[10];
        sprintf(buff, "%s %d", LV_SYMBOL_BELL, pending_messages_.size());
        pendingMessagesIcon_->set_text(buff);
        pendingMessagesIcon_->set_status(StatusBarIconStatus::Warning);
    }
    else
    {
        pendingMessagesIcon_->set_status(StatusBarIconStatus::Hidden);
    }
}

void Gui::show_settings()
{
    hide_main_bar(true);
    auto gui = this;
    NavigationView *setupMenu = NULL;

    setupMenu = new NavigationView(LOC_SETTINGS_MENU, [setupMenu, gui]()
                                   {
                                           setupMenu->remove_from_active_list(); // because, for some reason, `delete setupMenu;` doesn't remove it from View::active_views_
                                           delete setupMenu;
                                           gui->hide_main_bar(false);
                                           if (gui->get_gui_needs_saved())
                                           {
                                               gui->save();
                                           }
                                           gui->set_gui_needs_saved(false); });

    setupMenu->add_tile(LOC_CLOCK_SETTINGS_MENU, &time_48px, false, [gui]()
                        {
                                auto timeSetting = new TimeSettings(TTGOClass::getWatch(), gui->get_sk_socket());
                                timeSetting->set_24hour_format(gui->get_time_24hour_format());
                                timeSetting->set_timezone_id(gui->get_timezone_id());
                                timeSetting->on_close([timeSetting, gui]()
                                                      {
                                                          bool need_to_save_gui = false;
                                                          if (gui->get_time_24hour_format() != timeSetting->get_24hour_format())
                                                          {
                                                              gui->set_time_24hour_format(timeSetting->get_24hour_format());
                                                              need_to_save_gui = true;
                                                          }
                                                          if (gui->get_timezone_id() != timeSetting->get_timezone_id())
                                                          {
                                                              gui->set_timezone_id(timeSetting->get_timezone_id());
                                                              need_to_save_gui = true;
                                                          }

                                                          if (need_to_save_gui)
                                                          {
                                                              gui->save();
                                                          }

                                                          delete timeSetting;
                                                      });
                                timeSetting->show(lv_scr_act()); });

    setupMenu->add_tile(LOC_DISPLAY_SETTINGS_MENU, &display_48px, false, [gui, setupMenu]()
                        {
                                auto displaySettings = new DisplaySettings(TTGOClass::getWatch(), gui->get_sk_socket());

                                // screen_timeout is saved to disk through GUI::screen_timeout. Retrieve it here:
                                displaySettings->set_screen_timeout(gui->get_screen_timeout());

                                // display_setting is saved to disk through GUI::display_brightness. Retrieve it here:
                                displaySettings->set_display_brightness(gui->get_display_brightness());

                                // Save the value of dark_theme_enabled before going into Display tile
                                bool current_dark_theme_enabled = twatchsk::dark_theme_enabled;

                                // Define the callback function (on_close()). If the value of any setting
                                // changed while the Display tile was up, save it.
                                displaySettings->on_close([displaySettings, gui, current_dark_theme_enabled, setupMenu]()
                                                          {
                                                              bool need_to_save = false;
                                                              int new_timeout = displaySettings->get_screen_timeout();
                                                              if (gui->get_screen_timeout() != new_timeout &&
                                                                  new_timeout >= 5)
                                                              {
                                                                  gui->set_screen_timeout(new_timeout);
                                                                  need_to_save = true;
                                                              }
                                                              uint8_t new_brightness = displaySettings->get_display_brightness();
                                                              if (gui->get_display_brightness() != new_brightness &&
                                                                  new_brightness > 0)
                                                              {
                                                                  gui->set_display_brightness(new_brightness);
                                                                  need_to_save = true;
                                                              }
                                                              if (twatchsk::dark_theme_enabled != current_dark_theme_enabled) // dark_theme flag changed while in Display tile
                                                              {
                                                                  need_to_save = true;
                                                                  ESP_LOGI(GUI_TAG, "Dark theme changed to %d", twatchsk::dark_theme_enabled);
                                                                  //update themes on GUI objects
                                                                  setupMenu->theme_changed();
                                                                  gui->theme_changed();
                                                              }
                                                              if (need_to_save)
                                                              {
                                                                  gui->save();
                                                              }
                                                              delete displaySettings;
                                                          });

                                // show() does a few things, then calls show_internal(), which defines
                                // the way this tile looks and acts.
                                displaySettings->show(lv_scr_act()); });

    setupMenu->add_tile(LOC_WIFI_SETTINGS_MENU, &wifi_48px, false, [gui]()
                        {
                                auto wifiSettings = new WifiSettings(gui->get_wifi_manager());
                                wifiSettings->on_close([wifiSettings]()
                                                       { delete wifiSettings; });
                                wifiSettings->show(lv_scr_act()); });

    setupMenu->add_tile(LOC_SIGNALK_SETTING_MENU, &signalk_48px, true, [gui]()
                        {
                                auto skSettings = new SignalKSettings(gui->get_sk_socket());
                                skSettings->on_close([skSettings]()
                                                     { delete skSettings; });
                                skSettings->show(lv_scr_act()); });

    setupMenu->add_tile(LOC_WAKEUP_SETTINGS_MENU, &wakeup_48px, false, [gui]()
                        {
                                auto wakeupSettings = new WakeupSettings(gui, gui->get_hardware());
                                wakeupSettings->on_close([wakeupSettings]()
                                                         { delete wakeupSettings; });
                                wakeupSettings->show(lv_scr_act()); });

    setupMenu->add_tile(LOC_WATCH_INFO_MENU, &info_48px, false, [gui]()
                        {
                                auto watchInfo = new WatchInfo(gui);

                                // watch_name is saved to disk through GUI::watch_name. Retrieve it here:
                                watchInfo->set_watch_name(gui->get_watch_name());

                                // Define the callback function (on_close()). If the watch name
                                // changed while the Watch Info tile was up, save it.
                                watchInfo->on_close([watchInfo, gui]()
                                                    {
                                                        char *new_watch_name = watchInfo->get_watch_name();
                                                        if (strcmp(new_watch_name, "") != 0 // new_watch_name isn't blank
                                                            &&
                                                            strcmp(gui->get_watch_name(), new_watch_name) != 0) // and it has changed
                                                        {
                                                            gui->set_watch_name(new_watch_name);
                                                            gui->save();
                                                        }
                                                        delete watchInfo;
                                                    });

                                // show() does a few things, then calls show_internal(), which defines
                                // the way this tile looks and acts.
                                watchInfo->show(lv_scr_act()); });

    setupMenu->show(lv_scr_act());
}

void Gui::show_home()
{    
    lv_tileview_set_tile_act(dynamic_gui->get_tile_view(), 0, 0, LV_ANIM_ON);
}

void Gui::toggle_wifi()
{
    auto wifiStatus = wifiManager->get_status();

    if(wifiStatus == WifiState_t::Wifi_Connected)
    {
        wifiManager->off(true);
    }
    else if(wifiStatus == WifiState_t::Wifi_Off)
    {
        wifiManager->on();
    }
}

void Gui::update_arrows_visibility()
{
    lv_coord_t x,y;
    lv_tileview_get_tile_act(mainBar, &x, &y);
    update_arrows_visibility(x > 0, x < (tile_valid_points_count-1));
}

void Gui::update_arrows_visibility(bool left, bool right)
{
    ESP_LOGI(GUI_TAG, "Arrows update left=%d, right=%d", left, right);

    lv_obj_set_hidden(arrow_left, !left);
    lv_obj_set_hidden(arrow_right, !right);
    arrows_hide_at_time = millis() + ARROWS_DISAPPEAR_TIME;
}

void Gui::hide_arrows_task_cb(lv_task_t*task)
{
    auto gui = (Gui*)task->user_data;
    lv_obj_set_hidden(gui->arrow_left, true);
    lv_obj_set_hidden(gui->arrow_right, true);
    lv_task_del(task);
    ESP_LOGI(GUI_TAG, "Arrows are hidden.");
}