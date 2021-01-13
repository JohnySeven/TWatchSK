#pragma once
#include "dynamic_view.h"
#include "component_factory.h"
#include "vector"

class DynamicGui
{
public:
    DynamicGui()
    {
        factory = new ComponentFactory();
    }

    void initialize_builders()
    {
        
    }

    bool load_file(String path)
    {

    }
private:
    ComponentFactory *factory;
    std::vector<DynamicView*> views;
};