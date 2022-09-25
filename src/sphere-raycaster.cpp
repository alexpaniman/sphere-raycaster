#include "colored-vertex.h"
#include "gl.h"
#include "simple-window.h"
#include "pixel-drawing-manager.h" // TODO: rename
#include "vec.h"

using math::vec;

struct light_source {
    math::vec3 color;
    math::vec3 position;
};

struct renderer_config {
    light_source light;

    math::vec3 ambient_color;
    math::vec3 surface_color;

    math::vec3 view_position;
};

class cpu_circle_raycaster: public gl::pixel_drawing_window<cpu_circle_raycaster> {
public:
    using gl::pixel_drawing_window<cpu_circle_raycaster>::pixel_drawing_window;

    static float clamp(float value, float min, float max) {
        if (value < min)
            return min;

        if (value > max)
            return max;

        return value;
    }

    math::vec3 draw_pixel(math::vec2 position2D) /* CRTP override */ {
        const float radius = 0.7f;

        float z = sqrt(radius * radius - position2D.dot(position2D));
        vec position = { position2D.x(), position2D.y(), z };

        static renderer_config cfg = {
            .light = {
                .color    = { 0.5f, 0.5f,  0.5f },
                .position = { 7.0f, 7.0f,  7.0f }
            },

            .ambient_color = { 0.1f, 0.1f,  0.7f },
            .surface_color = { 1.0f, 1.0f,  1.0f },

            .view_position = { 0.0f, 0.0f, -3.5f }
        };

        if (position2D.len() <= radius)
            return get_sphere_surface_color(position, cfg);

        return { 0.0f, 0.0f, 0.0f };
    }

    void on_fps_updated() override {
        std::cout << "FPS: " << get_fps() << std::endl;
    }

private:

    math::vec3 get_sphere_surface_color(math::vec3 position, const renderer_config &cfg) {
        vec normal = position.normalized();

        vec relative_light = (cfg.light.position - position).normalized();
        vec relative_view  = (cfg.view_position  - position).normalized();

        float sin_alpha = normal.dot(relative_light);

        vec mirrored_light = relative_light - 2 * sin_alpha * normal;
        float sin_phi = mirrored_light.dot(relative_view);

        float diffuse  = clamp(sin_alpha,        0, 10000);
        float specular = clamp(pow(sin_phi, 15), 0, 10000);

        return specular * cfg.light.color +
            (diffuse * vec(1.0f, 1.0f, 1.0f) + cfg.ambient_color)
                * cfg.light.color * cfg.surface_color;
    }
};

int main() {
    cpu_circle_raycaster drawer(1080, 1080, "My vector drawer!");
    drawer.draw_loop();
}
