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
        void destroy() override;
    private:
        Data_formating_t formating;
};

class DynamicLabelBuilder
{
public:
    static void initialize(ComponentFactory *factory);

private:
    DynamicLabelBuilder() {}
};