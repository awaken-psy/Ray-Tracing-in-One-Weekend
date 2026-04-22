#include "camera.h"
#include "hittable_list.h"
#include "material.h"
#include "rtweekend.h"
#include "sphere.h"
#include <atomic>
#include <fstream>
#include <iostream>
#include <omp.h>
#include <vector>

vec3 ray_color(const ray &r, const hittable &world, int depth) {
    hit_record rec;

    // If we've exceeded the ray bounce limit, no more light is gathered.
    if (depth <= 0)
        return vec3(0, 0, 0);

    if (world.hit(r, 0.001, infinity, rec)) {
        ray scattered;
        vec3 attenuation;
        if (rec.mat_ptr->scatter(r, rec, attenuation, scattered))
            return attenuation * ray_color(scattered, world, depth - 1);
        return vec3(0, 0, 0);
    }

    vec3 unit_direction = unit_vector(r.direction());
    auto t = 0.5 * (unit_direction.y() + 1.0);
    return (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);
}

hittable_list random_scene() {
    hittable_list world;

    world.add(make_shared<sphere>(vec3(0, -1000, 0), 1000, make_shared<lambertian>(vec3(0.5, 0.5, 0.5))));

    int i = 1;
    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = random_double();
            vec3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double());
            if ((center - vec3(4, 0.2, 0)).length() > 0.9) {
                if (choose_mat < 0.8) {
                    // diffuse
                    auto albedo = vec3::random() * vec3::random();
                    world.add(make_shared<sphere>(center, 0.2, make_shared<lambertian>(albedo)));
                } else if (choose_mat < 0.95) {
                    // metal
                    auto albedo = vec3::random(.5, 1);
                    auto fuzz = random_double(0, .5);
                    world.add(make_shared<sphere>(center, 0.2, make_shared<metal>(albedo, fuzz)));
                } else {
                    // glass
                    world.add(make_shared<sphere>(center, 0.2, make_shared<dielectric>(1.5)));
                }
            }
        }
    }

    world.add(make_shared<sphere>(vec3(0, 1, 0), 1.0, make_shared<dielectric>(1.5)));

    world.add(make_shared<sphere>(vec3(-4, 1, 0), 1.0, make_shared<lambertian>(vec3(0.4, 0.2, 0.1))));

    world.add(make_shared<sphere>(vec3(4, 1, 0), 1.0, make_shared<metal>(vec3(0.7, 0.6, 0.5), 0.0)));

    return world;
}

int main() {
    const int image_width = 1200;
    const int image_height = 800;
    const int samples_per_pixel = 50;
    const int max_depth = 10;

    std::ofstream file("image.ppm");
    std::streambuf *old_buf = std::cout.rdbuf(file.rdbuf());

    std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    // object
    hittable_list world = random_scene();

    // camera
    const auto aspect_ratio = double(image_width) / image_height;
    vec3 lookfrom(5, 5, 4);
    vec3 lookat(0, 0, -1);
    vec3 vup(0, 1, 0);
    auto dist_to_focus = (lookfrom - lookat).length();
    auto aperture = 0.1;
    camera cam(lookfrom, lookat, vup, 40, aspect_ratio, aperture, dist_to_focus);

    // 多线程
    const int num_threads = 16;                                // 修改此处指定核心数
    omp_set_num_threads(num_threads);                          // 设置 OpenMP 使用的线程数
    std::vector<vec3> framebuffer(image_width * image_height); // 帧缓冲区，存储所有像素颜色（多线程写入，按行索引）
    std::atomic<int> done(0);                                  // 原子计数器，线程安全地统计已完成的扫描行数

// OpenMP 并行 for 循环，动态调度（每次分配 1 行给空闲线程）
#pragma omp parallel for schedule(dynamic, 1)
    for (int j = image_height - 1; j >= 0; --j) {
        for (int i = 0; i < image_width; ++i) {
            vec3 color(0, 0, 0);
            for (int s = 0; s < samples_per_pixel; ++s) {
                auto u = (i + random_double()) / image_width;
                auto v = (j + random_double()) / image_height;
                ray r = cam.get_ray(u, v);
                color += ray_color(r, world, max_depth);
            }
            framebuffer[(image_height - 1 - j) * image_width + i] = color; // 写入帧缓冲区（各行独立，无竞争）
        }
        std::cerr << "\rScanlines done: " << ++done << '/' << image_height << ' '
                  << std::flush; // 原子自增，线程安全进度输出
    }

    // 主线程串行输出，帧缓冲区已由并行阶段填充完毕
    for (auto &c : framebuffer)
        c.write_color(std::cout, samples_per_pixel);

    std::cerr << "\nDone.\n";
    std::cout.rdbuf(old_buf);
}