#pragma once
#include "config.h"

/*****************************************************************
 *
 *          ! MesBox Class
 *
 */

class MBox
{
public:
    MBox()
    {
        _mbox = nullptr;
    }
    ~MBox()
    {
        if (_mbox == nullptr)
            return;
        lv_obj_del(_mbox);
        _mbox = nullptr;
    }

    void create(const char *text, lv_event_cb_t event_cb, const char **btns = nullptr, lv_obj_t *par = nullptr)
    {
        if (_mbox != nullptr)
            return;
        lv_obj_t *p = par == nullptr ? lv_scr_act() : par;
        _mbox = lv_msgbox_create(p, NULL);
        lv_msgbox_set_text(_mbox, text);
        if (btns == nullptr)
        {
            static const char *defBtns[] = {"Ok", ""};
            lv_msgbox_add_btns(_mbox, defBtns);
        }
        else
        {
            lv_msgbox_add_btns(_mbox, btns);
        }
        lv_obj_set_width(_mbox, LV_HOR_RES - 40);
        lv_obj_set_event_cb(_mbox, event_cb);
        lv_obj_align(_mbox, NULL, LV_ALIGN_CENTER, 0, 0);
    }

    void setData(void *data)
    {
        lv_obj_set_user_data(_mbox, data);
    }

    void *getData()
    {
        return lv_obj_get_user_data(_mbox);
    }

    void setBtn(const char **btns)
    {
        lv_msgbox_add_btns(_mbox, btns);
    }

private:
    lv_obj_t *_mbox = nullptr;
};