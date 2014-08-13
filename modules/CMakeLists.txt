cmake_minimum_required(VERSION 2.6)
project(ufoir C)

set(TARNAME "ufo-ir-modules")

set(UFO_IR_MODULES_VERSION_MAJOR "0")
set(UFO_IR_MODULES_VERSION_MINOR "1")
set(UFO_IR_MODULES_VERSION_PATCH "0")
set(UFO_IR_MODULES_VERSION_STRING_LONG "${UFO_IR_MODULES_VERSION_MAJOR}.${UFO_IR_MODULES_VERSION_MINOR}.${UFO_IR_MODULES_VERSION_PATCH}")
set(UFO_IR_MODULES_VERSION_STRING_SHORT "${UFO_IR_MODULES_VERSION_MAJOR}.${UFO_IR_MODULES_VERSION_MINOR}")

set(UFO_IR_MODULES_DESCRIPTION "Plugins for Ufo-Ir")
set(UFO_IR_MODULES_DESCRIPTION_SUMMARY "Methods, Geometries, Projection models, etc.")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/common/cmake")

#{{{ Variables and Options
include(ConfigurePaths)
configure_paths(UFO_IR_MODULES)

set(UFO_FILTERS_PLUGINDIR "${UFO_IR_MODULES_LIBDIR}/ufo")
set(UFO_FILTERS_KERNELDIR "${UFO_IR_MODULES_DATADIR}/ufo")
set(PKG_UFO_CORE_MIN_REQUIRED "0.6")
set(PKG_UFO_IR_CORE_MIN_REQUIRED "0.1")

option(WITH_PROFILING "Enable profiling" OFF)

if (WITH_PROFILING)
    add_definitions("-pg")
    set(CMAKE_C_FLAGS "-pg")
endif ()
#}}}
#{{{ Common dependencies
find_package(OpenCL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(UFO ufo>=${PKG_UFO_CORE_MIN_REQUIRED} REQUIRED)
pkg_check_modules(UFO_IR ufoir>=${PKG_UFO_IR_CORE_MIN_REQUIRED} REQUIRED)

link_directories(${UFO_LIBRARY_DIRS})
link_directories(${UFO_IR_LIBRARY_DIRS})
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
