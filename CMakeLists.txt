cmake_minimum_required(VERSION 3.0)

# UNIX : is TRUE on all UNIX-like OS's, including Apple OS X and CygWin
# WIN32 : is TRUE on Windows. Prior to 2.8.4 this included CygWin
# APPLE : is TRUE on Apple systems. Note this does not imply the system is Mac OS X, only that APPLE is #defined in C/C++ header files.
# MINGW : is TRUE when using the MinGW compiler in Windows
# MSYS : is TRUE when using the MSYS developer environment in Windows
# CYGWIN : is TRUE on Windows when using the CygWin version of cmake

#---------------------------------------------------------------------------------------
# Set default build type to release
#---------------------------------------------------------------------------------------
if (UNIX)
    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
    endif ()
endif ()

#---------------------------------------------------------------------------------------
# Set CMAKE_MACOSX_RPATH
#---------------------------------------------------------------------------------------
if (APPLE)
    set(CMAKE_MACOSX_RPATH 1)
endif ()

# CMake 3.9 and newer remove any effect the following settings may have on the install_name of a target on macOS:
#
# BUILD_WITH_INSTALL_RPATH target property
# SKIP_BUILD_RPATH target property
# CMAKE_SKIP_RPATH variable
# CMAKE_SKIP_INSTALL_RPATH variable
if (APPLE AND (${CMAKE_MAJOR_VERSION} GREATER_EQUAL 3 AND ${CMAKE_MINOR_VERSION} GREATER_EQUAL 9))
    #message("cmake version is ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}")
    option(MACOS_NO_RPATH "CMake 3.9 and newer remove any effect of RPATH" ON)
endif ()

#---------------------------------------------------------------------------------------
# Compiler config
#---------------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#---------------------------------------------------------------------------------------
# 3rd cmake modules
#---------------------------------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

#---------------------------------------------------------------------------------------
# CMake switches
#---------------------------------------------------------------------------------------
option(ASIO_STANDALONE "Enable asio standalone" ON) # if you wanna use http/websocket, please set this option off.
option(ZEPHYR_USE_SSL "Enable ssl" OFF)
option(ZEPHYR_ENABLE_RPC "Enable rpc" OFF)
option(ZEPHYR_BUILD_TESTS "Enable build tests" ON)
option(ZEPHYR_BUILD_EXAMPLES "Enable build examples" ON)
option(ZEPHYR_BUILD_BENCHMARK "Enable build benchmarks" ON)
#option(ZEPHYR_USE_XD_KCP "Enable X.D. KCP" OFF)

#---------------------------------------------------------------------------------------
# Get version number.
#---------------------------------------------------------------------------------------
include(cmake/utils.cmake)
zephyr_extract_version()

#---------------------------------------------------------------------------------------
# zephyr project
#---------------------------------------------------------------------------------------
project(zephyr VERSION ${ZEPHYR_VERSION} LANGUAGES CXX C)
message(STATUS "Build zephyr: v${ZEPHYR_VERSION}")

set(ZEPHYR_ROOT_DIR ${PROJECT_SOURCE_DIR})

if (WIN32)
    if (MSVC_VERSION LESS 1911)
        message(FATAL_ERROR "need msvc++ 14.1 Visual Studio 2017+")
    endif (MSVC_VERSION LESS 1911)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest -D_CRT_SECURE_NO_WARNINGS -D_SILENCE_CXX17_C_HEADER_DEPRECATION_WARNING")
else (UNIX)
    if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0)
        message(FATAL_ERROR "need gcc7.0+ or other compiler supports c++ 17")
    endif ()

    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -ggdb -Wall -Wextra -D_DEBUG -fPIC")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall -Wextra -DNDEBUG -fPIC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    add_definitions(-Wno-unused-parameter -Wno-unused-variable -Wno-unused-function)
endif ()


#if (ZEPHYR_USE_XD_KCP)
#    add_definitions(-DXD_KCP) # XD KCP
#endif ()

if (ASIO_STANDALONE)
    # standalone asio
    add_definitions(-DASIO_STANDALONE)
    include_directories(${ZEPHYR_ROOT_DIR}/asio/asio/include)
else ()
    # boost.asio
    find_package(Boost 1.72.0 REQUIRED)
    if (Boost_FOUND)
        message(STATUS "boost library found")
    else ()
        message(FATAL_ERROR "boost library is needed but can‘t be found")
    endif ()
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})
endif ()

if (ZEPHYR_ENABLE_RPC)
    # protobuf
    find_package(Protobuf 3.0.0 REQUIRED)
    if (PROTOBUF_FOUND)
        message(STATUS "protobuf library found")
    else ()
        message(FATAL_ERROR "protobuf library is needed but cant be found")
    endif ()
    add_definitions(-DZEPHYR_ENABLE_RPC)
    include_directories(${Protobuf_INCLUDE_DIRS})
endif ()

# zephyr include files
include_directories(${ZEPHYR_ROOT_DIR}/include)

#---------------------------------------------------------------------------------------
# log
#---------------------------------------------------------------------------------------
message(STATUS "Current system is [${CMAKE_SYSTEM}]")
message(STATUS "ZEPHYR_ROOT_DIR=${ZEPHYR_ROOT_DIR}")
message(STATUS "CMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}")
message(STATUS "Start to build all ...")

#---------------------------------------------------------------------------------------
# Self defined postfix
#---------------------------------------------------------------------------------------
set(CMAKE_DEBUG_POSTFIX "_d" CACHE STRING "Set debug library postfix")

#---------------------------------------------------------------------------------------
# Building samples
#---------------------------------------------------------------------------------------
if (ZEPHYR_BUILD_EXAMPLES)
    message(STATUS "Building examples ...")
    add_subdirectory(examples)
endif ()

#---------------------------------------------------------------------------------------
# Building tests
#---------------------------------------------------------------------------------------
if (ZEPHYR_BUILD_TESTS)
    message(STATUS "Building tests ...")
    enable_testing()
    #add_subdirectory(test)
endif ()

#---------------------------------------------------------------------------------------
# Building tests
#---------------------------------------------------------------------------------------
if (ZEPHYR_BUILD_BENCHMARK)
    message(STATUS "Building benchmark ...")
    add_subdirectory(benchmark)
endif ()
