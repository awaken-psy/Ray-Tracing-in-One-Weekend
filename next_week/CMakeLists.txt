cmake_minimum_required(VERSION 3.10)
project(RayTracing)

set(CMAKE_CXX_STANDARD 17)

add_executable(ray_tracing main.cpp)
target_compile_options(ray_tracing PRIVATE -fopenmp)
target_link_libraries(ray_tracing "E:/tools/scoop/apps/llvm/22.1.1/lib/libomp.lib")
