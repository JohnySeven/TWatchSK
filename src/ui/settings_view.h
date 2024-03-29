#pragma once
#include "view.h"
#include "functional"
#define SETTINGS_TOP_BAR_HEIGHT 42
LV_IMG_DECLARE(exit_32px);
const char *SETTINGS_TAG = "SETTINGS";

class SettingsView : public View
{
public:
    SettingsView(char *title)
    {
        this->titleText = title;
    }

    void on_close(std::function<void()> closeCallback)
    {
        this->callback = closeCallback;
    }

    void show(lv_obj_t *parent) override
    {
        static lv_style_t plStyle;
        lv_style_init(&plStyle);
        lv_style_set_radius(&plStyle, LV_OBJ_PART_MAIN, 0);
        lv_style_set_border_width(&plStyle, LV_OBJ_PART_MAIN, 0);
        container = lv_cont_create(parent, NULL);
        lv_obj_set_size(container, LV_HOR_RES, LV_VER_RES - SETTINGS_TOP_BAR_HEIGHT);
        lv_obj_set_pos(container, 0, SETTINGS_TOP_BAR_HEIGHT);
        lv_obj_add_style(container, LV_OBJ_PART_MAIN, &plStyle);
        lv_cont_set_layout(container, LV_LAYOUT_ROW_TOP);

        //define style of top bar - do not change colors - these are theme defined!
        static lv_style_t barStyle;
        lv_style_copy(&barStyle, &plStyle);
        lv_style_set_border_width(&barStyle, LV_OBJ_PART_MAIN, 2);
        lv_style_set_border_side(&barStyle, LV_OBJ_PART_MAIN, LV_BORDER_SIDE_BOTTOM);

        topBar = lv_cont_create(parent, NULL);
        lv_obj_set_size(topBar, LV_HOR_RES, SETTINGS_TOP_BAR_HEIGHT);
        lv_obj_set_pos(topBar, 0, 0);
        lv_obj_add_style(topBar, LV_OBJ_PART_MAIN, &barStyle); //BS: barStyle is currently just a copy of plStyle

        back = lv_imgbtn_create(topBar, NULL);
        lv_imgbtn_set_src(back, LV_BTN_STATE_RELEASED, &exit_32px);
        lv_imgbtn_set_src(back, LV_BTN_STATE_PRESSED, &exit_32px);
        lv_imgbtn_set_src(back, LV_BTN_STATE_CHECKED_RELEASED, &exit_32px);
        lv_imgbtn_set_src(back, LV_BTN_STATE_CHECKED_PRESSED, &exit_32px);
        twatchsk::update_imgbtn_color(back);

        lv_obj_set_click(back, true);
        lv_obj_set_ext_click_area(back, 0, 20, 0, 20);
        lv_obj_align(back, topBar, LV_ALIGN_IN_LEFT_MID, 6, 0);
        back->user_data = this;
        lv_obj_set_event_cb(back, __backButtonCallBack);

        title = lv_label_create(topBar, NULL);
        lv_label_set_text(title, titleText);
        lv_obj_align(title, topBar, LV_ALIGN_IN_LEFT_MID, 50, 0);

        show_internal(container);
    }

    void hide() override
    {
        if (hide_internal())
        {
            cleanup();
            callback();
        }
    }
    void cleanup()
    {
        if (container != NULL)
        {
            lv_obj_del(container);
            lv_obj_del(topBar);
            container = topBar = back = title = NULL;
        }
    }
    virtual ~SettingsView()
    {
        cleanup();
    }

    virtual void theme_changed() // updates graphic elements for a theme change; override for every descendant of SettingsView with specific theme change needs
    {
        twatchsk::update_imgbtn_color(back); // make the "Back" button the correct color depending on the theme
    }

protected:
    virtual void show_internal(lv_obj_t *parent){};
    virtual bool hide_internal() { return true; }

    /**
     * shows message box with message and if close_delay != 0 it will auto dismiss the message box in miliseconds.
     */
    void show_message(const char *message, int close_delay = 0)
    {
        static const char *btns[] = {LOC_MESSAGEBOX_OK, ""};
        lv_obj_t *mbox = lv_msgbox_create(lv_scr_act(), NULL);
        lv_msgbox_set_text(mbox, message);
        lv_msgbox_add_btns(mbox, btns);
        if(close_delay > 0)
        {
            lv_msgbox_start_auto_close(mbox, close_delay);
        }
        lv_obj_set_width(mbox, 200);
        lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
        //trigger activity on main screen to avoid message is displayed and device goes into sleep
        lv_disp_trig_activity(NULL);
    }
    lv_obj_t *container;
    lv_obj_t *topBar;
    lv_obj_t *back;
    const int spacing = 6; //defines default spacing between elements in layout, to make all spacing same on every page
private:
    lv_obj_t *title;
    char *titleText;
    std::function<void()> callback;

    static void __backButtonCallBack(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_CLICKED)
        {
            auto view = (SettingsView *)obj->user_data;
            ESP_LOGI("SETTINGS", "Closing view %s", view->titleText);
            view->hide();
        }
    }
};