#pragma once

#include "vec.h"

namespace math {

    struct rectangle {
        math::vec2 x0, x1; // Top left point and bottom right point

        explicit rectangle(math::vec2 top_left, math::vec2 bottom_right)
            : x0(top_left), x1(bottom_right) {}

        rectangle& shrink(math::vec2 delta) {
            x0 += delta;
            x1 -= delta;

            return *this;
        }
    };

}
