#pragma once

#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <ostream>
#include <type_traits>
#include <tuple>
#include <utility>

namespace details {

    // If vector stores floating point values, use the same value for len type,
    // on the other hand, if it uses anything else fallback to double.
    template <typename element_type>
    using default_len_type =
        std::conditional_t<std::is_floating_point_v<element_type>, element_type, double>;

    template <typename vec_type, typename element_type>
    concept has_coordinates = requires(vec_type vec, int i) {
        { vec[i] } -> std::convertible_to<element_type>;
    };

    template <typename vec_type, typename element_type>
    concept right_multipliable = requires(vec_type vec, element_type value) {
        { vec * value };
    };

    template <typename value_type, typename callback_type>
    class catch_modifications_proxy {
    public:
        catch_modifications_proxy(value_type& editable_value, callback_type&& callback)
            : m_editable_value(editable_value), m_callback(callback) {}

        #define FORWARD_OPERATOR(name)                                         \
            template <typename new_value_type>                                 \
            catch_modifications_proxy& operator name(new_value_type&& value) { \
                m_callback(); /* Notify about assignment */                    \
                m_editable_value name std::forward<new_value_type>(value);     \
                return *this;                                                  \
            }

        // Forward all known assignment operators to source type (notify about whichever)
        FORWARD_OPERATOR(=)
        FORWARD_OPERATOR(+=)
        FORWARD_OPERATOR(-=)
        FORWARD_OPERATOR(/=)
        FORWARD_OPERATOR(*=)
        FORWARD_OPERATOR(^=)
        FORWARD_OPERATOR(%=)
        FORWARD_OPERATOR(&=)
        FORWARD_OPERATOR(|=)
        FORWARD_OPERATOR(>>=)
        FORWARD_OPERATOR(<<=)

        #undef FORWARD_OPERATOR

        // Get back original value, but read-only
        operator const value_type&() { return m_editable_value; }

    private:
        value_type& m_editable_value;
        callback_type m_callback;
    };


    template <typename element_type>
    struct strip_proxy { using type = element_type; };

    template <typename element_type, typename callback_type>
    struct strip_proxy<catch_modifications_proxy<element_type, callback_type>> {
        using type = element_type;
    };

    template <typename element_type>
    using strip_proxy_t = typename strip_proxy<element_type>::type;


    template <typename range_type, typename callback_type>
    class catch_modifications_iter_proxy {
    public:
        catch_modifications_iter_proxy(range_type current, callback_type callback)
            : m_current(current), m_callback(callback) {}

        catch_modifications_iter_proxy<range_type, callback_type>& operator++() {
            ++ m_current; // Go to the next element
            return *this;
        }

        template <typename other_callback_type>
        bool operator!=(const catch_modifications_iter_proxy<range_type, other_callback_type>& other) const {
            return m_current != other.m_current;
        }

        auto operator*() {
            return catch_modifications_proxy(*m_current, std::move(m_callback));
        }

        range_type m_current; // TODO: make private somehow

    private:
        callback_type m_callback;
    };

    template <typename impl_type, typename element_type, size_t count,
              typename len_type = default_len_type<element_type>>
    class vec_base {
    public:
        template<typename... vector_coordinates>
        constexpr vec_base(vector_coordinates... initializer_coordinates)
            : m_coordinates { initializer_coordinates... } {

            const size_t n = sizeof...(vector_coordinates);
            static_assert(n == count, "Invalid number of vector coordinates!");
        }

        constexpr auto operator[](const size_t index) {
            return catch_modifications_proxy(m_coordinates[index], get_change_callback());
        }

        constexpr const element_type &operator[](const size_t index) const {
            return m_coordinates[index];
        }

        // Make sure there is no vec.z() in two-dimensional vec (which has only x and y)
        #define STATIC_ASSERT_AVAILABILITY(coordinate_name, index)                 \
            static_assert(count > index, "Vector doesn't have `" #coordinate_name  \
                                         "' because it's too small!");

