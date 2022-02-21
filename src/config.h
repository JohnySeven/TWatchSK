#pragma once
// => Hardware select
#ifndef GIT_REV
#define GIT_REV "???"
#warning "GIT revision not defined!"
#endif
#define LV_USE_LOG  1
// => Function select
#define LILYGO_WATCH_LVGL                   //To use LVGL, you need to enable the macro LVGL
//#define TWATCH_USE_PSRAM_ALLOC_LVGL
#include <LilyGoWatch.h>

