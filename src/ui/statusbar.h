#pragma once
#include "config.h"
#include "system/observable.h"
#include "ui/themes.h"
#include <functional>
#include <vector>

enum StatusBarIconType
{
    Text,
    Image
};

enum StatusBarLocation
{
    Left,
    Right
};

enum StatusBarIconStatus
{
    Visible,
    Warning,
    Hidden
};

class StatusBarIcon
{
public:
    StatusBarIcon(lv_obj_t *parent, const char *text, StatusBarLocation location, std::function<void(void)> update_func, StatusBarIconStatus status)
    {
        type_ = StatusBarIconType::Text;
        obj_ = lv_label_create(parent, NULL);
        lv_label_set_text(obj_, text);
        update_func_ = update_func;
        location_ = location;
        set_status(status);
    }

    StatusBarIcon(lv_obj_t *parent, const void *image, StatusBarLocation location, std::function<void(void)> update_func, StatusBarIconStatus status)
    {
        type_ = StatusBarIconType::Image;
        obj_ = lv_img_create(parent, NULL);
        lv_img_set_src(obj_, image);
        twatchsk::update_imgbtn_color(obj_);
        update_func_ = update_func;
        location_ = location;
        set_status(status);
    }

    lv_obj_t *get_obj()
    {
        return obj_;
    }

    StatusBarLocation get_location()
    {
        return location_;
    }

    StatusBarIconStatus get_status()
    {
        return status_;
    }

    StatusBarIconType get_type()
    {
        return type_;
    }

    void set_text(const char *text)
    {
        if (type_ != StatusBarIconType::Image)
        {
            lv_label_set_text(obj_, text);
            update_func_();
        }
    }

    void set_status(StatusBarIconStatus status)
    {
        if (status_ != status)
        {
            bool call_update = (status == StatusBarIconStatus::Hidden || status_ == StatusBarIconStatus::Hidden);

            if (status == StatusBarIconStatus::Hidden)
            {
                lv_obj_set_hidden(obj_, true);
            }
            else if (status == StatusBarIconStatus::Warning)
            {
                lv_obj_set_hidden(obj_, false);
                set_color(LV_COLOR_RED);
            }
            else //normal
            {
                lv_obj_set_hidden(obj_, false);
                set_color(twatchsk::get_text_color());
            }

            status_ = status;

            if (call_update) //when status is changing from normal <--> warning we don't need update locations
            {
                update_func_();
            }
        }
    }

    void set_color(lv_color_t color)
    {
        if (type_ == StatusBarIconType::Image)
        {
            lv_obj_set_style_local_image_recolor(obj_, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, color);
        }
        else
        {
            lv_obj_set_style_local_text_color(obj_, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, color);
        }
    }

private:
    lv_obj_t *obj_ = NULL;
    StatusBarIconType type_;
    std::function<void(void)> update_func_;
    StatusBarLocation location_;
    StatusBarIconStatus status_;
};

class StatusBar
{
public:
    StatusBar()
    {
    }

    StatusBarIcon *create_text_icon(char *initial, StatusBarLocation location, StatusBarIconStatus status = StatusBarIconStatus::Visible)
    {
        auto ret = new StatusBarIcon(_par, initial, location, std::bind(&StatusBar::refresh, this), status);
        icons_.push_back(ret);

        return ret;
    }

    StatusBarIcon *create_image_icon(const void *image_bytes, StatusBarLocation location, StatusBarIconStatus status = StatusBarIconStatus::Visible)
    {
        auto ret = new StatusBarIcon(_par, image_bytes, location, std::bind(&StatusBar::refresh, this), status);
        icons_.push_back(ret);
        return ret;
    }

    void setup(lv_obj_t *par)
    {
        _par = par;

        _bar = lv_cont_create(_par, NULL);
        lv_obj_set_size(_bar, LV_HOR_RES, _barHeight);
        lv_obj_set_style_local_radius(_bar, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
        lv_obj_set_style_local_border_width(_bar, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 2);
        lv_obj_set_style_local_border_side(_bar, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_BORDER_SIDE_BOTTOM);
        
        refresh();
        theme_changed();
    }

    void theme_changed()
    {
        for (int i = 0; i < icons_.size(); i++)
        {
            auto icon = icons_.at(i);
            if (icon->get_type() == StatusBarIconType::Image)
            {
                twatchsk::update_imgbtn_color(icon->get_obj());
            }
            else if(icon->get_type() == StatusBarIconType::Text && icon->get_status() == StatusBarIconStatus::Visible)
            {
                icon->set_color(twatchsk::get_text_color());
            }
        }
    }

    uint8_t height()
    {
        return _barHeight;
    }

    lv_obj_t *self()
    {
        return _bar;
    }

    void set_hidden(bool hidden)
    {
        lv_obj_set_hidden(_bar, hidden);
    }

    void dont_refresh()
    {
        refresh_hold_ = true;
    }

    void do_refresh()
    {
        refresh_hold_ = false;
        refresh();
    }

private:
    void refresh()
    {
        if (!refresh_hold_)
        {
            lv_obj_t *lastLeft = NULL;
            lv_obj_t *lastRight = NULL;
            for (int i = 0; i < icons_.size(); i++)
            {
                auto icon = icons_.at(i);
                if (icon->get_status() != StatusBarIconStatus::Hidden)
                {
                    if (icon->get_location() == StatusBarLocation::Left)
                    {
                        if (lastLeft == NULL)
                        {
                            lv_obj_align(icon->get_obj(), _bar, LV_ALIGN_IN_LEFT_MID, iconOffset, 0);
                        }
                        else
                        {
                            lv_obj_align(icon->get_obj(), lastLeft, LV_ALIGN_OUT_RIGHT_MID, iconOffset, 0);
                        }

                        lastLeft = icon->get_obj();
                    }
                    else
                    {
                        if (lastRight == NULL)
                        {
                            lv_obj_align(icon->get_obj(), _bar, LV_ALIGN_IN_RIGHT_MID, -iconOffset, 0);
                        }
                        else
                        {
                            lv_obj_align(icon->get_obj(), lastRight, LV_ALIGN_OUT_LEFT_MID, -iconOffset, 0);
                        }

                        lastRight = icon->get_obj();
                    }
                }
            }
        }
    };
    lv_obj_t *_bar = nullptr;
    lv_obj_t *_par = nullptr;
    uint8_t _barHeight = 30;
    std::vector<StatusBarIcon *> icons_;

    const int8_t iconOffset = 5;
    bool refresh_hold_ = false;
};
