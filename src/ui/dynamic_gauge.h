#pragma once
#include "component_factory.h"
#include "dynamic_helpers.h"
#include "component.h"
#include "data_adapter.h"

class DynamicGauge : public Component
{
    public:
        DynamicGauge(lv_obj_t*parent) : Component(parent)
        {

        };
        void load(const JsonObject &json) override;
        void update(const JsonVariant &update) override;
        void destroy() override;
    private:
        Data_formating_t formating;
};

class DynamicGaugeBuilder
{
public:
    static void initialize(ComponentFactory *factory);

private:
    DynamicGaugeBuilder() {}
};