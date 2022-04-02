#pragma once
#include "component_factory.h"
#include "dynamic_helpers.h"
#include "component.h"
#include "data_adapter.h"

class DynamicLabel : public Component
{
    public:
        DynamicLabel(lv_obj_t*parent) : Component(parent)
        {

        };
        void load(const JsonObject &json) override;
        void update(const JsonVariant &update) override;
        void on_offline() override;
        void destroy() override;
    private:
        Data_formating_t formating;
        bool has_binding_ = false;
};

class DynamicLabelBuilder
{
public:
    static void initialize(ComponentFactory *factory);

private:
    DynamicLabelBuilder() {}
};