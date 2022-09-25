#pragma once

#include <cstddef>

namespace math {

    template <typename type>
    constexpr type pow(type value, size_t power) {
        if (power == 1)
            return value;

        return power % 2 == 0?
              pow(value * value, power / 2)
            : value * pow(value, power - 1);
    }

}
