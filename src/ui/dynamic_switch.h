#pragma once
#include "component_factory.h"
#include "dynamic_helpers.h"
#include "component.h"
#include "data_adapter.h"

class DynamicSwitch : public Component
{
    public:
        DynamicSwitch(lv_obj_t*parent) : Component(parent)
        {

        };
        void load(const JsonObject &json) override;
        void update(const JsonVariant &update) override;
        void destroy() override;
        bool IsChangeHandlerLocked() { return change_handler_locked_; }
        bool send_put_request(bool value);
    private:
        String path_;
        bool change_handler_locked_ = false;
        DataAdapter* adapter_ = NULL;
};

class DynamicSwitchBuilder
{
public:
    static void initialize(ComponentFactory *factory);

private:
    DynamicSwitchBuilder() {}

};