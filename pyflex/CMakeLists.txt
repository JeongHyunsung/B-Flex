cmake_minimum_required(VERSION 3.16)
project(pyflex LANGUAGES CXX)

find_package(pybind11 REQUIRED)

pybind11_add_module(pyflex bindings.cpp)

target_link_libraries(pyflex PRIVATE flex)

target_include_directories(pyflex PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

install(TARGETS pyflex LIBRARY DESTINATION .)
