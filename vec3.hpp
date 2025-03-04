#ifndef VEC3_HPP
#define VEC3_HPP

#include <array>
#include <cmath>
#include <ostream>

template <typename T>
class vec3
{
public:
    vec3() = default;
    vec3(const T &e0) : e{e0, e0, e0} {}
    vec3(const T &e0, const T &e1) : e{e0, e1, T()} {}
    vec3(const T &e0, const T &e1, const T &e2) : e{e0, e1, e2} {}

    template <typename U>
    inline operator vec3<U>() const { return {static_cast<U>(e[0]), static_cast<U>(e[1]), static_cast<U>(e[2])}; }

    inline T x() const { return e[0]; }
    inline T y() const { return e[1]; }
    inline T z() const { return e[2]; }
    inline T r() const { return e[0]; }
    inline T g() const { return e[1]; }
    inline T b() const { return e[2]; }

    inline T &x() { return e[0]; }
    inline T &y() { return e[1]; }
    inline T &z() { return e[2]; }
    inline T &r() { return e[0]; }
    inline T &g() { return e[1]; }
    inline T &b() { return e[2]; }

    inline vec3 operator-() const { return {-e[0], -e[1], -e[2]}; }
    inline vec3 operator-(const vec3 &v) const { return {e[0] - v.e[0], e[1] - v.e[1], e[2] - v.e[2]}; }
    inline vec3 operator+(const vec3 &v) const { return {e[0] + v.e[0], e[1] + v.e[1], e[2] + v.e[2]}; }
    inline vec3 operator*(const vec3 &v) const { return {e[0] * v.e[0], e[1] * v.e[1], e[2] * v.e[2]}; }
    inline vec3 operator/(const vec3 &v) const { return {e[0] / v.e[0], e[1] / v.e[1], e[2] / v.e[2]}; }

    inline vec3 &operator-=(const vec3 &v) { return *this = *this - v; }
    inline vec3 &operator+=(const vec3 &v) { return *this = *this + v; }
    inline vec3 &operator*=(const vec3 &v) { return *this = *this * v; }
    inline vec3 &operator/=(const vec3 &v) { return *this = *this / v; }

    inline vec3<bool> operator!() const { return {!e[0], !e[1], !e[2]}; }
    inline vec3<bool> operator!=(const vec3 &v) const { return !(*this == v); }
    inline vec3<bool> operator==(const vec3 &v) const { return {e[0] == v.e[0], e[1] == v.e[1], e[2] == v.e[2]}; }
    inline vec3<bool> operator<(const vec3 &v) const { return {e[0] < v.e[0], e[1] < v.e[1], e[2] < v.e[2]}; }
    inline vec3<bool> operator<=(const vec3 &v) const { return !(*this > v); }
    inline vec3<bool> operator>(const vec3 &v) const { return {e[0] > v.e[0], e[1] > v.e[1], e[2] > v.e[2]}; }
    inline vec3<bool> operator>=(const vec3 &v) const { return !(*this < v); }

    inline vec3 clamp(const vec3 &minV, const vec3 &maxV) const { return this->max(minV).min(maxV); }
    inline vec3 max(const vec3 &v) const { return select(*this > v, *this, v); }
    inline vec3 min(const vec3 &v) const { return select(*this < v, *this, v); }
    template <typename U>
    friend inline vec3<U> select(const vec3<bool> &condition, const vec3<U> &true_value, const vec3<U> &false_value);

    inline vec3 cross(const vec3 &v) const
    {
        return {e[1] * v.e[2] - e[2] * v.e[1],
                e[2] * v.e[0] - e[0] * v.e[2],
                e[0] * v.e[1] - e[1] * v.e[0]};
    }
    inline vec3 div(const vec3 &v) const
    {
        return select(v == vec3(), {}, *this / v);
    }
    inline T dot(const vec3 &v) const { return ((*this) * v).sum(); }
    inline T length() const { return sqrt(this->dot(*this)); }
    inline vec3 normalized() const
    {
        T length = this->length();
        return length == T() ? *this : *this / length;
    }
    inline T sum() const { return e[0] + e[1] + e[2]; }

    template <typename U = T, std::enable_if_t<std::is_same_v<U, bool>> * = nullptr>
    inline bool all() const
    {
        return e[0] && e[1] && e[2];
    }
    template <typename U = T, std::enable_if_t<std::is_same_v<U, bool>> * = nullptr>
    inline bool any() const
    {
        return e[0] || e[1] || e[2];
    }

    friend inline std::ostream &operator<<(std::ostream &os, const vec3 &v)
    {
        return os << '{' << v.e[0] << ' ' << v.e[1] << ' ' << v.e[2] << '}';
    }

private:
    std::array<T, 3> e;
};

template <typename T>
inline vec3<T> select(const vec3<bool> &condition, const vec3<T> &true_value, const vec3<T> &false_value)
{
    return {condition.e[0] ? true_value.e[0] : false_value.e[0],
            condition.e[1] ? true_value.e[1] : false_value.e[1],
            condition.e[2] ? true_value.e[2] : false_value.e[2]};
}

#endif // VEC3_HPP
