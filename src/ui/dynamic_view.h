#include "view.h"
#include "ArduinoJson.h"
#include "component_factory.h"
#include "vector"

enum ViewType_t
{
    WatchFace,
    Normal
};

class DynamicView
{
    public:
        void Load(lv_obj_t*parent, JsonObject viewObject, ComponentFactory * factory)
        {
            container = lv_cont_create(parent, NULL);
            String viewType = viewObject["type"].as<String>();
            if(viewType == "watchface")
            {
                type = ViewType_t::WatchFace;
            }
            else if (viewType == "normal")
            {
                type = ViewType_t::Normal;
            }
            else
            {
                ESP_LOGW("UI", "Invalid view type of %s", viewType.c_str());
                type = ViewType_t::Normal;
            }

            JsonArray components = viewObject["components"].as<JsonArray>();

            for(JsonObject component : components)
            {
                auto obj = factory->createComponent(component, container);
                if(obj != NULL)
                {
                    created_components.push_back(obj);
                }
            }            
        }
    private:
        ViewType_t type;
        lv_obj_t*container;
        std::vector<lv_obj_t*> created_components;
};