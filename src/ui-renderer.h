#pragma once

#include "axes.h"
#include "drawing-manager.h"
#include "opengl-setup.h"
#include "rect.h"
#include "renderer.h"
#include "simple-drawing-renderer.h"
#include "simple-window.h"
#include "vec.h"
#include <GLFW/glfw3.h>
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <limits>
#include <memory>
#include <mutex>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace axp {

    template <typename type>
    struct linear_range {
        type min, max; // TODO: make this a vec?

        template <typename other_type>
        linear_range<type>& operator*=(other_type value) {
            min *= value;
            max *= value;

            return *this;
        }

        void intersect(const linear_range<type>& other) {
            min = std::max(min, other.min);
            max = std::min(max, other.max);
        }
    };

    using screen_dimension_range = linear_range<std::size_t>;

    struct screen_space_range {
        screen_dimension_range width, height;
    };

    struct screen_space_size {
        size_t width, height;

        void expand(const screen_space_size& other) {
            width  = std::max(other.width ,  width);
            height = std::max(other.height, height);
        }
    };

    struct space_range {
        screen_space_range range;
        screen_space_size preffered;

        template <typename type>
        space_range& operator*=(type value) {
            range.height *= value;
            range.width  *= value;

            preffered.width *= value;
            preffered.height *= value;

            return *this;
        }

        void intersect(const space_range& other) {
            preffered.expand(other.preffered);

            range.width.intersect(other.range.width); // TODO: exract
            range.height.intersect(other.range.height);
        }
    };


    struct max_proxy {
        template <std::integral integral_type>
        operator       integral_type() const noexcept {
            return std::numeric_limits<integral_type>::max();
        }

        template <std::floating_point floating_point_type>
        operator floating_point_type() const noexcept {
            return std::numeric_limits<floating_point_type>::max();
        }
    };

    inline constexpr max_proxy max;

    struct min_proxy {
        template <std::integral integral_type>
        operator       integral_type() const noexcept {
            return std::numeric_limits<integral_type>::min();
        }

        template <std::floating_point floating_point_type>
        operator floating_point_type() const noexcept {
            return std::numeric_limits<floating_point_type>::min();
        }
    };

    inline constexpr min_proxy min;

}

enum class mouse_button { left, middle, right };

struct mouse_event {
    enum its_kind { click, hover, drag } kind;

    math::vec2 point; // Current cursor position

    struct click_event { mouse_button clicked; };
    struct  drag_event { math::vec2   delta;   };

    union {
        click_event access;
        drag_event movement;
    };

    mouse_event transform(math::axes axis) {
        mouse_event copy = *this;

        copy.point = axis.get_world_coordinates(point);

        // if (copy.kind == mouse_event::drag) // TODO: what if hover?
        //     copy.movement.delta =
        //         axis.get_view_coordinates(movement.delta);

        // TODO: transform one widget up!

        return copy;
    };
};

class widget {
public:
    /*
     * Implement look of a widget.
     *
     * Drawing happens in a relative coordinates with points
     * (-1, -1) and (1, 1) being top left corner and
     * right bottom corner respectively.
     *
     * Drawing out of bounds is not permitted, behaviour in
     * such cases is left to the underlying implementation.
     *
     * Drawing only specifies the look of a widget, so
     * issuing, for example draw_triangle call, may not have
     * immediate effect of displaying such on the screen for
     * the sake of optimisations, drawing other wigets, etc.
     */
    virtual void draw(gl::drawing_manager &mgr) = 0;

