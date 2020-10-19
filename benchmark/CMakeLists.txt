if (UNIX)
    find_package(Threads REQUIRED)
endif ()

function(BENCHMARK_FUNCTION project_name)
    file(GLOB_RECURSE BENCHMARK_SRC ${project_name}.cpp)
    add_executable(${project_name} ${BENCHMARK_SRC})
    if (UNIX)
        target_link_libraries(${project_name} pthread)
    endif ()
    set_target_properties(${project_name} PROPERTIES OUTPUT_NAME_DEBUG "${project_name}_d")
endfunction()

set(BENCHMARKS
        ping_pong_client
        ping_pong_server
        broadcast_client
        broadcast_server
        kcp_client
        kcp_server)

foreach (benchmark_index ${BENCHMARKS})
    BENCHMARK_FUNCTION(${benchmark_index})
endforeach ()