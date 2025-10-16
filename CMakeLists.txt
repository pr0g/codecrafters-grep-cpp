cmake_minimum_required(VERSION 3.13)

project(grep-starter-cpp)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)

add_executable(grep ${SOURCE_FILES})
target_compile_features(grep PRIVATE cxx_std_23)
