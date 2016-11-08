cmake_minimum_required(VERSION 2.6)

if (POLICY CMP0053)
cmake_policy(SET CMP0053 OLD)
endif ()

if (POLICY CMP0054)
cmake_policy(SET CMP0054 OLD)
endif ()

project(ufoir C)

set(TARNAME "ufo-ir")

set(UFO_IR_DESCRIPTION "Iterative reconstruction for UFO")
set(UFO_IR_DESCRIPTION_SUMMARY "Iterative reconstruction for UFO")
#{{{ Library version
set(UFO_IR_VERSION_MAJOR "0")
set(UFO_IR_VERSION_MINOR "1")
set(UFO_IR_VERSION_PATCH "0")
set(UFO_IR_GIR_VERSION "${UFO_IR_VERSION_MAJOR}.0")
#}}}
#{{{ CMake
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/common/cmake")
#}}}
#{{{ Options
option(WITH_PROFILING "Enable profiling" OFF)
option(WITH_DEBUG "Build with DEBUG support" OFF)

if (WITH_PROFILING)
    add_definitions("-pg")
    set(CMAKE_C_FLAGS "-pg")
endif ()

if (WITH_DEBUG)
    add_definitions ("-DDEBUG")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb")
endif ()
#}}}
#{{{ Dependencies
set(PKG_GLIB2_MIN_REQUIRED "2.22")
set(PKG_UFO_MIN_REQUIRED "0.2")

find_package(OpenCL REQUIRED)
find_package(PkgConfig REQUIRED)

include(PkgConfigVars)
include(GNUInstallDirs)

pkg_check_modules(GLIB2 glib-2.0>=${PKG_GLIB2_MIN_REQUIRED} REQUIRED)
pkg_check_modules(GOBJECT2 gobject-2.0>=${PKG_GLIB2_MIN_REQUIRED} REQUIRED)
pkg_check_modules(GIO2 gio-2.0>=${PKG_GLIB2_MIN_REQUIRED} REQUIRED)
pkg_check_modules(UFO ufo>=${PKG_UFO_MIN_REQUIRED} REQUIRED)

pkg_check_variable(ufo plugindir)
pkg_check_variable(ufo kerneldir)

set(UFO_IR_DEPS
    ${OPENCL_LIBRARIES}
    ${GLIB2_LIBRARIES}
    ${GOBJECT2_LIBRARIES}
    ${UFO_LIBRARIES}
    )

set(CMAKE_INSTALL_PLUGINDIR     "${UFO_PLUGINDIR}")
set(CMAKE_INSTALL_KERNELDIR     "${UFO_KERNELDIR}")
set(CMAKE_INSTALL_PKGCONFIGDIR  "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

link_directories(${UFO_LIBRARY_DIRS})

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${GLIB2_INCLUDE_DIRS}
    ${GIO2_INCLUDE_DIRS}
    ${OPENCL_INCLUDE_DIRS}
    ${UFO_INCLUDE_DIRS})
#}}}
#{{{ Definitions
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -Wextra -fPIC -Wno-unused-parameter -Wno-deprecated-declarations")
#}}}
#{{{ Subdirectories
add_subdirectory(src)
#}}}
