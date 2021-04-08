#pragma once
#include "../config.h"
#include <vector>

class View
{
private:
    inline static std::vector<View*> active_views_;
    bool is_active_ = false;

public:
    View()
    {
        is_active_ = true;
        active_views_.push_back(this); // add each View descendant to a std::vector of active views
        ESP_LOGI("VIEW_CONSTRUCTOR", "Size of active_views_ is %d", active_views_.size());
    }
    
    ~View()
    {
        if (is_active_)
        {
            is_active_ = false;
            active_views_.pop_back();
            ESP_LOGI("VIEW_DESTRUCTOR", "Number of active_views_ is %d", active_views_.size());
        }
    }
    
    void remove_from_active_list()
    {
        active_views_.pop_back();
        ESP_LOGI("VIEW_POST_REMOVE", "Number of active_views_ is %d", active_views_.size());
    }

    static int get_active_views_count() { return active_views_.size(); }

    virtual void show(lv_obj_t*parent) { }
    virtual void hide()
    {
        if (is_active_)
        {
            is_active_ = false;
            active_views_.pop_back();
            ESP_LOGI("VIEW_HIDE", "Number of active_views_ is %d", active_views_.size());
        }
    }
    virtual void theme_changed() { }
    static void invoke_theme_changed()
    {
        for (std::vector<View*>::iterator it = active_views_.begin(); it != active_views_.end(); it++)
        {
            (*it)->theme_changed();
        }
    }

};