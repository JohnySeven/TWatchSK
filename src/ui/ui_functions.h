#include "gui.h"

namespace twatchsk
{
    class UIFunctions
    {
    public:
        UIFunctions(Gui *gui)
        {
            gui_ = gui;
        }

        void show_home()
        {
            gui_->show_home();
        };
        void show_settings()
        {
            gui_->show_settings();
        }
        void toggle_wifi()
        {
            gui_->toggle_wifi();
        }


    private:
        Gui *gui_;
    };

    static UIFunctions * UI_Functions;
}