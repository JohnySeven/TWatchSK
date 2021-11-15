#include "dynamic_switch.h"
#include "localization.h"
#include "data_adapter.h"
#include "gui.h"
#include "system/events.h"
#include "localization.h"
static void dynamic_switch_callback(lv_obj_t *obj, lv_event_t event)
{
    if (event == LV_EVENT_VALUE_CHANGED)
    {
        DynamicSwitch *swtch = (DynamicSwitch *)obj->user_data;

        if (!swtch->IsChangeHandlerLocked())
        {
            auto value = lv_switch_get_state(obj);

            if(!swtch->send_put_request(value))
            {
                post_gui_warning(LOC_SK_PUT_SEND_FAIL);
            }
        }
    }
}

void DynamicSwitchBuilder::initialize(ComponentFactory *factory)
{
    factory->register_constructor("switch", [factory](JsonObject &json, lv_obj_t *parent) -> Component *
                                  {
                                      auto swtch = new DynamicSwitch(parent);
                                      swtch->load(json);
                                      return swtch;
                                  });
}

void DynamicSwitch::load(const JsonObject &json)
{
    lv_obj_t *ui_switch = lv_switch_create(parent_, NULL);

    this->obj_ = ui_switch;

    lv_obj_set_user_data(ui_switch, this);
    lv_obj_set_event_cb(ui_switch, dynamic_switch_callback);

    if (json.containsKey("binding"))
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
    }

    DynamicHelpers::set_location(ui_switch, json);
    DynamicHelpers::set_size(ui_switch, json);
    DynamicHelpers::set_layout(ui_switch, parent_, json);
}

void DynamicSwitch::update(const JsonVariant &json)
{
    if (json.is<bool>())
    {
        change_handler_locked_ = true;
        auto value = json.as<bool>();
        if (value)
        {
            lv_switch_on(obj_, LV_ANIM_ON);
        }
        else
        {
            lv_switch_off(obj_, LV_ANIM_ON);
        }
        change_handler_locked_ = false;
    }
}

bool DynamicSwitch::send_put_request(bool value)
{
    return adapter_->put_request(value);
}

void DynamicSwitch::destroy()
{
    if (obj_ != NULL)
    {
        lv_obj_del(obj_);
        obj_ = NULL;
    }
}