    /*
     * Get information about widget's space requirements.
     *
     * Some widgets represent finite bitmaps, which can't
     * ever change size, or they become blurry.
     *
     * Others are vector images and can undergo arbitrary
     * resizing without any loss in quality, and only 
     * constraint is perseverance of aspect ratio.
     *
     * Vector widgets may still want to be of a specific
     * size for usability reasons (e.g. buttons -- it's
     * not the best idea make them extra large, enough space
     * is enough).
     *
     * Some widgets don't care about either. E.g. canvas
     * can very well be of any size without any loss in 
     * quality or usability. Moreover, it's preffered for
     * it to be as big as possible.
     *
     * This function is a customization point, which
     * enables widgets to select desired behaviour for
     * themselfs. It's output may letter be used by
     * container widgets to for smarter widget positioning.
     */
    [[nodiscard]]
    virtual axp::space_range get_desired_space() const noexcept = 0;
    virtual bool resize(axp::screen_space_size new_size) = 0;

    virtual bool on_mouse_event(mouse_event event) = 0;

    virtual ~widget() = default;

    // Utilities for invoking children widgets in subspaces

    void draw(gl::drawing_manager &mgr, math::rectangle subspace) {
        gl::drawing_manager child_manager =
            mgr.with_applied(math::axes { subspace });

        draw(child_manager);
    }

    void on_mouse_event(mouse_event event, math::rectangle subspace) {
        on_mouse_event(event.transform(math::axes { subspace }));
    }
};

template <typename widget_type>
void dispatch_mouse_event(widget_type* widget_this, mouse_event event) {
    switch (event.kind) {
    case mouse_event::click:
        widget_this->on_click(event.point, event.access.clicked);
        break;

    case mouse_event::hover:
        widget_this->on_hover(event.point);
        break;

    case mouse_event::drag:
        widget_this->on_drag(event.point, event.movement.delta);
        break;

    default:
        assert(false && "Unhandled mouse event!");
    };
}

struct plain_widget: public widget {
    void on_mouse_event(mouse_event event) final {
        dispatch_mouse_event(this, event);
    }

    virtual void on_click([[maybe_unused]] math::vec2 point,
                          [[maybe_unused]]
                            mouse_button clicked) {}

    virtual void on_drag ([[maybe_unused]] math::vec2 point,
                          [[maybe_unused]] math::vec2 delta) {}

    virtual void on_hover([[maybe_unused]] math::vec2 point) {}
};

class widget_container {
public:
    widget_container(auto&&... widgets)
        : m_widgets { std::move(widgets)... } {}

protected:
    std::vector<std::unique_ptr<widget>> m_widgets;
};


class widget_wrapper {
public:
    widget_wrapper(std::unique_ptr<widget>&& widget)
        : m_widget(std::move(widget)) {}

protected:
    std::unique_ptr<widget> m_widget;
};



class equally_spaced_list: public widget, private widget_container {
public:
    using widget_container::widget_container;

    void draw(gl::drawing_manager &mgr) override {
        for (std::size_t i = 0; i < m_widgets.size(); ++ i)
            m_widgets[i]->draw(mgr, get_child_space(i));
    }

    bool resize(axp::screen_space_size screen_space) override {
        const std::size_t count = m_widgets.size();

        // Allocate equal space for all widgets!
        const axp::screen_space_size m_space = {
            screen_space.width,
            screen_space.height / count
        };

        bool success = true;
        for (auto&& widget: m_widgets)
            success &= widget->resize(m_space);

        return success;
    }

    void on_mouse_event(mouse_event event) override {
        double spacing = 2.0f / m_widgets.size();

        int index = (int) floor((1.0f + event.point.y()) / spacing);
        assert(index <= 0 || index >= (int) m_widgets.size());

        // TODO: What if click is in spacing? Check!

        m_widgets[index]->on_mouse_event(event, get_child_space(index));
    }

    axp::space_range get_desired_space() override {
        axp::space_range max_child_requirements;
        for (auto&& widget: m_widgets)
            max_child_requirements.intersect(widget->get_desired_space());

        return max_child_requirements *= m_widgets.size();
    }

private:
    double m_spacing = 0.0f;

