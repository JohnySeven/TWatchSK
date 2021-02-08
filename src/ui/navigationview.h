#include "settings_view.h"
#include <functional>
#include <vector>
struct LinkData_t
{
    const lv_img_dsc_t *img;
    char*title;
    std::function<void(void)> callback;
};

class NavigationView : public SettingsView
{
public:
    NavigationView(char *title, std::function<void()> on_close) : SettingsView(title)
    {
        lv_style_init(&buttonStyle);
        lv_style_set_border_width(&buttonStyle, LV_STATE_DEFAULT, 1);
        lv_style_set_margin_all(&buttonStyle, LV_STATE_DEFAULT, 2);

        lv_style_init(&pageStyle);
        lv_style_set_border_width(&pageStyle, LV_STATE_DEFAULT, 0);
        lv_style_set_margin_all(&pageStyle, LV_STATE_DEFAULT, 0);
        this->on_close(on_close);
    }

    void add_tile(char* title, const lv_img_dsc_t *img, std::function<void(void)> callback)
    {
        LinkData_t newTile;
        newTile.img = img;
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
        lv_page_set_edge_flash(list, true); 
        for (auto it = tiles.begin(); it != tiles.end();it++)
        {
            auto btn = lv_list_add_btn(list, it.base()->img, it.base()->title);
            btn->user_data = it.base();
            lv_obj_set_event_cb(btn, NavigationView::tile_event); 
        }

        ESP_LOGI("SETTINGS", "Tiles loaded!");
    }
    bool hide_internal() override
    {
        return true;
    }

private:
    lv_obj_t* list;
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
            auto tile = (LinkData_t*)btn->user_data;
            ESP_LOGI("GUI", "Tile %s touched!", tile->title);
            tile->callback();
        }
    }
};