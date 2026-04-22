#pragma once

#include "hittable.h"
#include "ray.h"
#include "vec3.h"

// 材质基类：定义光线与物体表面交互的行为
class material {
public:
    // 散射函数：计算光线击中表面后的散射行为
    // 参数：
    //   r_in: 入射光线（从光源或相机射向物体表面的光线）
    //   rec: 碰撞记录（包含碰撞点位置、法线、材质指针等信息）
    //   attenuation: 衰减系数（输出参数，表示光线被吸收后剩余的颜色强度，范围 0-1）
    //   scattered: 散射光线（输出参数，表示光线从表面反射/折射后的新方向）
    // 返回值：true 表示光线发生散射，false 表示光线被完全吸收
    virtual bool scatter(const ray &r_in, const hit_record &rec, vec3 &attenuation, ray &scattered) const = 0;
};

// 朗伯材质（漫反射材质）：模拟粗糙表面的漫反射效果
// 光线击中表面后会向各个方向均匀散射，符合朗伯余弦定律
class lambertian : public material {
public:
    lambertian(const vec3 &a) : albedo(a) {}

    // 散射函数：实现朗伯漫反射
    // 参数：
    //   r_in: 入射光线（未使用，因为漫反射不依赖入射角度）
    //   rec: 碰撞记录，包含碰撞点 rec.p 和表面法线 rec.normal
    //   attenuation: 输出参数，设置为材质的反照率（决定反射光的颜色）
    //   scattered: 输出参数，生成的散射光线（从碰撞点出发，方向为法线+随机单位向量）
    // 返回值：总是返回 true，表示光线一定会发生散射
    virtual bool scatter(const ray &r_in, const hit_record &rec, vec3 &attenuation, ray &scattered) const {
        // 计算散射方向：法线方向 + 随机单位向量（实现半球面均匀散射）
        vec3 scatter_direction = rec.normal + random_unit_vector();
        // 生成散射光线：从碰撞点出发，沿散射方向传播
        scattered = ray(rec.p, scatter_direction);
        // 设置衰减系数为材质的反照率（光线颜色会乘以这个值）
        attenuation = albedo;
        return true;
    }

public:
    vec3 albedo;  // 反照率：材质的固有颜色（RGB 值，范围 0-1）
};

// 金属材质：模拟光滑金属表面的镜面反射效果
// 光线击中表面后会按照反射定律进行完美镜面反射
class metal : public material {
public:
    metal(const vec3 &a, double f) : albedo(a), fuzz(f < 1 ? f : 1) {}

    virtual bool scatter(const ray &r_in, const hit_record &rec, vec3 &attenuation, ray &scattered) const {
        // 计算反射方向：使用反射定律 R = V - 2(V·N)N
        // 先将入射方向归一化，然后根据法线计算反射向量
        vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
        // 生成反射光线：从碰撞点出发，沿反射方向传播
        scattered = ray(rec.p, reflected + fuzz * random_in_unit_sphere());
        // 设置衰减系数为金属的反照率（反射光会带上金属的颜色）
        attenuation = albedo;
        // 检查反射光线是否在表面上方：点积 > 0 表示反射方向与法线夹角 < 90°
        // 如果反射光线指向表面下方，说明发生了异常，应该吸收光线
        return (dot(scattered.direction(), rec.normal) > 0);
    }

public:
    vec3 albedo;  // 反照率：金属的颜色（RGB 值，范围 0-1）
    double fuzz;  // 粗糙度：控制反射方向的偏移量，范围 0-1
};

// 电介质材质：模拟玻璃、水等透明材料的折射和反射效果
// 光线击中表面后会根据 Snell 定律发生折射，或在临界角时发生全反射
class dielectric : public material {
public:
    dielectric(double ri) : ref_idx(ri) {}

    virtual bool scatter(const ray &r_in, const hit_record &rec, vec3 &attenuation, ray &scattered) const {
        // 电介质不吸收光线，保持原色
        attenuation = vec3(1.0, 1.0, 1.0);
        // 计算折射率比：从空气进入介质时为 1.0/ref_idx，从介质射出时为 ref_idx
        double etai_over_etat = (rec.front_face) ? (1.0 / ref_idx) : (ref_idx);

        vec3 unit_direction = unit_vector(r_in.direction());
        double cos_theta = ffmin(dot(-unit_direction, rec.normal), 1.0);
        double sin_theta = sqrt(1.0 - cos_theta * cos_theta);
        // 判断是否发生全反射：根据 Snell 定律，当 etai_over_etat * sin_theta > 1.0 时
        // 折射角的正弦值会大于 1，此时无法发生折射，只能发生全反射
        if (etai_over_etat * sin_theta > 1.0) {
            vec3 reflected = reflect(unit_direction, rec.normal);
            scattered = ray(rec.p, reflected);
            return true;
        }

        // 使用 Schlick 近似计算菲涅尔反射概率
        // 即使没有全反射，电介质表面也会有部分光线反射（如玻璃表面的镜面反射）
        // 反射概率随入射角变化：垂直入射时反射少，掠射时反射多
        double reflect_prob = schlick(cos_theta, etai_over_etat);
        // 根据反射概率随机决定是反射还是折射，模拟真实的菲涅尔效应
        if (random_double() < reflect_prob) {
            vec3 reflected = reflect(unit_direction, rec.normal);
            scattered = ray(rec.p, reflected);
            return true;
        }

        // 应用 Snell 定律计算折射光线方向
        vec3 refracted = refract(unit_direction, rec.normal, etai_over_etat);
        scattered = ray(rec.p, refracted);
        return true;
    }

public:
    double ref_idx;  // 折射率：材质的折射率（如玻璃约为 1.5，水约为 1.33）
};