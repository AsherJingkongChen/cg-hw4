#include <filesystem>
#include <vector>
#include <string>
#include <cmath>
#include <limits>
#include <algorithm>
#include <functional>
#include <random>

#include "io.hpp"
#include "ray3.hpp"
#include "vec3.hpp"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 400;
const float ASPECT_RATIO = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
const int SAMPLES_PER_PIXEL = 100;
const int SEED = 42;

struct Material
{
    vec3<float> albedo;
    float diffuse_k = 0.9f;
    float specular_k = 0.5f;
    float shininess = 32.0f;
    float reflectivity = 0.0f;
    float transparency = 0.0f;
    float refractive_index = 1.0f;
};

struct Sphere
{
    vec3<float> center;
    float radius;
    Material material;
};

struct HitRecord
{
    float t;
    vec3<float> point;
    vec3<float> normal;
    bool front_face;
    Material material;

    inline void set_face_normal(const ray3<float> &r, const vec3<float> &outward_normal)
    {
        front_face = r.direction().dot(outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

bool hit_sphere(const ray3<float> &r, const Sphere &s, float t_min, float t_max, HitRecord &rec)
{
    vec3<float> oc = r.origin() - s.center;
    auto a = r.direction().dot(r.direction());
    auto half_b = oc.dot(r.direction());
    auto c = oc.dot(oc) - s.radius * s.radius;
    auto discriminant = half_b * half_b - a * c;

    if (discriminant < 0)
    {
        return false;
    }
    auto sqrtd = std::sqrt(discriminant);

    auto root = (-half_b - sqrtd) / a;
    if (root <= t_min || t_max <= root)
    {
        root = (-half_b + sqrtd) / a;
        if (root <= t_min || t_max <= root)
        {
            return false;
        }
    }

    rec.t = root;
    rec.point = r.at(rec.t);
    vec3<float> outward_normal = (rec.point - s.center) / s.radius;
    rec.set_face_normal(r, outward_normal);
    rec.material = s.material;
    return true;
}

bool find_nearest_hit(const ray3<float> &r, const std::vector<Sphere> &world, float t_min, float t_max, HitRecord &rec)
{
    HitRecord temp_rec;
    bool hit_anything = false;
    auto closest_so_far = t_max;

    for (const auto &sphere_obj : world)
    {
        if (hit_sphere(r, sphere_obj, t_min, closest_so_far, temp_rec))
        {
            hit_anything = true;
            closest_so_far = temp_rec.t;
            rec = temp_rec;
        }
    }
    return hit_anything;
}

vec3<float> color_for_ray_multisphere(const ray3<float> &r, const std::vector<Sphere> &world)
{
    HitRecord rec;
    if (find_nearest_hit(r, world, 0.001f, std::numeric_limits<float>::infinity(), rec))
    {
        return (rec.normal + vec3<float>(1.0f, 1.0f, 1.0f)) * 0.5f;
    }

    vec3<float> unit_direction = r.direction().normalized();
    auto t_bg = 0.5f * (unit_direction.y() + 1.0f);
    return vec3<float>(1.0f - t_bg) * vec3<float>(1.0f, 1.0f, 1.0f) + t_bg * vec3<float>(0.5f, 0.7f, 1.0f);
}

struct PointLight
{
    vec3<float> position;
    vec3<float> intensity;
    float att_c = 1.0f;
    float att_l = 0.0f;
    float att_q = 0.0f;
};

vec3<float> color_for_ray_shadows(const ray3<float> &r, const std::vector<Sphere> &world, const std::vector<PointLight> &lights)
{
    HitRecord rec;
    if (find_nearest_hit(r, world, 0.001f, std::numeric_limits<float>::infinity(), rec))
    {
        vec3<float> final_color(0.0f, 0.0f, 0.0f);
        vec3<float> ambient_color = rec.material.albedo * 0.1f;
        final_color += ambient_color;

        for (const auto &light : lights)
        {
            vec3<float> light_vec = light.position - rec.point;
            float light_distance = light_vec.length();
            vec3<float> light_dir = light_vec.normalized();

            ray3<float> shadow_ray(rec.point + rec.normal * 0.0001f, light_dir);
            HitRecord shadow_rec;

            bool in_shadow = false;
            if (find_nearest_hit(shadow_ray, world, 0.001f, light_distance, shadow_rec))
            {
                in_shadow = true;
            }

            if (!in_shadow)
            {
                float attenuation = 1.0f / (light.att_c + light.att_l * light_distance + light.att_q * light_distance * light_distance);
                attenuation = std::clamp(attenuation, 0.0f, 1.0f);

                float diffuse_factor = std::max(0.0f, rec.normal.dot(light_dir));
                vec3<float> diffuse_color = rec.material.albedo * light.intensity * diffuse_factor * attenuation;
                final_color += diffuse_color;
            }
        }
        return final_color.clamp(0.0f, 1.0f);
    }

    vec3<float> unit_direction = r.direction().normalized();
    auto t_bg = 0.5f * (unit_direction.y() + 1.0f);
    return vec3<float>(1.0f - t_bg) * vec3<float>(1.0f, 1.0f, 1.0f) + t_bg * vec3<float>(0.5f, 0.7f, 1.0f);
}

vec3<float> reflect(const vec3<float> &v, const vec3<float> &n)
{
    return v - n * 2.0f * v.dot(n);
}

bool refract(const vec3<float> &incident_v, const vec3<float> &normal_at_surface, float n_ratio_etai_over_etat, vec3<float> &refracted_dir)
{

    float cos_theta_i = (-incident_v).dot(normal_at_surface);
    cos_theta_i = std::clamp(cos_theta_i, -1.0f, 1.0f);

    float sin2_theta_i = 1.0f - cos_theta_i * cos_theta_i;
    float sin2_theta_t = n_ratio_etai_over_etat * n_ratio_etai_over_etat * sin2_theta_i;

    if (sin2_theta_t > 1.0f)
    {
        return false;
    }

    float cos_theta_t = std::sqrt(1.0f - sin2_theta_t);

    refracted_dir = incident_v * n_ratio_etai_over_etat + normal_at_surface * (n_ratio_etai_over_etat * cos_theta_i - cos_theta_t);
    return true;
}

float schlick_reflectance(float cosine, float ref_idx_ratio)
{
    auto r0 = (1.0f - ref_idx_ratio) / (1.0f + ref_idx_ratio);
    r0 = r0 * r0;
    return r0 + (1.0f - r0) * std::pow((1.0f - cosine), 5.0f);
}

const int MAX_RECURSION_DEPTH = 5;
const float SHADOW_RAY_T_MIN = 0.001f;

vec3<float> color_for_ray_recursive(const ray3<float> &r, const std::vector<Sphere> &world, const std::vector<PointLight> &lights, int depth);

// Removed get_shadow_attenuation as it's no longer used.

vec3<float> color_for_ray_recursive(const ray3<float> &r, const std::vector<Sphere> &world, const std::vector<PointLight> &lights, int depth)
{
    if (depth <= 0)
    {
        return vec3<float>(0.0f, 0.0f, 0.0f);
    }

    HitRecord rec;
    if (find_nearest_hit(r, world, SHADOW_RAY_T_MIN, std::numeric_limits<float>::infinity(), rec))
    {
        vec3<float> emitted_color(0.0f, 0.0f, 0.0f);
        vec3<float> scattered_color(0.0f, 0.0f, 0.0f);

        if (rec.material.transparency > 0.0f)
        {
            vec3<float> refraction_color(0.0f, 0.0f, 0.0f);
            vec3<float> reflection_color(0.0f, 0.0f, 0.0f);
            float kr;

            float eta_i_val;
            float eta_t_val;
            vec3<float> surface_normal = rec.normal;

            if (rec.front_face)
            {
                eta_i_val = 1.0f;
                eta_t_val = rec.material.refractive_index;
            }
            else
            {
                eta_i_val = rec.material.refractive_index;
                eta_t_val = 1.0f;
            }

            float n_ratio = eta_i_val / eta_t_val;
            vec3<float> unit_incident_dir = r.direction().normalized();
            float cos_theta_i = std::clamp((-unit_incident_dir).dot(surface_normal), -1.0f, 1.0f);

            kr = schlick_reflectance(std::abs(cos_theta_i), n_ratio);

            vec3<float> reflected_dir = reflect(unit_incident_dir, surface_normal);
            ray3<float> reflected_ray(rec.point + surface_normal * SHADOW_RAY_T_MIN, reflected_dir.normalized());
            reflection_color = color_for_ray_recursive(reflected_ray, world, lights, depth - 1);

            vec3<float> refracted_dir;
            if (refract(unit_incident_dir, surface_normal, n_ratio, refracted_dir))
            {
                ray3<float> refracted_ray(rec.point - surface_normal * SHADOW_RAY_T_MIN, refracted_dir.normalized());
                refraction_color = color_for_ray_recursive(refracted_ray, world, lights, depth - 1);
            }
            else
            {
                kr = 1.0f;
            }

            scattered_color = reflection_color * kr + refraction_color * (1.0f - kr) * rec.material.albedo;
            return scattered_color.clamp(0.0f, 1.0f);
        }
        else
        {
            vec3<float> local_illumination(0.0f, 0.0f, 0.0f);
            vec3<float> ambient = rec.material.albedo * 0.1f;
            local_illumination += ambient;
            vec3<float> view_dir = (r.origin() - rec.point).normalized();

            for (const auto &light : lights)
            {
                vec3<float> light_vec = light.position - rec.point;
                float light_distance = light_vec.length();
                vec3<float> light_dir = light_vec.normalized();

                vec3<float> shadow_attenuation_factor(1.0f, 1.0f, 1.0f);

                ray3<float> shadow_ray_obj(rec.point + rec.normal * SHADOW_RAY_T_MIN, light_dir);
                HitRecord shadow_hit_rec;
                if (find_nearest_hit(shadow_ray_obj, world, SHADOW_RAY_T_MIN, light_distance, shadow_hit_rec))
                {
                    shadow_attenuation_factor = vec3<float>(0.0f, 0.0f, 0.0f);
                }

                if (shadow_attenuation_factor.x() > 0.0f || shadow_attenuation_factor.y() > 0.0f || shadow_attenuation_factor.z() > 0.0f)
                {
                    float light_dist_attenuation = 1.0f / (light.att_c + light.att_l * light_distance + light.att_q * light_distance * light_distance);
                    light_dist_attenuation = std::clamp(light_dist_attenuation, 0.0f, 1.0f);

                    vec3<float> effective_light_intensity = light.intensity * light_dist_attenuation * shadow_attenuation_factor;

                    float diff = std::max(0.0f, rec.normal.dot(light_dir));
                    local_illumination += rec.material.albedo * effective_light_intensity * diff * rec.material.diffuse_k;

                    vec3<float> halfway_dir = (light_dir + view_dir).normalized();
                    float spec = std::pow(std::max(0.0f, rec.normal.dot(halfway_dir)), rec.material.shininess);
                    local_illumination += vec3<float>(1.0f, 1.0f, 1.0f) * effective_light_intensity * spec * rec.material.specular_k;
                }
            }

            vec3<float> reflected_contribution(0.0f, 0.0f, 0.0f);
            if (rec.material.reflectivity > 0.0f)
            {
                vec3<float> reflection_ray_dir = reflect(r.direction().normalized(), rec.normal);
                ray3<float> reflection_ray(rec.point + rec.normal * SHADOW_RAY_T_MIN, reflection_ray_dir);
                reflected_contribution = color_for_ray_recursive(reflection_ray, world, lights, depth - 1) * rec.material.reflectivity;
            }
            scattered_color = local_illumination * (1.0f - rec.material.reflectivity) + reflected_contribution;
            return (emitted_color + scattered_color).clamp(0.0f, 1.0f);
        }
    }

    vec3<float> unit_direction = r.direction().normalized();
    auto t_bg = 0.5f * (unit_direction.y() + 1.0f);
    return vec3<float>(1.0f - t_bg) * vec3<float>(1.0f, 1.0f, 1.0f) + t_bg * vec3<float>(0.5f, 0.7f, 1.0f);
}

vec3<float> trace_ray(const ray3<float> &r, const std::vector<Sphere> &world, const std::vector<PointLight> &lights, int depth)
{
    return color_for_ray_recursive(r, world, lights, depth);
}

void render_scene(
    const std::string &output_filename,
    const std::vector<Sphere> &world,
    const std::function<vec3<float>(const ray3<float> &, const std::vector<Sphere> &)> &color_func)
{
    using namespace std;
    vector<vec3<float>> colors_float(WIDTH * HEIGHT);
    std::mt19937 gen(SEED);
    std::uniform_real_distribution<float> distrib(0.0f, 1.0f);

    auto viewport_height = 2.0f;
    auto viewport_width = ASPECT_RATIO * viewport_height;
    auto focal_length = 1.0f;

    vec3<float> cam_origin(0.0f, 0.0f, 0.0f);
    vec3<float> horizontal(viewport_width, 0.0f, 0.0f);
    vec3<float> vertical(0.0f, viewport_height, 0.0f);
    vec3<float> lower_left_corner = cam_origin - horizontal / 2.0f - vertical / 2.0f - vec3<float>(0.0f, 0.0f, focal_length);

    for (uint32_t j = 0; j < HEIGHT; ++j)
    {
        for (uint32_t i = 0; i < WIDTH; ++i)
        {
            vec3<float> pixel_color(0.0f, 0.0f, 0.0f);
            for (int s = 0; s < SAMPLES_PER_PIXEL; ++s)
            {
                float u_sample = (static_cast<float>(i) + distrib(gen)) / (WIDTH - 1);
                float v_sample = (static_cast<float>(HEIGHT - 1 - j) + distrib(gen)) / (HEIGHT - 1);

                vec3<float> ray_target_on_viewport = lower_left_corner + u_sample * horizontal + v_sample * vertical;
                ray3<float> r_sample(cam_origin, ray_target_on_viewport - cam_origin);
                pixel_color += color_func(r_sample, world);
            }
            colors_float[j * WIDTH + i] = pixel_color / static_cast<float>(SAMPLES_PER_PIXEL);
        }
    }

    vector<vec3<uint8_t>> colors_quantized = convert_vec3_float_to_uint8_many(colors_float, 0.0f, 1.0f);
    encode_ppm_p3(WIDTH, HEIGHT, colors_quantized, output_filename);
}

void render_scene(
    const std::string &output_filename,
    const std::vector<Sphere> &world,
    const std::vector<PointLight> &lights,
    const std::function<vec3<float>(const ray3<float> &, const std::vector<Sphere> &, const std::vector<PointLight> &, int)> &color_func_recursive)
{
    using namespace std;
    vector<vec3<float>> colors_float(WIDTH * HEIGHT);
    std::mt19937 gen(SEED);
    std::uniform_real_distribution<float> distrib(0.0f, 1.0f);

    auto viewport_height = 2.0f;
    auto viewport_width = ASPECT_RATIO * viewport_height;
    auto focal_length = 1.0f;

    vec3<float> cam_origin(0.0f, 0.0f, 0.0f);
    vec3<float> horizontal(viewport_width, 0.0f, 0.0f);
    vec3<float> vertical(0.0f, viewport_height, 0.0f);
    vec3<float> lower_left_corner = cam_origin - horizontal / 2.0f - vertical / 2.0f - vec3<float>(0.0f, 0.0f, focal_length);

    for (uint32_t j = 0; j < HEIGHT; ++j)
    {
        for (uint32_t i = 0; i < WIDTH; ++i)
        {
            vec3<float> pixel_color(0.0f, 0.0f, 0.0f);
            for (int s = 0; s < SAMPLES_PER_PIXEL; ++s)
            {
                float u_sample = (static_cast<float>(i) + distrib(gen)) / (WIDTH - 1);
                float v_sample = (static_cast<float>(HEIGHT - 1 - j) + distrib(gen)) / (HEIGHT - 1);

                vec3<float> ray_target_on_viewport = lower_left_corner + u_sample * horizontal + v_sample * vertical;
                ray3<float> r_sample(cam_origin, ray_target_on_viewport - cam_origin);
                pixel_color += color_func_recursive(r_sample, world, lights, MAX_RECURSION_DEPTH);
            }
            colors_float[j * WIDTH + i] = pixel_color / static_cast<float>(SAMPLES_PER_PIXEL);
        }
    }

    vector<vec3<uint8_t>> colors_quantized = convert_vec3_float_to_uint8_many(colors_float, 0.0f, 1.0f);
    encode_ppm_p3(WIDTH, HEIGHT, colors_quantized, output_filename);
}

int main()
{
    using namespace std;
    namespace fs = std::filesystem;

    if (!fs::exists("outputs"))
    {
        fs::create_directory("outputs");
    }

    std::vector<Sphere> world_spheres;
    world_spheres.push_back({{0.0f, -100.5f, -1.0f}, 100.0f, {{0.5f, 0.5f, 0.5f}}});
    world_spheres.push_back({{0.0f, 0.0f, -1.0f}, 0.5f, {{0.8f, 0.3f, 0.3f}}});
    world_spheres.push_back({{-1.0f, 0.0f, -1.0f}, 0.5f, {{0.3f, 0.8f, 0.3f}}});
    world_spheres.push_back({{1.0f, 0.0f, -1.0f}, 0.5f, {{0.3f, 0.3f, 0.8f}}});

    world_spheres.push_back({{0.0f, -0.3f, -0.4f}, 0.1f, {{0.9f, 0.7f, 0.1f}}});
    world_spheres.push_back({{0.2f, -0.35f, -0.5f}, 0.1f, {{0.1f, 0.9f, 0.9f}}});
    world_spheres.push_back({{-0.2f, -0.35f, -0.5f}, 0.1f, {{0.9f, 0.1f, 0.9f}}});
    world_spheres.push_back({{0.4f, -0.25f, -0.6f}, 0.1f, {{0.5f, 0.5f, 0.9f}}});
    world_spheres.push_back({{-0.4f, -0.25f, -0.6f}, 0.1f, {{0.9f, 0.5f, 0.5f}}});
    world_spheres.push_back({{0.0f, -0.15f, -0.3f}, 0.1f, {{0.5f, 0.9f, 0.5f}}});

    auto multisphere_adapter =
        [&](const ray3<float> &r_in, const std::vector<Sphere> &w_in)
    {
        return color_for_ray_multisphere(r_in, w_in);
    };
    render_scene("outputs/1_multisphere.ppm", world_spheres, multisphere_adapter);

    std::vector<PointLight> lights;
    lights.push_back({{-5.0f, 5.0f, -0.5f}, {1.5f, 1.5f, 1.5f}, 1.0f, 0.09f, 0.032f});
    lights.push_back({{5.0f, 2.0f, 1.0f}, {1.0f, 1.0f, 1.4f}, 1.0f, 0.045f, 0.0075f});

    auto shadows_adapter =
        [&](const ray3<float> &r_in, const std::vector<Sphere> &w_in, const std::vector<PointLight> &l_in, int)
    {
        return color_for_ray_shadows(r_in, w_in, l_in);
    };
    render_scene("outputs/2_shadow.ppm", world_spheres, lights, shadows_adapter);

    std::vector<Sphere> world_spheres_rt = world_spheres;

    world_spheres_rt[1].material.reflectivity = 0.6f;
    world_spheres_rt[1].material.albedo = {0.1f, 0.1f, 0.1f};
    world_spheres_rt[2].material.reflectivity = 0.2f;
    world_spheres_rt[0].material.albedo = {0.8f, 0.8f, 0.2f};

    auto trace_ray_adapter =
        [&](const ray3<float> &r_in, const std::vector<Sphere> &w_in, const std::vector<PointLight> &l_in, int d)
    {
        return trace_ray(r_in, w_in, l_in, d);
    };

    render_scene("outputs/3_reflection.ppm", world_spheres_rt, lights, trace_ray_adapter);

    std::vector<Sphere> world_spheres_transmission = world_spheres_rt;
    world_spheres_transmission[1].material.albedo = {0.9f, 0.9f, 0.95f};
    world_spheres_transmission[1].material.reflectivity = 0.0f;
    world_spheres_transmission[1].material.transparency = 1.0f;
    world_spheres_transmission[1].material.refractive_index = 1.5f;
    world_spheres_transmission[1].material.diffuse_k = 0.1f;
    world_spheres_transmission[1].material.specular_k = 0.8f;

    world_spheres_transmission[3].material.albedo = {0.95f, 0.9f, 0.9f};
    world_spheres_transmission[3].material.reflectivity = 0.0f;
    world_spheres_transmission[3].material.transparency = 1.0f;
    world_spheres_transmission[3].material.refractive_index = 1.3f;
    world_spheres_transmission[3].material.diffuse_k = 0.1f;
    world_spheres_transmission[3].material.specular_k = 0.7f;

    render_scene("outputs/4_transmission.ppm", world_spheres_transmission, lights, trace_ray_adapter);

    return 0;
}
