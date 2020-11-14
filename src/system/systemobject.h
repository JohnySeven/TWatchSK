#pragma once
#include <forward_list>
#include <map>
#include <functional>
#include "Arduino.h"

class SystemObject
{
    public:
        SystemObject(String name);
        static SystemObject*get_object(String name);
        String get_name() { return object_name; }
    private:
        String object_name;
};
