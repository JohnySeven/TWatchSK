#pragma once
#include "component_factory.h"
#include "dynamic_helpers.h"

class DynamicLabelBuilder
{
public:
    static void initialize(ComponentFactory *factory);

private:
    DynamicLabelBuilder() {}
    static void initializeStyles();
};