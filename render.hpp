#ifndef RENDER_HPP
#define RENDER_HPP

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <vector>

// `void shader(uint32_t x, uint32_t y, T& datum, const std::vector<T>& data);`
template <typename T>
using shader_t = std::function<void(uint32_t, uint32_t, T &, const std::vector<T> &)>;

template <typename T>
std::vector<T> &rasterize(
    const uint32_t width,
    std::vector<T> &data,
    const shader_t<T>& shader)
{
    uint32_t index = 0;
    for (auto& datum : data)
    {
        auto [y, x] = std::ldiv(index++, width);
        shader(x, y, datum, data);
    }
    return data;
}

#endif // RENDER_HPP