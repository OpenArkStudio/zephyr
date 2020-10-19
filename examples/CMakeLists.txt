if (UNIX)
    find_package(Threads REQUIRED)
endif ()

# client
set(client_project "client")
file(GLOB_RECURSE CLIENT_SRC src/client.cpp src/*.hpp ${ZEPHYR_ROOT_DIR}/include/*.hpp)
#list(FILTER ${CLIENT_SRC} EXCLUDE REGEX "*rpc*")
add_executable(${client_project} ${CLIENT_SRC})
if (UNIX)
    target_link_libraries(${client_project} pthread)
else ()
    target_link_libraries(${client_project})
endif ()


# server
set(server_project "server")
file(GLOB_RECURSE SERVER_SRC src/server.cpp src/*.hpp ${ZEPHYR_ROOT_DIR}/include/*.hpp)
#list(FILTER ${SERVER_SRC} EXCLUDE REGEX "*rpc*")
add_executable(${server_project} ${SERVER_SRC})
if (UNIX)
    target_link_libraries(${server_project} pthread)
else ()
    target_link_libraries(${server_project})
endif ()