        #define COORDINATE_GETTER(coordinate_name, index)                          \
            constexpr auto coordinate_name() {                                     \
                STATIC_ASSERT_AVAILABILITY(coordinate_name, index)                 \
                return (*get_impl())[index];                                       \
            }                                                                      \
                                                                                   \
            constexpr const element_type &coordinate_name() const {                \
                STATIC_ASSERT_AVAILABILITY(coordinate_name, index)                 \
                return (*get_impl())[index];                                       \
            }                                                                      \

        // There are conventional names for vector coordinates up to 4
        #define COORDINATE_GETTERS(name0, name1, name2, name3)                     \
            COORDINATE_GETTER(name0, 0) COORDINATE_GETTER(name1, 1)                \
            COORDINATE_GETTER(name2, 2) COORDINATE_GETTER(name3, 3)                \

        // ==> Define same coordinate getters that OpenGL uses:

        // Useful when accessing vectors that represent points or normals:
        COORDINATE_GETTERS(x, y, z, w)

        // Useful when accessing vectors that represent colors:
        COORDINATE_GETTERS(r, g, b, a)

        // Useful when accessing vectors that represent texture coordinates:
        COORDINATE_GETTERS(s, t, p, q)

        #undef COORDINATE_GETTERS
        #undef COORDINATE_GETTER

        #undef STATIC_ASSERT_AVAILABLE

        constexpr auto begin() {
            return catch_modifications_iter_proxy(m_coordinates,
                                                  get_change_callback());
        }

        constexpr auto end() {
            return catch_modifications_iter_proxy(m_coordinates + count,
                                                  get_change_callback());
        }

        constexpr const element_type* begin() const { return m_coordinates;         }
        constexpr const element_type*   end() const { return m_coordinates + count; }


        #define DEFINE_ASSIGNMENT(assignment)                                      \
            template <has_coordinates<element_type> other_vec>                     \
            constexpr impl_type& operator assignment(const other_vec& other) {     \
                get_impl()->notify_vector_changed();                               \
                for (size_t i = 0; i < count; ++ i)                                \
                    m_coordinates[i] assignment other[i];                          \
                                                                                   \
                return *get_impl();                                                \
            }

        DEFINE_ASSIGNMENT(*=) DEFINE_ASSIGNMENT(/=)
        DEFINE_ASSIGNMENT(-=) DEFINE_ASSIGNMENT(+=)

        #undef DEFINE_ASSIGNMENT


        #define DEFINE_OPERATOR(name, corresponding_assignment)                    \
            template <has_coordinates<element_type> other_vec>                     \
            constexpr impl_type operator name(const other_vec& other) const {      \
                impl_type new_vec = *get_impl();                                   \
                return new_vec corresponding_assignment other;                     \
            }                                                                      \

        DEFINE_OPERATOR(*, *=) DEFINE_OPERATOR(-, -=)
        DEFINE_OPERATOR(+, +=) DEFINE_OPERATOR(/, /=)

        #undef DEFINE_OPERATOR

        constexpr impl_type& operator*=(const element_type value) {
            for (element_type& coordinate: m_coordinates)
                coordinate *= value;

            return *get_impl();
        }

        constexpr impl_type operator*(const element_type value) const {
            impl_type new_vec = *get_impl();
            return new_vec *= value;
        }

        template <right_multipliable<element_type> other_vec>
        friend constexpr impl_type operator*(const element_type value,
                                             const other_vec& other) {
            return other * value;
        }

        template <typename other_vector>
        constexpr element_type dot(const other_vector& other) const {
            element_type accumulator {};
            for (size_t i = 0; i < count; ++ i)
                accumulator += (*get_impl())[i] * other[i];

            return accumulator;
        }

        constexpr len_type len() const {
            return static_cast<len_type>(sqrt(this->dot(*this)));
        }

