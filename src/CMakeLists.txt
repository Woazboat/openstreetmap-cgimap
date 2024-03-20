#############
# cgimap_core
#############
add_library(cgimap_core)

target_sources(cgimap_core PRIVATE
    backend.cpp
    backend.cpp
    bbox.cpp
    brotli.cpp
    choose_formatter.cpp
    handler.cpp
    http.cpp
    logger.cpp
    mime_types.cpp
    basicauth.cpp
    oauth.cpp
    oauth2.cpp
    options.cpp
    osm_responder.cpp
    osm_current_responder.cpp
    osm_diffresult_responder.cpp
    osmchange_responder.cpp
    output_formatter.cpp
    process_request.cpp
    rate_limiter.cpp
    request.cpp
    request_helpers.cpp
    router.cpp
    routes.cpp
    text_formatter.cpp
    text_responder.cpp
    text_writer.cpp
    time.cpp
    xml_formatter.cpp
    xml_writer.cpp
    zlib.cpp

    api06/changeset_close_handler.cpp
    api06/changeset_create_handler.cpp
    api06/changeset_download_handler.cpp
    api06/changeset_handler.cpp
    api06/changeset_update_handler.cpp
    api06/changeset_upload_handler.cpp
    api06/handler_utils.cpp
    api06/map_handler.cpp
    api06/node_handler.cpp
    api06/node_history_handler.cpp
    api06/node_relations_handler.cpp
    api06/nodes_handler.cpp
    api06/node_version_handler.cpp
    api06/node_ways_handler.cpp
    api06/relation_full_handler.cpp
    api06/relation_handler.cpp
    api06/relation_history_handler.cpp
    api06/relation_relations_handler.cpp
    api06/relations_handler.cpp
    api06/relation_version_handler.cpp
    api06/way_full_handler.cpp
    api06/way_handler.cpp
    api06/way_history_handler.cpp
    api06/way_relations_handler.cpp
    api06/ways_handler.cpp
    api06/way_version_handler.cpp

    api06/changeset_upload/osmchange_handler.cpp
    api06/changeset_upload/osmchange_tracking.cpp

    $<$<BOOL:${ENABLE_API07}>:api07/map_handler.cpp>

    $<$<BOOL:${ENABLE_YAJL}>:json_formatter.cpp>
    $<$<BOOL:${ENABLE_YAJL}>:json_writer.cpp>
)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.23.0)
    target_sources(cgimap_core PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ../include
        FILES
            ../include/cgimap/api06/changeset_upload/changeset_input_format.hpp
            ../include/cgimap/api06/changeset_upload/changeset_updater.hpp
            ../include/cgimap/api06/changeset_upload/node_updater.hpp
            ../include/cgimap/api06/changeset_upload/node.hpp
            ../include/cgimap/api06/changeset_upload/osmchange_handler.hpp
            ../include/cgimap/api06/changeset_upload/osmchange_input_format.hpp
            ../include/cgimap/api06/changeset_upload/osmchange_tracking.hpp
            ../include/cgimap/api06/changeset_upload/osmobject.hpp
            ../include/cgimap/api06/changeset_upload/parser_callback.hpp
            ../include/cgimap/api06/changeset_upload/relation_updater.hpp
            ../include/cgimap/api06/changeset_upload/relation.hpp
            ../include/cgimap/api06/changeset_upload/way_updater.hpp
            ../include/cgimap/api06/changeset_upload/way.hpp

            ../include/cgimap/api06/changeset_close_handler.hpp
            ../include/cgimap/api06/changeset_create_handler.hpp
            ../include/cgimap/api06/changeset_download_handler.hpp
            ../include/cgimap/api06/changeset_handler.hpp
            ../include/cgimap/api06/changeset_update_handler.hpp
            ../include/cgimap/api06/changeset_upload_handler.hpp
            ../include/cgimap/api06/handler_utils.hpp
            ../include/cgimap/api06/id_version.hpp
            ../include/cgimap/api06/map_handler.hpp
            ../include/cgimap/api06/node_history_handler.hpp
            ../include/cgimap/api06/node_relations_handler.hpp
            ../include/cgimap/api06/node_version_handler.hpp
            ../include/cgimap/api06/node_ways_handler.hpp
            ../include/cgimap/api06/nodes_handler.hpp
            ../include/cgimap/api06/nodes_handler.hpp
            ../include/cgimap/api06/relation_full_handler.hpp
            ../include/cgimap/api06/relation_handler.hpp
            ../include/cgimap/api06/relation_history_handler.hpp
            ../include/cgimap/api06/relation_relations_handler.hpp
            ../include/cgimap/api06/relation_version_handler.hpp
            ../include/cgimap/api06/relations_handler.hpp
            ../include/cgimap/api06/way_full_handler.hpp
            ../include/cgimap/api06/way_handler.hpp
            ../include/cgimap/api06/way_history_handler.hpp
            ../include/cgimap/api06/way_relations_handler.hpp
            ../include/cgimap/api06/way_version_handler.hpp
            ../include/cgimap/api06/ways_handler.hpp

            $<$<BOOL:${ENABLE_API07}>:${CMAKE_CURRENT_SOURCE_DIR}/../include/cgimap/api07/map_handler.hpp>

            ../include/cgimap/backend.hpp
            ../include/cgimap/basicauth.hpp
            ../include/cgimap/bbox.hpp
            ../include/cgimap/choose_formatter.hpp
            ../include/cgimap/data_selection.hpp
            ../include/cgimap/data_update.hpp
            ../include/cgimap/fcgi_request.hpp
            ../include/cgimap/handler.hpp
            ../include/cgimap/http.hpp
            $<$<BOOL:${ENABLE_YAJL}>:${CMAKE_CURRENT_SOURCE_DIR}/../include/cgimap/json_formatter.hpp>
            $<$<BOOL:${ENABLE_YAJL}>:${CMAKE_CURRENT_SOURCE_DIR}/../include/cgimap/json_writer.hpp>
            ../include/cgimap/logger.hpp
            ../include/cgimap/mime_types.hpp
            ../include/cgimap/oauth_io.hpp
            ../include/cgimap/oauth.hpp
            ../include/cgimap/oauth2.hpp
            ../include/cgimap/options.hpp
            ../include/cgimap/osm_current_responder.hpp
            ../include/cgimap/osm_diffresult_responder.hpp
            ../include/cgimap/osm_responder.hpp
            ../include/cgimap/osmchange_responder.hpp
            ../include/cgimap/output_buffer.hpp
            ../include/cgimap/output_formatter.hpp
            ../include/cgimap/output_writer.hpp
            ../include/cgimap/process_request.hpp
            ../include/cgimap/rate_limiter.hpp
            ../include/cgimap/request_helpers.hpp
            ../include/cgimap/request.hpp
            ../include/cgimap/router.hpp
            ../include/cgimap/routes.hpp
            ../include/cgimap/text_formatter.hpp
            ../include/cgimap/text_responder.hpp
            ../include/cgimap/text_writer.hpp
            ../include/cgimap/time.hpp
            ../include/cgimap/types.hpp
            ../include/cgimap/util.hpp
            ../include/cgimap/xml_formatter.hpp
            ../include/cgimap/xml_writer.hpp
            ../include/cgimap/zlib.hpp
    )
