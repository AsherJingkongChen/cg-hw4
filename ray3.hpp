#ifndef RAY3_HPP
#define RAY3_HPP

#include "vec3.hpp"

template <typename T>
class ray3
{
private:
    vec3<T> d;
    vec3<T> o;

public:
    ray3() = default;
    ray3(const vec3<T> &direction) : d(direction), o() {}
    ray3(const vec3<T> &origin, const vec3<T> &target) : d(target - origin), o(origin) {}

    inline vec3<T> direction() const { return d; }
    inline vec3<T> origin() const { return o; }

    inline vec3<T> &direction() { return d; }
    inline vec3<T> &origin() { return o; }

    inline vec3<T> at(const T &times) const { return d * times + o; }
};

#endif // RAY3_HPP