        constexpr impl_type& normalize() & {
            return (*this) *= (static_cast<len_type>(1) / len());
        }

        constexpr impl_type& rotate(double angle) & {
            static_assert(count == 2, "Rotation for now is impemented only for 2D case!");

            x() = static_cast<element_type>(cos(angle) * x() - sin(angle) * y());
            y() = static_cast<element_type>(sin(angle) * x() + cos(angle) * y());

            return *get_impl();
        }

        // If you need to make current vector perpendicular to itself, you can use rotate!
        constexpr impl_type perpendicular() const {
            static_assert(count == 2, "Perpendicular for now is impemented only for 2D case!");

            return impl_type { y(), - x() };
        }

        constexpr impl_type rotated(double angle) const {
            impl_type new_vec = *get_impl();
            return new_vec.rotate(angle);
        }

        constexpr impl_type normalized() const {
            impl_type new_vec = *get_impl();
            return new_vec.normalize();
        }


        friend std::ostream& operator<<(std::ostream& os, const impl_type& vector) {
            os << "(";

            for (size_t i = 0; i < count; ++ i) {
                os << vector[i];
                if (i != count - 1) os << ", ";
            }

            return os << ")";
        }

    private:
        element_type m_coordinates[count];

        // Simplify CRTP usage of implementation class
              impl_type* get_impl()       { return static_cast<      impl_type*>(this); }
        const impl_type* get_impl() const { return static_cast<const impl_type*>(this); }

        void notify_vector_changed() {} // Can be overloaded via CRTP

        auto get_change_callback() {
            return [this]() { get_impl()->notify_vector_changed(); };
        }

    };

}

namespace math {

    // There's no way to use type alias in inheritance, or class specifier
    // (e.g friend), macro is cringe, but still will do the job.
    #define VEC_BASE_TYPE details::vec_base<vec<element_type, count, len_type>,          \
                                                element_type, count, len_type>

    // Deduction guide has to be defined twice (for both vecs), but they
    // have the exact same interface, so we'll use macro (there is no other way)
    #define VEC_DEDUCTION_GUIDE                                                          \
        template<class... vector_coordinates>                                            \
        vec(vector_coordinates... initializer_coordinates) ->                            \
            vec<details::strip_proxy_t<                                                  \
                    std::tuple_element_t<0, std::tuple<vector_coordinates...>>>,         \
                sizeof...(vector_coordinates)>; // Deduce vec type by first element!

    inline // Makes uncached::vec "leak" in outer namespace, making it "default" in a way
    namespace uncached {

        template <typename element_type, size_t count,
                  typename len_type = details::default_len_type<element_type>>
        class vec: public VEC_BASE_TYPE {
        public:
            using VEC_BASE_TYPE::vec_base;
        };

        VEC_DEDUCTION_GUIDE

    }

    namespace cached {

        template <typename element_type, size_t count,
                  typename len_type = details::default_len_type<element_type>>
        class vec: public VEC_BASE_TYPE {
        public:
            using VEC_BASE_TYPE::vec_base;

            constexpr len_type len() const /* CRTP override */ {
                if (!std::isnan(m_cached_length))
                    return m_cached_length;

                return m_cached_length = VEC_BASE_TYPE::len();
            }

        private:
            mutable len_type m_cached_length = std::numeric_limits<len_type>::quiet_NaN();

            // Do not expose notify_vector_changed, yet override it!
            friend class VEC_BASE_TYPE;

            void notify_vector_changed() /* CRTP override */ {
                // Invalidate current length, it will be recalculated on demand!
                m_cached_length = std::numeric_limits<len_type>::quiet_NaN();
            }
        };

        VEC_DEDUCTION_GUIDE

    }

    #undef VEC_BASE_TYPE
    #undef VEC_DEDUCTION_GUIDE

    // Use shorthands from OpenGL
    using vec2 = vec<float, 2>;
    using vec3 = vec<float, 3>;
    using vec4 = vec<float, 4>;

};