else()
    target_include_directories(cgimap_core PUBLIC
        ../include)
endif()

target_link_libraries(cgimap_core
    cgimap_common_compiler_options
    cgimap_libxml++
    ZLIB::ZLIB
    CryptoPP::CryptoPP
    Libmemcached::Libmemcached
    Argon2::Argon2
    $<$<BOOL:${ENABLE_BROTLI}>:Brotli::common>
    $<$<BOOL:${ENABLE_BROTLI}>:Brotli::encoder>
    $<$<BOOL:${ENABLE_BROTLI}>:Brotli::decoder>
    $<$<BOOL:${ENABLE_YAJL}>:YAJL::YAJL>
    PQXX::PQXX)

#############
# cgimap_fcgi
#############
add_library(cgimap_fcgi)

target_sources(cgimap_fcgi PRIVATE
    fcgi_request.cpp)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.23.0)
    target_sources(cgimap_fcgi PUBLIC
            FILE_SET HEADERS
            BASE_DIRS ../include
            FILES
                ../include/cgimap/fcgi_request.hpp
    )
else()
    target_include_directories(cgimap_fcgi PUBLIC
        ../include)
endif()

target_link_libraries(cgimap_fcgi
    cgimap_common_compiler_options
    Fcgi::Fcgi)



add_subdirectory(backend/apidb)

