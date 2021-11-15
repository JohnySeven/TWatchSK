#pragma once
#include "dynamic_view.h"
#include "component_factory.h"
#include "vector"
#include "networking/signalk_socket.h"


class DynamicGui
{
public:
    DynamicGui();
    void initialize();
    bool load_file(String path, lv_obj_t*parent, SignalKSocket*socket, int& count);
    void handle_signalk_update(const String& path, const JsonVariant&value);
private:
    ComponentFactory *factory;
    std::vector<DynamicView*> views;
};