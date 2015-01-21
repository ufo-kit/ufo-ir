cmake_minimum_required(VERSION 2.6)
project(ufo_math C)

set(TARNAME "ufo-math")

set(UFO_MATH_PLUGINS_VERSION_MAJOR "0")
set(UFO_MATH_PLUGINS_VERSION_MINOR "1")
set(UFO_MATH_PLUGINS_VERSION_PATCH "0")
set(UFO_MATH_PLUGINS_VERSION_STRING_LONG "${UFO_MATH_PLUGINS_VERSION_MAJOR}.${UFO_MATH_PLUGINS_VERSION_MINOR}.${UFO_MATH_PLUGINS_VERSION_PATCH}")
set(UFO_MATH_PLUGINS_VERSION_STRING_SHORT "${UFO_MATH_PLUGINS_VERSION_MAJOR}.${UFO_MATH_PLUGINS_VERSION_MINOR}")

set(UFO_MATH_DESCRIPTION "UFO mathematical methods")
set(UFO_MATH_DESCRIPTION_SUMMARY "UFO mathematical methods")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/common/cmake")

#{{{ Variables and Options
include(ConfigurePaths)
configure_paths(UFO_MATH_PLUGINS)

set(UFO_MATH_PLUGINS_PLUGINDIR "${UFO_MATH_PLUGINS_LIBDIR}/ufo")
set(UFO_MATH_PLUGINS_KERNELDIR "${UFO_MATH_PLUGINS_DATADIR}/ufo")
set(PKG_UFO_MATH_MIN_REQUIRED "0.1")

option(WITH_PROFILING "Enable profiling" OFF)

if (WITH_PROFILING)
    add_definitions("-pg")
    set(CMAKE_C_FLAGS "-pg")
endif ()
#}}}
#{{{ Common dependencies
find_package(OpenCL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(UFO ufo>=${PKG_UFO_MATH_MIN_REQUIRED} REQUIRED)

link_directories(${UFO_MATH_LIBRARY_DIRS})
#}}}
#{{{ Global definitions
add_definitions("-std=c99 -pedantic -Wall -Wextra -fPIC")
add_definitions(-DG_LOG_DOMAIN="UfoIr")

if (CMAKE_COMPILER_IS_GNUCC OR ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang"))
    add_definitions("-Wno-unused-parameter")
endif ()

#}}}
#{{{ Subdirectories
add_subdirectory(src)
#}}}
