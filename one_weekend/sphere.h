#pragma once
#include "hittable.h"
#include "vec3.h"

class sphere : public hittable {
public:
    sphere() {}
    sphere(vec3 cen, double r, shared_ptr<material> m) : center(cen), radius(r), mat_ptr(m) {};

    virtual bool hit(const ray &r, double tmin, double tmax, hit_record &rec) const;

public:
    vec3 center;
    double radius;
    shared_ptr<material> mat_ptr;
};

bool sphere::hit(const ray &r, double t_min, double t_max, hit_record &rec) const {
    vec3 oc = r.origin() - center;
    auto a = r.direction().length_squared();
    auto half_b = dot(oc, r.direction());
    auto c = oc.length_squared() - radius * radius;
    auto discriminant = half_b * half_b - a * c;

    // 判别式 > 0 表示光线与球体有交点
    if (discriminant > 0) {
        auto root = sqrt(discriminant);

        // 计算第一个交点（较近的交点，使用 -sqrt）
        // 这是二次方程 at² + 2bt + c = 0 的解：t = (-b - √Δ) / a
        auto temp = (-half_b - root) / a;
        // 检查交点是否在有效范围 [t_min, t_max] 内
        if (temp < t_max && temp > t_min) {
            rec.t = temp;                                    // 记录光线参数 t
            rec.p = r.at(rec.t);                             // 计算交点的世界坐标
            vec3 outward_normal = (rec.p - center) / radius; // 计算交点处的单位法向量
            rec.set_face_normal(r, outward_normal);
            rec.mat_ptr = mat_ptr;
            return true;
        }

        // 如果第一个交点不在范围内，检查第二个交点（较远的交点，使用 +sqrt）
        temp = (-half_b + root) / a;
        if (temp < t_max && temp > t_min) {
            rec.t = temp;
            rec.p = r.at(rec.t);
            vec3 outward_normal = (rec.p - center) / radius;
            rec.set_face_normal(r, outward_normal);
            rec.mat_ptr = mat_ptr;
            return true;
        }
    }
    return false;
}
