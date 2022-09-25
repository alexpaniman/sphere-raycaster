#pragma once

#include "colored-vertex.h"
#include "drawing-manager.h"
#include "opengl-setup.h"
#include "vec-layout.h"
#include "vec.h"
#include "vertex-vector-array.h"

namespace gl {

    template <typename impl_type>
    class pixel_drawing_window: public gl::window {
    public:
        using gl::window::window;

        void setup() override {
            gradient_shader.from_file("res/gradient.glsl");
            vertices.set_layout(math::vector_layout<float, 2>() +
                                math::vector_layout<float, 3>());

            // Initialize matrix
            for (int i = 0; i < height; ++ i)
                for (int j = 0; j < width; ++ j) {
                    math::vec current_pos {
                        2 * i / static_cast<float>(height) - 1,
                        2 * j / static_cast<float>(width)  - 1
                    };

                    vertices.push_back({ current_pos, DEFAULT_COLOR });
                }
        }

        void draw() override {
            for (int i = 0; i < height; ++ i)
                for (int j = 0; j < width; ++ j) {
                    colored_vertex &current = vertices[i * height + j];

                    // Update color:
                    current.color = static_cast<impl_type*>(this)
                        ->draw_pixel(current.point);
                }

            vertices.update(); // Update point list

            gl::draw(gl::drawing_type::POINTS, vertices, gradient_shader);
        }

        // Default implementation
        math::vec4 draw_pixel(math::vec2 /* position */) {
            // Just white:
            return { 1.0f, 1.0f, 1.0f, 1.0f };
        }

    private:
        gl::vertex_vector_array<colored_vertex> vertices;
        gl::shaders::shader_program gradient_shader;

        inline static constexpr math::vec3 DEFAULT_COLOR = { 1.0f, 1.0f, 1.0f };
    };

}
