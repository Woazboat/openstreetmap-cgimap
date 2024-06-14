cmake_minimum_required(VERSION 3.14)

project(cgimap_foo_plugin
        LANGUAGES CXX)

add_library(cgimap_foo_plugin SHARED)

target_sources(cgimap_foo_plugin 
    PRIVATE
        foo.cpp)

target_link_libraries(cgimap_foo_plugin PRIVATE plugin_interface)