    math::rectangle get_child_space(int index) {
        const std::size_t children_count = m_widgets.size();

        const float delta = (2.0f - (children_count - 1) * m_spacing)
            / (float) children_count;

        const math::vec spacing = index *
            math::vec { 0.0f, delta + m_spacing };

        return math::rectangle {
            math::vec { - 1.0f,       - 1.0f } + spacing,
            math::vec {   1.0f, delta - 1.0f } + spacing
        };
    }
};

class bounded_widget: public widget {
public: // TODO: fix!
    math::rectangle subspace = math::rectangle {
        { -0.5f, -0.5f },
        {  0.5f,  0.5f }
    };
};

class draggable_window: public bounded_widget, private widget_wrapper {
public:
    using widget_wrapper::widget_wrapper;

    void draw(gl::drawing_manager &mgr) override {
        mgr.set_color({ 0.1f, 0.33f, 0.2f });
        mgr.draw_rectangle({ -1.0f, -1.0f },
                           {  1.0f,  1.0f });


        m_widget->draw(mgr, get_main_subspace());
    }

    void on_mouse_event(mouse_event event) override {
        if (!get_main_subspace().contains(event.point)) {
            if (event.kind != mouse_event::drag)
                return;

            subspace.x0 += event.movement.delta;
            subspace.x1 += event.movement.delta; // TODO: extract!
            return;
        }

        // TODO: make return bool!
        m_widget->on_mouse_event(event, get_main_subspace());
    }

    axp::space_range get_desired_space() override {
        return m_widget->get_desired_space(); // TODO: fix!
    }

    bool resize(axp::screen_space_size new_size) override {
        return m_widget->resize(new_size);    // TODO: fix!
    }

private:
    math::rectangle get_main_subspace() {
        return math::rectangle {
            { -0.97f, -0.97f },
            {  0.97f,  0.97f }
        };
    };
};

class windows: public widget {
public:
    windows(auto&&... widget) {
        (m_windows.emplace_back(widget), ...);
    };

    void draw(gl::drawing_manager &mgr) override {
        for (auto &&window: m_windows)
            window->draw(mgr, window->subspace);
    }

    axp::space_range get_desired_space() override {
        return axp::space_range(); // TODO: !
    }

    bool resize([[maybe_unused]] axp::screen_space_size new_size) override {
        return true;
    }

    void on_mouse_event(mouse_event event) override {
        for (auto &&window: m_windows)
            if (window->subspace.contains(event.point)) {
                window->on_mouse_event(event, window->subspace);
                return; // TODO: Windows cannot intersect!
            }
    }

private:
    std::vector<std::unique_ptr<bounded_widget>> m_windows;
};



class solid_button: public plain_widget { // TODO: bare bones for now
public:
    solid_button(math::vec3 color): m_color(color) {}

    void draw(gl::drawing_manager &mgr) override {
        mgr.set_color(m_color);
        mgr.draw_rectangle({ -1.0f, -1.0f }, { 1.0f, 1.0f });
    }

    bool resize(axp::screen_space_size /* space */) override {
        return true; // Can be resized to any size!
    }

    axp::space_range get_desired_space() override {
        return axp::space_range();
    }

private:
    math::vec3 m_color;
};

class ui_window: public gl::simple_drawing_window {
public:
    ui_window(std::size_t width, std::size_t height, const char* name,
              std::unique_ptr<widget>&& root_widget)
        : gl::simple_drawing_window(width, height, name),
          m_root_widget(std::move(root_widget)),
          m_current_state(ui_window::idle) {}
    // TODO:              ^~~~~~~~~~~~~~~ ensure idle

    void loop_draw(gl::drawing_manager &mgr) override {
        m_root_widget->draw(mgr);
    }

    void on_mouse_moved(math::vec2 cursor) override {
        move(cursor);
    }

