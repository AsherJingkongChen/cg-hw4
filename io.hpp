#ifndef IO_HPP
#define IO_HPP

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "vec3.hpp"

void encode_ppm_p3(
    uint32_t width,
    uint32_t height,
    const std::vector<vec3<uint8_t>> &colors,
    const std::string &filename)
{
    using std::endl;

    std::ofstream file(filename, std::ios::out);
    if (!file.is_open())
    {
        std::cerr << "[IO Error] Can't open the file: " << filename << std::endl;
        return;
    }
    if (colors.size() < width * height)
    {
        std::cerr << "[IO Error] The buffer is too small: " << filename << std::endl;
        return;
    }

    file << "P3" << endl
         << width << ' ' << height << endl
         << 255 << endl;

    for (const auto &color : colors)
    {
        file.operator<<(color.r()) << ' ';
        file.operator<<(color.g()) << ' ';
        file.operator<<(color.b()) << endl;
    }
}

vec3<uint8_t> convert_vec3_float_to_uint8_once(
    const vec3<float> &value,
    const vec3<float> &min,
    const vec3<float> &max)
{
    return ((value - min) / (max - min) * 255.0f + 0.5f).clamp(0.0f, 255.0f);
}

std::vector<vec3<uint8_t>> convert_vec3_float_to_uint8_many(
    const std::vector<vec3<float>> &values,
    const vec3<float> &min,
    const vec3<float> &max)
{
    std::vector<vec3<uint8_t>> result;
    result.reserve(values.size());
    for (const auto &value : values)
    {
        result.push_back(convert_vec3_float_to_uint8_once(value, min, max));
    }
    return result;
}

#endif // IO_HPP
