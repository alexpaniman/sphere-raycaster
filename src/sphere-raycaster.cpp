#include "gl.h"

#include "rect.h"
#include "colored-vertex.h"

#include "simple-window.h"
#include "pixel-drawing-manager.h" // TODO: rename

#include "vec.h"
#include "math-utils.h"

#include <climits>
#include <optional>

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

    renderer_config cfg = {
        .light = {
            .color     = { 0.5f, 0.5f,  0.5f },
            .position  = { 7.0f, 7.0f,  7.0f }
        },

        .ambient_color = { 0.1f, 0.1f,  0.7f },
        .surface_color = { 1.0f, 1.0f,  1.0f },

        .view_position = { 0.0f, 0.0f, -1.1f }
    };

    void draw() override {
        // srand(0);

        if (get_fps()) {
            math::vec2 view2D { cfg.view_position.x(), cfg.view_position.z() };
            view2D.rotate(0.01f);

            cfg.view_position.x() = view2D.x();
            cfg.view_position.z() = view2D.y();
        }

        gl::pixel_drawing_window<cpu_circle_raycaster>::draw();
    }

    math::vec3 draw_pixel(math::vec2 position2D) /* CRTP override */ {

        const float radius = 0.7f;

        auto intersection =
            trace_ray_to_sphere({ position2D.x(), position2D.y(), 0.0f }, radius, cfg.view_position);

        if (intersection)
            return get_sphere_surface_color(*intersection, cfg);

        return { 0.0f, 0.0f, 0.0f };

        float z = sqrt(radius * radius - position2D.dot(position2D));
        vec position = { position2D.x(), position2D.y(), z };

        if (position2D.len() <= radius)
            return get_sphere_surface_color(position, cfg);

        return { 0.0f, 0.0f, 0.0f };
    }

    void on_fps_updated() override {
        std::cout << "FPS: " << get_fps() << std::endl;
        std::cout << cfg.view_position << std::endl;
    }

private:
    std::optional<math::vec3> trace_ray_to_sphere(math::vec3 position, float radius, math::vec3 view_pos) {
        vec ray_direction = view_pos - position;
        float a = ray_direction.dot(ray_direction),
            b = 2 * view_pos.dot(ray_direction),
            c = view_pos.dot(view_pos) - radius * radius;

        float discriminant = b * b - 4 * a * c;
        if (discriminant < 0)
            return std::nullopt;

        float t[] = {
            - b + sqrt(discriminant) / (2 * a),
            - b - sqrt(discriminant) / (2 * a)
        };

        math::vec3 intersections[] = {
            view_pos + ray_direction * t[0],
            view_pos + ray_direction * t[1]
        };

        return intersections[1];
    }

    math::vec3 get_sphere_surface_color(math::vec3 position, const renderer_config &cfg) {
        vec normal = position.normalized();

        vec relative_light = (cfg.light.position - position).normalized();
        vec relative_view  = (cfg.view_position  - position).normalized();

        float sin_alpha = normal.dot(relative_light);

        vec mirrored_light = relative_light - 2 * sin_alpha * normal;
        float sin_phi = mirrored_light.dot(relative_view);

        float diffuse  = math::clamp(sin_alpha,        0, 1);
        float specular = math::clamp(pow(sin_phi, 15), 0, 1);

        return specular * cfg.light.color +
            (diffuse * vec(1.0f, 1.0f, 1.0f) + cfg.ambient_color)
                * cfg.light.color * cfg.surface_color;
    }

};

int main() {
    cpu_circle_raycaster drawer(1080, 1080, "My vector drawer!");
    drawer.draw_loop();
}
