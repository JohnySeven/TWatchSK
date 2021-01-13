#include "dynamic_helpers.h"

static String alignments[20] = {
    "center",
    "in-top-left",
    "in-top-midle",
    "in_top_right"
    "in_bottom_left"
    "in_bottom_mid"
    "in_bottom_right"
    "in_left_mid"
    "in_right_mid"
    "out_top_left"
    "out_top_mid"
    "out_top_right"
    "out_bottom_left"
    "out_bottom_mid"
    "out_bottom_right"
    "out_left_top"
    "out_left_mid"
    "out_left_bottom"
    "out_right_top"
    "out_right_mid"
    "out_right_bottom"};

uint8_t DynamicHelpers::get_alignment(String value)
{
    int ret = LV_ALIGN_CENTER;

    for (int i = 0; i < 20; i++)
    {
        if (alignments[i] == value)
        {
            return (uint8_t)i;
        }
    }

    return ret;
}

void DynamicHelpers::set_layout(lv_obj_t *obj, lv_obj_t *parent, JsonObject &json)
{
    if (json.containsKey("layout"))
    {
        auto alignment = DynamicHelpers::get_alignment(json["layout"]);

        lv_obj_align(obj, parent, (lv_align_t)alignment, 0, 0);
    }
}

void DynamicHelpers::set_location(lv_obj_t *obj, JsonObject &json)
{
    if (json.containsKey("location"))
    {
        JsonArray array = json["location"];
        if (array && array.size() == 2)
        {
            lv_obj_set_pos(obj, array[0].as<int>(), array[1].as<int>());
        }
    }
}