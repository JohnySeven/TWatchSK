#pragma once
#include "config.h"

LV_IMG_DECLARE(wifi);
LV_IMG_DECLARE(setting);
LV_IMG_DECLARE(menu);

class MenuBar
{
public:
    typedef struct
    {
        const char *name;
        void *img;
        void (*event_cb)();
    } lv_menu_config_t;

    MenuBar()
    {
        _cont = nullptr;
        _view = nullptr;
        _exit = nullptr;
        _obj = nullptr;
        _vp = nullptr;
    };
    ~MenuBar(){};

    void createMenu(lv_menu_config_t *config, int count, lv_event_cb_t event_cb, int direction = 1)
    {
        static lv_style_t menuStyle;
        lv_style_init(&menuStyle);
        lv_style_set_radius(&menuStyle, LV_OBJ_PART_MAIN, 0);
        lv_style_set_bg_color(&menuStyle, LV_OBJ_PART_MAIN, LV_COLOR_GRAY);
        lv_style_set_bg_opa(&menuStyle, LV_OBJ_PART_MAIN, LV_OPA_0);
        lv_style_set_border_width(&menuStyle, LV_OBJ_PART_MAIN, 0);
        lv_style_set_text_color(&menuStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);
        lv_style_set_image_recolor(&menuStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);

        _count = count;

        _vp = new lv_point_t[count];

        _obj = new lv_obj_t *[count];

        for (int i = 0; i < count; i++)
        {
            if (direction)
            {
                _vp[i].x = 0;
                _vp[i].y = i;
            }
            else
            {
                _vp[i].x = i;
                _vp[i].y = 0;
            }
        }

        _cont = lv_cont_create(lv_scr_act(), NULL);
        lv_obj_set_size(_cont, LV_HOR_RES, LV_VER_RES - 30);
        lv_obj_align(_cont, NULL, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
        //lv_obj_add_style(_cont, LV_OBJ_PART_MAIN, &menuStyle);

        _view = lv_tileview_create(_cont, NULL);
        lv_tileview_set_valid_positions(_view, _vp, count);
        lv_tileview_set_edge_flash(_view, false);
        lv_obj_align(_view, NULL, LV_ALIGN_CENTER, 0, 0);
        lv_page_set_scrlbar_mode(_view, LV_SCRLBAR_MODE_OFF);
        //lv_obj_add_style(_view, LV_OBJ_PART_MAIN, &menuStyle);

        lv_coord_t _w = lv_obj_get_width(_view);
        lv_coord_t _h = lv_obj_get_height(_view);

        for (int i = 0; i < count; i++)
        {
            _obj[i] = lv_cont_create(_view, _view);
            lv_obj_set_size(_obj[i], _w, _h);

            lv_obj_t *img = lv_img_create(_obj[i], NULL);
            lv_img_set_src(img, config[i].img);
            lv_obj_align(img, _obj[i], LV_ALIGN_CENTER, 0, 0);

            lv_obj_t *label = lv_label_create(_obj[i], NULL);
            lv_label_set_text(label, config[i].name);
            lv_obj_align(label, img, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

            i == 0 ? lv_obj_align(_obj[i], NULL, LV_ALIGN_CENTER, 0, 0) : lv_obj_align(_obj[i], _obj[i - 1], direction ? LV_ALIGN_OUT_BOTTOM_MID : LV_ALIGN_OUT_RIGHT_MID, 0, 0);

            lv_tileview_add_element(_view, _obj[i]);
            lv_obj_set_click(_obj[i], true);
            lv_obj_set_event_cb(_obj[i], event_cb);
        }

        _exit = lv_imgbtn_create(lv_scr_act(), NULL);
        lv_imgbtn_set_src(_exit, LV_BTN_STATE_RELEASED, &menu);
        lv_imgbtn_set_src(_exit, LV_BTN_STATE_PRESSED, &menu);
        lv_imgbtn_set_src(_exit, LV_BTN_STATE_CHECKED_PRESSED, &menu);
        lv_imgbtn_set_src(_exit, LV_BTN_STATE_CHECKED_RELEASED, &menu);
        lv_obj_align(_exit, NULL, LV_ALIGN_IN_BOTTOM_RIGHT, -20, -20);
        lv_obj_set_event_cb(_exit, event_cb);
        lv_obj_set_top(_exit, true);
    }
    lv_obj_t *exitBtn() const
    {
        return _exit;
    }
    lv_obj_t *self() const
    {
        return _cont;
    }
    void hidden(bool en = true)
    {
        lv_obj_set_hidden(_cont, en);
        lv_obj_set_hidden(_exit, en);
    }
    lv_obj_t *obj(int index) const
    {
        if (index > _count)
            return nullptr;
        return _obj[index];
    }

private:
    lv_obj_t *_cont, *_view, *_exit, **_obj;
    lv_point_t *_vp;
    int _count = 0;
};