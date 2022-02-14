#pragma once
#include "component_factory.h"
#include "dynamic_helpers.h"
#include "component.h"
#include "data_adapter.h"

enum ButtonOperation
{
    TapSKPut
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
        bool send_put_request(bool value);
        void on_clicked();
        ButtonOperation get_operation();
    private:
        ButtonOperation button_operation_; 
        String sk_put_path_;
        String sk_put_value_;
};

class DynamicButtonBuilder
{
public:
    static void initialize(ComponentFactory *factory);

private:
    DynamicButtonBuilder() {}

};