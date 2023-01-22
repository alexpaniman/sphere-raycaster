#include "ui-window.h"

namespace gl::imgui {

    void ui_window::window_setup() {
        ui_renderer.set_main_renderer(std::bind(&ui_window::draw_main, this));
        ui_renderer.set_ui_renderer(  std::bind(&ui_window::draw_ui  , this));

        set_renderer(&ui_renderer);

        init();
    }

}