    void on_mouse_event(math::vec2 cursor,
                        gl::mouse_button button,
                        gl::mouse_action action) override {

        if (button != gl::mouse_button::LEFT) {
            // TODO: unimplemented
            return;
        }

        switch (action) {
        case gl::mouse_action::PRESS:
        case gl::mouse_action::REPEAT:
            press(cursor);
            break;

        case gl::mouse_action::RELEASE:
            release(cursor);
            break;
        }
    };


private:
    std::unique_ptr<widget> m_root_widget;

    enum input_state {
        idle,
        dragging,
        pressing,
        hovering,
    } m_current_state;

    math::vec2 m_last_cursor_position = { 0, 0 };

    void press(math::vec2 point) {
        switch (m_current_state) {
        case idle:
        case hovering:
            send_movement_event(point, mouse_event::hover);
            switch_to_contiguous(pressing, point);
            break;

        case pressing: 
        case dragging:
            assert(false && "Impossible transition, can't press again while dragging!");
        }

    }

    void release(math::vec2 point) {
        switch (m_current_state) {
        case pressing:
        case dragging:
            send_click_event(point, mouse_button::left);
            // no need to retain when idle state begun:
            m_current_state = idle;
            break;

        case idle:
        case hovering:
            assert(false && "Impossible, mouse button have already been released!");
        }
    }

    void move(math::vec2 point) {
        switch (m_current_state) {
        case pressing: // dragging just started
        case dragging: // dragging continues
            send_movement_event(point, mouse_event::drag);
            switch_to_contiguous(pressing, point);
            break;

        case idle:     // hovering just started
        case hovering: // hovering continues
            send_movement_event(point, mouse_event::hover);
            switch_to_contiguous(hovering, point);
            break;
        }

    }

    void send_movement_event(math::vec2 point, mouse_event::its_kind event_kind) {
        m_root_widget->on_mouse_event({
            .kind  = event_kind,
            .point = point,
            .movement = { // specific for drag & hover
                .delta = point - m_last_cursor_position
            }
        });
    }

    void send_click_event(math::vec2 point, mouse_button button) {
        m_root_widget->on_mouse_event({
            .kind  = mouse_event::click,
            .point = point,
            .access = { // specific for left & right click
                .clicked = button
            }
        });
    }

    // switching state requires remembering where switch happend,
    // because some states use this information later to produce
    // mouse_events (e.g. drag needs to know where it started)
    void switch_to_contiguous(input_state new_state, math::vec2 new_point) {
        m_current_state = new_state;
        m_last_cursor_position = new_point;
    }
};



namespace gl::gui {

    // class window: public widget {
    // public:
    //     void on_draw(gl::drawing_manager &mgr) override {
    //         for (const auto &contained_widget: m_contained_widgets)
    //             contained_widget->on_draw(mgr);
    //     }

    // private:
    //     std::vector<std::unique_ptr<widget>> m_contained_widgets;
    // };

    class window {
    private:
        math::rectangle m_space;
        std::unique_ptr<widget> m_widget;
    };

    class button: public widget {
    public:
        void draw(gl::drawing_manager &mgr) override {
            mgr.draw_rectangle({ -1.0f, -1.0f }, { 0.0f, 0.0f });
        }

        virtual ~button() = default;
    };

    class window_space: public widget {
    public:
        void draw(gl::drawing_manager &mgr) override {
            mgr.set_color({ 1.0f, 0.5f, 0.3f });
            mgr.set_width(0.03f);
            mgr.draw_line({ 0.0f, 0.0f }, { 0.5f, 0.5f });
        }

        virtual ~window_space() = default;
    // private:
    //     std::
    };

    class gui_renderer: public gl::vector_renderer {
    public:
        gui_renderer(std::unique_ptr<widget>&& top_level_widget)
            : m_top_level_widget(std::move(top_level_widget)) {}

        void vector_draw(gl::drawing_manager &mgr) override {
            m_top_level_widget->draw(mgr);
        }

    private:
        std::unique_ptr<widget> m_top_level_widget;
    };

}
