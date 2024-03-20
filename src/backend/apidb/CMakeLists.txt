##############
# cgimap_apidb
##############
if(ENABLE_APIDB)
    add_library(cgimap_apidb)

    target_sources(cgimap_apidb PRIVATE
        apidb.cpp
        readonly_pgsql_selection.cpp
        common_pgsql_selection.cpp
        pgsql_update.cpp
        changeset.cpp
        quad_tile.cpp
        oauth_store.cpp
        transaction_manager.cpp
        utils.cpp
        changeset_upload/changeset_updater.cpp
        changeset_upload/node_updater.cpp
        changeset_upload/relation_updater.cpp
        changeset_upload/way_updater.cpp
    )

    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.23.0)
        target_sources(cgimap_apidb PUBLIC
            FILE_SET HEADERS
            BASE_DIRS ../../../include/
            FILES
                ../../../include/cgimap/backend/apidb/apidb.hpp
                ../../../include/cgimap/backend/apidb/changeset.hpp
                ../../../include/cgimap/backend/apidb/common_pgsql_selection.hpp
                ../../../include/cgimap/backend/apidb/oauth_store.hpp
                ../../../include/cgimap/backend/apidb/pgsql_update.hpp
                ../../../include/cgimap/backend/apidb/pqxx_string_traits.hpp
                ../../../include/cgimap/backend/apidb/quad_tile.hpp
                ../../../include/cgimap/backend/apidb/readonly_pgsql_selection.hpp
                ../../../include/cgimap/backend/apidb/transaction_manager.hpp
                ../../../include/cgimap/backend/apidb/utils.hpp
                ../../../include/cgimap/backend/apidb/changeset_upload/changeset_updater.hpp
                ../../../include/cgimap/backend/apidb/changeset_upload/node_updater.hpp
                ../../../include/cgimap/backend/apidb/changeset_upload/relation_updater.hpp
                ../../../include/cgimap/backend/apidb/changeset_upload/way_updater.hpp
        )
    else()
        target_include_directories(cgimap_apidb PUBLIC
        ../../../include)
    endif()

    target_link_libraries(cgimap_apidb
        cgimap_common_compiler_options
        cgimap_core
        PQXX::PQXX)
endif()
