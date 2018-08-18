file( GLOB HEADERS "include/eosio/sync_api_plugin/*.hpp" )
add_library( sync_api_plugin
             sync_api_plugin.cpp
             ${HEADERS} )

target_link_libraries( sync_api_plugin sync_plugin chain_plugin http_plugin appbase )
target_include_directories( sync_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
