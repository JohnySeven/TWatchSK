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
        }
        T get();
        void attach(Observer<T>* observer)
        {
            observers.push_front(observer);
            observer->notify_change(value);
        }
    protected:
        void emit(T value)
        {
            this->value = value;
            int index = 0;
            for(auto observer : observers)
            {
                observer->notify_change(value);
                index++;
            }
        }
        T value;
    private:
        std::forward_list<Observer<T>*> observers;
};