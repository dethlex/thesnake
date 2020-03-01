cmake_minimum_required(VERSION 3.15)
project(Snake)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "/usr/share/SFML/cmake/Modules/")

find_package(SFML 2 COMPONENTS graphics window system audio REQUIRED)

add_executable(Snake main.cpp)

target_link_libraries(Snake sfml-graphics sfml-window sfml-system sfml-audio)