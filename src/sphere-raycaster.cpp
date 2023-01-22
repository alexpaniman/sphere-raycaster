#include "axes.h"
#include "gl.h"

#include "rect.h"
#include "colored-vertex.h"

#include "renderer-handler-window.h"
#include "simple-drawing-renderer.h"
#include "simple-window.h"
#include "pixel-drawing-manager.h" // TODO: rename

#include "sphere.h"
#include "ui-renderer.h"
#include "vec.h"
#include "math-utils.h"

#include "renderer-config.h"

#include "matrix.h"

#include <climits>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <optional>
#include <stdexcept>

using math::vec;

class cpu_circle_raycaster: public gl::pixel_drawing_window<cpu_circle_raycaster> {
public:
    using gl::pixel_drawing_window<cpu_circle_raycaster>::pixel_drawing_window;

    renderer_config cfg = {
        .light = {
            .color     = { 0.5f, 0.5f,  0.5f },
            .position  = {-7.59025f, -8.68034f, -3.25375f},
        },

        .ambient_color = { 0.1f, 0.1f,  0.7f },
        .surface_color = { 1.0f, 1.0f,  1.0f },

        .camera = {
            .eye    = { 0.442229f, 0.442229f, 0.442229f },
            .center = { 0.0f, 0.0f, 0.0f },
 
            .distance_from_screen = 0.75179f
        }
    };

    void draw() override {
        srand(0);

        if (get_fps()) {
            cfg.light.position.slice(0, 1).rotate(0.1f);

            // cfg.camera.distance_from_screen *= 1.01f;
            cfg.camera.eye *= 1.01f;

            // for (auto&& sphere: spheres)
            //     sphere.radius *= 1.01f;
        }

        gl::pixel_drawing_window<cpu_circle_raycaster>::draw();
    }

    std::vector<sphere> spheres;

    void setup() override {

        gl::pixel_drawing_window<cpu_circle_raycaster>::setup();

        spheres.push_back({
            .center = { 0.0f, 0.0f, 0.0f },
            .radius = 0.2f
        });

        spheres.push_back({
            .center = { 0.0f, 0.2f, 0.2f },
            .radius = 0.2f
        });
    }

    math::vec3 draw_pixel(math::vec2 position2D) /* CRTP override */ {

        ray current_ray = get_eye_camer_ray(position2D, cfg.camera);

        static vec closest_color { 0.0f, 0.0f, 0.0f };
        double min_distance = INFINITY;

        for (sphere& current: spheres) {
            vec center = { static_cast<double>(current.center.x()),
                           static_cast<double>(current.center.y()),
                           static_cast<double>(current.center.z()) };

            auto intersection =
                current.intersect_with_ray(current_ray.to, current_ray.from);

            if (intersection) {
                double new_distance = (*intersection - current_ray.from).len();
                if (new_distance < min_distance) {
                    closest_color = current.get_sphere_surface_color(*intersection, cfg);
                    min_distance = new_distance;
                }
            }

            // TODO: simple raycasting

            // double z = sqrt(pow(current.radius, 2) - position2D.dot(position2D));
            // vec position = { position2D.x(), position2D.y(), z };

            // if (position2D.len() <= current.radius)
            //     return current.get_sphere_surface_color(position, cfg);

            // return { 0.0f, 0.0f, 0.0f };
        }

        if (min_distance == INFINITY)
            return { 0.0f, 0.0f, 0.0f };

        return closest_color;
    }

    void on_fps_updated() override {
        std::cout << "FPS: " << get_fps() << std::endl;
        // std::cout << cfg.camera.eye << std::endl;
    }

private:
    struct ray {
        math::vec3 from;
        math::vec3 to;
    };

    // ray get_camera_position(math::vec2 on_screen, camera_cfg& cam) {
    //     math::vec3 horizontal =
    //         math::vec { cam.direction.y(),
    //                     cam.direction.dot(cam.screen_position) - cam.direction.x(),
    //                     0.0f
    //                   } - cam.screen_position;

    //     math::vec3 vertical = horizontal.cross(cam.direction);
    //     // TODO: normalize?

    //     return {
    //         - cam.distance_from_screen * cam.direction
    //             + cam.screen_position,

    //         horizontal * on_screen.x() + vertical * on_screen.y()
    //             + cam.screen_position
    //     };
    // }

    ray get_eye_camer_ray(math::vec2 on_screen, camera_cfg &cam) {
        const vec up = { 0.0f, 1.0f, 0.0f };

        vec direction = (cam.center - cam.eye).normalize();
        vec right = direction.cross(up);

        vec screen_pos = cam.eye -
            direction * cam.distance_from_screen;

        vec target = right * on_screen.x() + up * on_screen.y();

        return { cam.eye, target + screen_pos };
    }

};

class gui_window: public gl::renderer_handler_window {
public:
    gui_window(int width, int height, const char* window_name,
               std::unique_ptr<widget>&& top_level_widget)
        : gl::renderer_handler_window { width, height, window_name },
          m_gui_renderer { std::make_unique<gl::gui::gui_renderer>(std::move(top_level_widget)) } {

        set_renderer(m_gui_renderer.get()); // TODO: probably don't need window for this
    }

private:
    std::unique_ptr<gl::gui::gui_renderer> m_gui_renderer;
};

class vector_window: public gl::simple_drawing_window {
public:
    using gl::simple_drawing_window::simple_drawing_window;

    void loop_draw(gl::drawing_manager &mgr) override {
        // math::axes ax { { -0.5f, -0.5f }, { 0.5f, 0.5f } };
        // mgr.set_axes(ax);

        // mgr.set_width(0.03f);
        // mgr.set_color({ 1.0f, 0.5f, 0.3f });

        // mgr.draw_line({ 0.5f, 0.3f }, { 0.3f, 0.2f });
        list->draw(mgr);
    }

    void on_fps_updated() override {
        std::cout << "FPS: " << get_fps() << "\n";
    }

    void on_key_pressed(gl::key pressed_key) override {
        (void) pressed_key;
        // list->m_widgets.push_back(std::make_unique<plain_button>(math::random));
    };

    void on_mouse_moved(math::vec2 position) override {
        std::cout << position << "\n";
        // list->on_hover(position);
    }

    equally_spaced_list* list = nullptr;
};

class my_window: public ui_window {
    using ui_window::ui_window;
};


int main() {


    auto widget = new draggable_window {
        std::make_unique<solid_button>(
            math::vec { 0.3f, 0.5f, 0.7f })
    };

    std::unique_ptr<windows> main(new windows(widget));

    my_window window(1440, 1440, "My window", std::move(main));
    window.draw_loop();

    // vector_window vecs(3440, 1440, "My vector drawer!");

    // vecs.list = new equally_spaced_list {};
    // vecs.draw_loop();


    // std::cout << x * vec { 1, 2 };

    // cpu_circle_raycaster drawer(1080, 1080, "My vector drawer!");
    // drawer.draw_loop();

    // auto top_level = new gl::gui::window_space;
    // auto top_level0 = static_cast<widget*>(top_level);

    // gui_window gui(1080, 1080, "My GUI window", std::unique_ptr<widget>(top_level0));
    // gui.draw_loop();
}
