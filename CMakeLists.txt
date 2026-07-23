cmake_minimum_required(VERSION 3.20)
project(JustDid VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Qt 模块
find_package(Qt6 REQUIRED COMPONENTS Core Quick Network Sql HttpServer)

# yaml-cpp (FetchContent 自动获取)
include(FetchContent)
FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG        0.8.0
)
FetchContent_MakeAvailable(yaml-cpp)

add_subdirectory(src)
