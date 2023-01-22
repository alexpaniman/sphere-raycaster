#pragma once

#include <optional>
#include <math.h>

#include "math-utils.h"
#include "renderer-config.h"
#include "vec.h"

struct sphere {
    math::vec3 center;
    double radius;

    std::optional<math::vec3> intersect_with_ray(math::vec3 position, math::vec3 view_pos) {
        view_pos -= center;

        using math::vec;

        vec ray_direction = view_pos - position;
        double a = ray_direction.dot(ray_direction),
            b = 2 * view_pos.dot(ray_direction),
            c = view_pos.dot(view_pos) - pow(radius, 2);

        double discriminant = b * b - 4 * a * c;
        if (discriminant < 0)
            return std::nullopt;

        double t[] = {
            - b + sqrt(discriminant) / (2 * a),
            - b - sqrt(discriminant) / (2 * a)
        };

        math::vec3 intersections[] = {
            view_pos + ray_direction * t[0],
            view_pos + ray_direction * t[1]
        };

        if ((position - intersections[0]).len() < (position - intersections[1]).len()) {
            vec intersection = intersections[1];

            if ((intersection - view_pos).dot(ray_direction) > 0)
                return intersection;
        }

        vec intersection = intersections[0];
        if ((intersection - view_pos).dot(ray_direction) > 0)
            return intersection;

        return std::nullopt;
    }

    math::vec3 get_sphere_surface_color(math::vec3 position,
                                        const renderer_config &cfg) {

        using math::vec;

        vec normal = (position - center).normalized();

        // Add randomization to normal:
        normal.xy().rotate(rand() / (double) RAND_MAX * 0.05f);
        normal.xz().rotate(rand() / (double) RAND_MAX * 0.05f);

        vec relative_light = (cfg.light.position - position).normalized();
        vec relative_view  = (cfg.camera.eye - position).normalized();

        double sin_alpha = normal.dot(relative_light);

        vec mirrored_light = relative_light - 2 * sin_alpha * normal;
        double sin_phi = mirrored_light.dot(relative_view);

        double diffuse  = math::clamp(sin_alpha,        0, 1);
        double specular = math::clamp(pow(sin_phi, 15), 0, 1);

        return specular * cfg.light.color +
            (diffuse * vec(1.0f, 1.0f, 1.0f) + cfg.ambient_color)
                * cfg.light.color * cfg.surface_color;
    }

};
