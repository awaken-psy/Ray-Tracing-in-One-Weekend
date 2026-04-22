#pragma once
#include "hittable.h"
#include <memory>
#include <vector>

using std::make_shared;
using std::shared_ptr;

class hittable_list : public hittable {
public:
    hittable_list() {}
    hittable_list(shared_ptr<hittable> object) { add(object); }

    void clear() { objects.clear(); }
    void add(shared_ptr<hittable> object) { objects.push_back(object); }

    virtual bool hit(const ray &r, double tmin, double tmax, hit_record &rec) const;

public:
    std::vector<shared_ptr<hittable>> objects;
};

// 检测光线是否与场景中的任何物体相交
bool hittable_list::hit(const ray &r, double t_min, double t_max, hit_record &rec) const {
    hit_record temp_rec;              // 临时碰撞记录，用于存储每次检测的结果
    bool hit_anything = false;        
    auto closest_so_far = t_max;      // 记录目前找到的最近碰撞点的 t 值

    for (const auto &object : objects) {
        // 注意：这里用 closest_so_far 而不是 t_max，确保只记录最近的碰撞
        if (object->hit(r, t_min, closest_so_far, temp_rec)) {
            hit_anything = true;           
            closest_so_far = temp_rec.t;   // 更新最近碰撞点的 t 值
            rec = temp_rec;                
        }
    }

    return hit_anything;  // 返回是否发生了碰撞
}