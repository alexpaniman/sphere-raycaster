#pragma once

#include "vec.h"

struct light_source {
    math::vec3 color;
    math::vec3 position;
};

struct camera_cfg {
    // math::vec3 direction;
    // math::vec3 screen_position;

    math::vec3 eye;
    math::vec3 center;

    float distance_from_screen;

};

struct renderer_config {
    light_source light;

    math::vec3 ambient_color;
    math::vec3 surface_color;

    camera_cfg camera;
};
