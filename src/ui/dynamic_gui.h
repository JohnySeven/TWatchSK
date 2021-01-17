#pragma once
#include "dynamic_view.h"
#include "component_factory.h"
#include "dynamic_label.h"
#include "vector"


class DynamicGui
{
public:
    DynamicGui();
    void initialize_builders();
    bool load_file(String path, lv_obj_t*parent, int& count);
private:
    ComponentFactory *factory;
    std::vector<DynamicView*> views;
};