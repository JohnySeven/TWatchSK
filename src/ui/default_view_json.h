#include <pgmspace.h>
const char JSON_default_view[] PROGMEM = R"=====(
{
    "name": "Test view",
    "views": 
    [
        {
            "type" : "normal",
            "components" :
            [
                {
                    "type" : "label",
                    "layout" : "center",
                    "text" : "Hello world!",
                    "style" : "ubuntu50",
                    "color" : "blue"
                },
                {
                    "type" : "label",
                    "layout" : "in_bottom_mid",
                    "text" : "This is dynamic view!",
                    "style" : "ubuntu50",
                    "color" : "red"
                }
            ]
        }
    ]
})=====";