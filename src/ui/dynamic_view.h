#include "view.h"
#include "ArduinoJson.h"
#include "component_factory.h"
#include "vector"

enum ViewType_t
{
    WatchFace,
    NormalView
};

class DynamicView
{
public:
    lv_obj_t *get_obj() { return container; }
    void Load(lv_obj_t *parent, JsonObject viewObject, ComponentFactory *factory)
    {
        //! main
        static lv_style_t mainStyle;
        lv_style_init(&mainStyle);
        lv_style_set_radius(&mainStyle, LV_OBJ_PART_MAIN, 0);
        lv_style_set_bg_color(&mainStyle, LV_OBJ_PART_MAIN, LV_COLOR_GRAY);
        lv_style_set_bg_opa(&mainStyle, LV_OBJ_PART_MAIN, LV_OPA_0);
        lv_style_set_border_width(&mainStyle, LV_OBJ_PART_MAIN, 0);
        lv_style_set_text_color(&mainStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);
        lv_style_set_image_recolor(&mainStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);

        container = lv_cont_create(parent, NULL);
        lv_obj_add_style(container, LV_CONT_PART_MAIN, &mainStyle);
        lv_obj_set_size(container, LV_HOR_RES - 30, LV_VER_RES);

        String viewType = viewObject["type"].as<String>();
        if (viewType == "watchface")
        {
            type = ViewType_t::WatchFace;
        }
        else if (viewType == "normal")
        {
            type = ViewType_t::NormalView;
        }
        else
        {
            ESP_LOGW("UI", "Invalid view type of %s", viewType.c_str());
            type = ViewType_t::NormalView;
        }

        JsonArray components = viewObject["components"].as<JsonArray>();

        for (JsonObject component : components)
        {
            auto obj = factory->createComponent(component, container);
            if (obj != NULL)
            {
                created_components.push_back(obj);
            }
        }
    }

private:
    ViewType_t type;
    lv_obj_t *container;
    std::vector<lv_obj_t *> created_components;
};