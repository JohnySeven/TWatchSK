#pragma once
#include <forward_list>
#include <map>
#include <functional>
#include "Arduino.h"

class ObservableObject
{
    public:
        ObservableObject(String name);
        void subscribe(std::function<void(ObservableObject*,String&, void*)> func)
        {
            notifyList.push_front(func);
        }

        static ObservableObject*get_object(String name);
        String get_name() { return object_name; }
    protected:
        void notify(String name, void*value)
        {
            invoker(this, name, value);
        }
    private:
        String object_name;
        std::forward_list<std::function<void(ObservableObject*,String&, void*)>> notifyList;
};
