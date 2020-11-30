#pragma once
#include "config.h"

class View
{
    virtual void show(lv_obj_t*parent) { }
    virtual void hide() { }
};