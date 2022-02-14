#include "dynamic_button.h"
#include "localization.h"
#include "data_adapter.h"
#include "gui.h"
#include "system/events.h"
#include "localization.h"

static void dynamic_button_callback(lv_obj_t *obj, lv_event_t event)
{
    if (event == LV_EVENT_CLICKED)
    {
        DynamicButton *button = (DynamicButton *)obj->user_data;

        auto value = lv_switch_get_state(obj);

        button->on_clicked();
    }
}

void DynamicButtonBuilder::initialize(ComponentFactory *factory)
{
    factory->register_constructor("button", [factory](JsonObject &json, lv_obj_t *parent) -> Component *
                                  {
                                      auto btn = new DynamicButton(parent);
                                      btn->load(json);
                                      return btn;
                                  });
}

void DynamicButton::load(const JsonObject &json)
{
    lv_obj_t *btn = lv_btn_create(parent_, NULL);

    this->obj_ = btn;

    lv_obj_set_user_data(btn, this);
    lv_obj_set_event_cb(btn, dynamic_button_callback);

    if(json.containsKey("push"))
    {
        JsonObject push = json["push"].as<JsonObject>();

        sk_put_path_ = push["path"].as<String>();
        if(push["value"].is<JsonObject>())
        {
            serializeJson(sk_put_value_, push["value"].as<JsonObject>());
        }
        else
        {
            serializeJson(sk_put_value_, push["value"].as<JsonVariant>());
        }
    }

    if(json.containsKey("color"))
    {
        auto color = DynamicHelpers::get_color(json["color"]);

        
    }

    /*if (json.containsKey("binding"))
    {
        JsonObject binding = json["binding"].as<JsonObject>();
        int period = 1000;

        if (binding.containsKey("period"))
        {
            period = binding["period"].as<int>();
        }

        path_ = binding["path"].as<String>();

        //register dataadapter that will connect SK receiver and this label
        adapter_ = new DataAdapter(path_, period, this);
    }*/

    DynamicHelpers::set_location(btn, json);
    DynamicHelpers::set_size(btn, json);
    DynamicHelpers::set_layout(btn, parent_, json);
}

bool DynamicButton::send_put_request(bool value)
{
    return false;
}

bool DynamicButton::on_clicked()
{

}

void DynamicButton::destroy()
{
    if (obj_ != NULL)
    {
        lv_obj_del(obj_);
        obj_ = NULL;
    }
}