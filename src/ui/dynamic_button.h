#pragma once
#include "component_factory.h"
#include "dynamic_helpers.h"
#include "component.h"
#include "data_adapter.h"

enum ButtonAction
{
    SKPut,
    Settings,
    ToggleWifi,
    HomeTile
};

class DynamicButton : public Component
{
    public:
        DynamicButton(lv_obj_t*parent) : Component(parent)
        {

        };
        void load(const JsonObject &json) override;
        void update(const JsonVariant &update) override;
        void destroy() override;
        bool send_put_request();
        void on_clicked();
        ButtonAction get_action();
    private:
        ButtonAction button_action_; 
        lv_obj_t * label_;
        String sk_put_json_;
        DataAdapter* adapter_;
};

class DynamicButtonBuilder
{
public:
    static void initialize(ComponentFactory *factory);

private:
    DynamicButtonBuilder() {}

};