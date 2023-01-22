#pragma once

#include "renderer-handler-window.h"
#include "imgui-renderer.h"
#include <functional>

namespace gl::imgui {

    class ui_window: public gl::renderer_handler_window {
    public:
        using gl::renderer_handler_window::renderer_handler_window;

        void window_draw() override final {}

        virtual void draw_ui() {};
        virtual void draw_main() {};

        virtual void init() {};

        void window_setup() override final;

    private:
        gl::imgui::imgui_renderer ui_renderer;
    };

};
