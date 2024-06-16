add_library(plugin_interface INTERFACE)

# target_include_directories(plugin_interface INTERFACE 
#     # "${CMAKE_CURRENT_SOURCE_DIR}"
#     ../include/cgimap/plugins
#     )

target_link_libraries(plugin_interface INTERFACE 
    cgimap_core
    cgimap_fcgi
    cgimap_apidb)

add_subdirectory(foo)
