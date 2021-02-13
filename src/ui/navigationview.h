#include "settings_view.h"
#include <functional>
#include <vector>

LV_FONT_DECLARE(lv_font_montserrat_28);

struct LinkData_t
{
    const lv_img_dsc_t *img;
    bool color_img;
    char *title;
    std::function<void(void)> callback;
};

class NavigationView : public SettingsView
{
public:
    NavigationView(char *title, std::function<void()> on_close) : SettingsView(title)
    {
        this->on_close(on_close);
    }

    void add_tile(char *title, const lv_img_dsc_t *img, bool img_is_color, std::function<void(void)> callback)
    {
        LinkData_t newTile;
        newTile.img = img;
        newTile.color_img = img_is_color;
        newTile.callback = callback;
        newTile.title = title;
        tiles.push_back(newTile);
        ESP_LOGI("SETTINGS", "Added tiles %d", tiles.size());
    }

    void show_internal(lv_obj_t *parent) override
    {
        lv_cont_set_layout(parent, LV_LAYOUT_OFF);
        list = lv_list_create(parent, NULL);
        lv_obj_set_size(list, LV_HOR_RES, lv_obj_get_height(parent));
        static lv_style_t tileStyle;
        lv_style_init(&tileStyle);
        lv_style_set_border_width(&tileStyle, LV_OBJ_PART_MAIN, 0);
        lv_style_set_radius(&tileStyle, LV_OBJ_PART_MAIN, 0);
        lv_style_set_text_font(&tileStyle, LV_STATE_DEFAULT, &lv_font_montserrat_28);
        lv_obj_add_style(list, LV_OBJ_PART_MAIN, &tileStyle);
        lv_page_set_edge_flash(list, true);
        for (auto it = tiles.begin(); it != tiles.end(); it++)
        {
            auto btn = lv_list_add_btn(list, it.base()->img, it.base()->title);
            auto img = lv_list_get_btn_img(btn);
            if (it.base()->color_img == false) // if the tile's icon is a monochrome image
            {
                twatchsk::update_imgbtn_color(img); // make it the correct color depending on LIGHT/DARK setting
            }
            btn->user_data = it.base();
            lv_obj_set_event_cb(btn, NavigationView::tile_event);
        }

        ESP_LOGI("SETTINGS", "Tiles loaded!");
    }
    void theme_updated()
    {
        twatchsk::update_imgbtn_color(back); // make it the correct color depending on LIGHT/DARK setting
        auto count = lv_list_get_size(list);
        lv_obj_t *btn = NULL;
        for (int i = 0; i < count; i++)
        {
            auto tile = tiles.at(i);
            if (!tile.color_img)
            {
                btn = lv_list_get_next_btn(list, btn);
                if (btn != NULL)
                {
                    auto img = lv_list_get_btn_img(btn);
                    twatchsk::update_imgbtn_color(img);
                }
            }
        }
    }

    bool hide_internal() override
    {
        return true;
    }

private:
    lv_obj_t *list;
    lv_style_t buttonStyle;
    lv_style_t pageStyle;
    std::vector<LinkData_t> tiles;
    const int tile_width = 48;
    const int tile_height = 48;
    const int tile_spacing = 6;

    static void tile_event(lv_obj_t *btn, lv_event_t event)
    {
        if (event == LV_EVENT_CLICKED)
        {
            auto tile = (LinkData_t *)btn->user_data;
            ESP_LOGI("GUI", "Tile %s touched!", tile->title);
            tile->callback();
        }
    }
};