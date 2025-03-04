#include <filesystem>
#include <utility>
#include "io.hpp"
#include "ray3.hpp"
#include "render.hpp"
#include "vec3.hpp"

const uint32_t WIDTH = 400;
const uint32_t HEIGHT = 200;

int main()
{
	using namespace std;
	namespace fs = std::filesystem;

	vector<vec3<float>> colors(WIDTH * HEIGHT);
	vector<vec3<uint8_t>> colors_quantized;
	shader_t<vec3<float>> shader;

	if (!fs::exists("outputs"))
	{
		fs::create_directory("outputs");
	}

	auto evaluate = [&](const string &out_path)
	{
		rasterize(WIDTH, colors, shader);
		colors_quantized = convert_vec3_float_to_uint8_many(colors, 0.0f, 1.0f);
		encode_ppm_p3(WIDTH, HEIGHT, colors_quantized, out_path);
	};

	auto bulb = vec3<float>{-1.0f, 1.0f, 0.0f} * (HEIGHT * 0.5f);
	auto grad = ray3<float>{{0.6f, 0.3f, 0.6f}, {0.9f, 0.6f, 0.3f}};
	auto shift = vec3<float>{WIDTH, HEIGHT, HEIGHT} * 0.5f;
	auto radius = shift.z() * 0.5f;

	shader = [&](auto x, auto y, auto &color, const auto &)
	{
		auto pixel = vec3<float>{x, y};
		auto point = pixel - shift;
		color = grad.at(point.normalized().y() * 0.5f + 0.5f);
	};
	evaluate("outputs/skybox.ppm");

	shader = [&](auto x, auto y, auto &color, const auto &)
	{
		auto pixel = vec3<float>{x, y};
		auto point = pixel - shift;
		point.z() = 0.0f;
		auto length = point.length();
		if (length < radius)
		{
			color = {1.0f, 0.0f, 0.0f};
		}
	};
	evaluate("outputs/red.ppm");

	shader = [&](auto x, auto y, auto &color, const auto &)
	{
		auto pixel = vec3<float>{x, y};
		auto point = pixel - shift;
		point.z() = 0.0f;
		auto length = point.length();
		if (length < radius)
		{
			point.y() = -point.y();
			point.z() = sqrt(radius * radius - length * length);
			auto normal = point.normalized();
			color = normal * 0.5f + 0.5f;
		}
	};
	evaluate("outputs/normal.ppm");

	auto colors_bonus = colors;

	shader = [&](auto x, auto y, auto &color, const auto &)
	{
		auto pixel = vec3<float>{x, y};
		auto point = pixel - shift;
		point.z() = 0.0f;
		auto length = point.length();
		if (length < radius)
		{
			point.y() = -point.y();
			point.z() = sqrt(radius * radius - length * length);
			auto normal = point.normalized();
			point.z() -= shift.z();
			auto light = (bulb - point).normalized();
			color = vec3{1.0f} * max(normal.dot(light), 0.0f);
		}
	};
	evaluate("outputs/shading.ppm");

	colors = std::move(colors_bonus);
	shader = [&](auto x, auto y, auto &color, const auto &)
	{
		const uint32_t WIDTH_AA = 16u;
		const uint32_t WIDTH_AA_EXP_2 = WIDTH_AA * WIDTH_AA;
		vec3<float> color_accum{};
		for (auto index = WIDTH_AA_EXP_2; index--;)
		{
			auto [dy, dx] = std::ldiv(index, WIDTH_AA);
			auto pixel = vec3<float>{x, y};
			auto offset = (vec3<float>{dx, dy} + 0.5f) / WIDTH_AA;
			auto point = pixel + offset - shift;
			point.z() = 0.0f;
			auto length = point.length();
			if (length < radius)
			{
				point.y() = -point.y();
				point.z() = sqrt(radius * radius - length * length);
				auto normal = point.normalized();
				point.z() -= shift.z();
				auto light = (bulb - point).normalized();
				color_accum += (vec3{1.6f, 0.4f, 0.8f} * max(light.dot(normal), 0.0f) +
								vec3{1.6f, 0.8f, 0.4f} * max(light.dot(grad.at(normal.y() * 0.5f + 0.5f)), 0.0f) +
								vec3{0.8f, 0.4f, 1.6f} * max(light.dot(grad.at(-normal.x() * 1.5f - 0.5f)), 0.0f)) *
							   0.8f;
			}
			else
			{
				color_accum += grad.at(point.normalized().y() * 0.5f + 0.5f);
			}
		}
		color = (color_accum / WIDTH_AA_EXP_2).clamp(0.0f, 1.0f);
	};
	evaluate("outputs/bonus.ppm");

	return 0;
}
