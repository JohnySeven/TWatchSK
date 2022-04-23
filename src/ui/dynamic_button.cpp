#include "dynamic_button.h"
#include "localization.h"
#include "data_adapter.h"
#include "gui.h"
#include "system/events.h"
#include "localization.h"
#include "ui_functions.h"

static void dynamic_button_callback(lv_obj_t *obj, lv_event_t event)
{
    if (event == LV_EVENT_CLICKED)
    {
        DynamicButton *button = (DynamicButton *)obj->user_data;
        button->on_clicked();
    }
}

void DynamicButtonBuilder::initialize(ComponentFactory *factory)
{
    factory->register_constructor("button", [factory](JsonObject &json, lv_obj_t *parent) -> Component *
                                  {
            auto btn = new DynamicButton(parent);
            btn->load(json);
            return btn; });
}

void DynamicButton::load(const JsonObject &json)
{
    lv_obj_t *btn = lv_btn_create(parent_, NULL);

    this->obj_ = btn;

    lv_obj_set_user_data(btn, this);
    lv_obj_set_event_cb(btn, dynamic_button_callback);

    this->label_ = lv_label_create(obj_, NULL);

    if (json.containsKey("put"))
    {
        auto push = json["put"].as<JsonObject>();      
        serializeJson(push, sk_put_json_);
        ESP_LOGI("BTN", "SK Put %s", sk_put_json_.c_str());
        button_action_ = ButtonAction::SKPut;
        adapter_ = new DataAdapter(this);
    }
    else if (json.containsKey("action"))
    {
        String action = json["action"].as<String>();

        if (action == "Settings")
        {
            button_action_ = ButtonAction::Settings;
        }
        else if (action == "ToggleWifi")
        {
            button_action_ = ButtonAction::ToggleWifi;
        }
        else if (action == "HomeTile")
        {
            button_action_ = ButtonAction::HomeTile;
        }
        else
        {
            log_w("Button action %s is invalid", action.c_str());
        }
    }

    if (json.containsKey("text"))
    {
        lv_label_set_text(label_, json["text"].as<char *>());
    }

    if (json.containsKey("font"))
    {
        auto fontName = json["font"].as<String>();
        DynamicHelpers::set_font(label_, fontName);
    }

    DynamicHelpers::set_location(btn, json);
    DynamicHelpers::set_size(btn, json);
    DynamicHelpers::set_layout(btn, parent_, json);
}

void DynamicButton::on_clicked()
{
    switch (button_action_)
    {
    case ButtonAction::HomeTile:
        twatchsk::UI_Functions->show_home();
        break;
    case ButtonAction::Settings:
        twatchsk::UI_Functions->show_settings();
        break;
    case ButtonAction::ToggleWifi:
        twatchsk::UI_Functions->toggle_wifi();
        break;
    case ButtonAction::SKPut:
        send_put_request();
        break;
    }
}

bool DynamicButton::send_put_request()
{
    return adapter_->put_request(sk_put_json_);
}

void DynamicButton::destroy()
{
    if (obj_ != NULL)
    {
        lv_obj_del(obj_);
        obj_ = NULL;
    }
}

void DynamicButton::update(const JsonVariant &update)
{

}