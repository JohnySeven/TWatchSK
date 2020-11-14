#pragma once

template<class T>
class Observer
{
    public:
        virtual void notify_change(const T &value);
};