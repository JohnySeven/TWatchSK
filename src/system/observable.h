#pragma once
#include <functional>
#include <forward_list>
#include "system/observer.h"

template <class T>
class Observable
{
    public:
        Observable(T initialValue)
        {
             value = initialValue; 
             ESP_LOGI("OBSERVABLE", "Init %d", (int)value);
        }
        T get();
        void attach(Observer<T>* observer)
        {
            observers.push_front(observer);
            ESP_LOGI("OBSERVABLE", "Emit %d", (int)value);
            observer->notify_change(value);
        }
    protected:
        void emit(T value)
        {
            this->value = value;
            ESP_LOGI("OBSERVABLE", "Emit %d", (int)value);

            for(auto observer : observers)
            {
                observer->notify_change(value);
            }
        }
        T value;
    private:
        std::forward_list<Observer<T>*> observers;
};