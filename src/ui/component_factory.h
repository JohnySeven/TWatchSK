#include "config.h"
#include "map"
#include "ArduinoJson.h"
#include "functional"

class ComponentFactory
{
    public:
        lv_obj_t*createComponent(JsonObject& componentJson, lv_obj_t*parent)
        {
            lv_obj_t*ret = NULL;
            String type = componentJson["type"].as<String>();

            auto it = componentConstructors.find(type);

            if(it != componentConstructors.end())
            {
                ret = it->second(componentJson, parent);
                if(componentJson.containsKey("layout"))
                {
                    layoutComponent(componentJson["layout"].as<String>(), parent, ret);
                }
            }
            else
            {
                ESP_LOGI("UI", "Unknown component type %s!", type.c_str());
            }

            return ret;
        }

        void layoutComponent(String layoutType, lv_obj_t* parent, lv_obj_t*obj)
        {
            auto layout = LV_LAYOUT_CENTER;

            lv_obj_align(obj, parent, layout, 0, 0);
        }

        void registerConstructor(String name, std::function<lv_obj_t*(JsonObject&,lv_obj_t*)> factoryFunc)
        {
            componentConstructors[name] = factoryFunc;
        }
    private:
        std::map<String, std::function<lv_obj_t*(JsonObject&,lv_obj_t*)>> componentConstructors;
};