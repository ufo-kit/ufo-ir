cmake_minimum_required(VERSION 2.6)
project(ufomethods C)

set(TARNAME "libufomethods")

set(UFO_METHODS_DESCRIPTION "Basic classes UFO")
set(UFO_METHODS_DESCRIPTION_SUMMARY "Basic classes UFO")

#{{{ Library version
set(UFO_METHODS_VERSION_MAJOR "0")
set(UFO_METHODS_VERSION_MINOR "1")
set(UFO_METHODS_VERSION_PATCH "0")
set(UFO_METHODS_GIR_VERSION "${UFO_METHODS_VERSION_MAJOR}.0")
#}}}
#{{{ Package version
set(PACKAGE_VERSION_MAJOR ${UFO_METHODS_VERSION_MAJOR})
set(PACKAGE_VERSION_MINOR ${UFO_METHODS_VERSION_MINOR})
set(PACKAGE_VERSION_PATCH ${UFO_METHODS_VERSION_PATCH})
set(PACKAGE_VERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}.${PACKAGE_VERSION_PATCH}")
set(PACKAGE_NAME ${TARNAME})
set(PACKAGE_TARNAME ${TARNAME})
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "http://ufo.kit.edu/ufo/newticket")
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

pkg_check_modules(GLIB2 glib-2.0>=${PKG_GLIB2_MIN_REQUIRED} REQUIRED)
pkg_check_modules(GOBJECT2 gobject-2.0>=${PKG_GLIB2_MIN_REQUIRED} REQUIRED)
pkg_check_modules(UFO ufo>=${PKG_UFO_MIN_REQUIRED} REQUIRED)

set(UFO_METHODS_DEPS
    ${OPENCL_LIBRARIES}
    ${GLIB2_LIBRARIES}
    ${GOBJECT2_LIBRARIES}
    ${JSON_GLIB_LIBRARIES}
    ${UFO_LIBRARIES}
    )

link_directories(${UFO_LIBRARY_DIRS})

#{{{ Add include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${GLIB2_INCLUDE_DIRS}
    ${OPENCL_INCLUDE_DIRS}
    ${UFO_INCLUDE_DIRS})
#}}}
#}}}
#{{{ CMake: Add configure-like prefixes and directories
include(ConfigurePaths)
configure_paths(UFO_METHODS)

string(REGEX MATCH "[A-Za-z./]*ufo-[0-9]" UFO_METHODS_INCLUDE_DIR ${UFO_INCLUDE_DIRS})
message ("UFO_METHODS_INCLUDE_DIR: ${UFO_METHODS_INCLUDE_DIR}")
set (UFO_INCLUDE_DIR
     ${UFO_METHODS_INCLUDE_DIR})

if (${UFO_METHODS_INCLUDE_DIR})
    set(UFO_METHODS_INCLUDE_DIR "${UFO_METHODS_INCLUDEDIR}/ufomethods-${UFO_METHODS_VERSION_MAJOR}")
endif()
message ("UFO_INCLUDE_DIR: ${UFO_INCLUDE_DIR}")
message ("UFO_METHODS_INCLUDE_DIR: ${UFO_METHODS_INCLUDE_DIR}")

#}}}
#{{{ Definitions
add_definitions("-std=c99 -pedantic -Wall -Wextra -Werror -fPIC")

if (CMAKE_COMPILER_IS_GNUCC  OR ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang"))
    add_definitions("-Wcast-align
                     -Wcast-qual
                     -Winline
                     -Wmissing-declarations
                     -Wmissing-prototypes
                     -Wnested-externs
                     -Wno-deprecated-declarations
                     -Wno-long-long
                     -Wno-missing-field-initializers
                     -Wno-unused-parameter
                     -Wpointer-arith
                     -Wredundant-decls
                     -Wshadow
                     -Wstrict-prototypes
                     -Wwrite-strings")
endif()

#}}}
#{{{ Subdirectories
add_subdirectory(src)
#}}}
#{{{ CPack
set(CPACK_PACKAGE_DESCRIPTION ${UFO_METHODS_DESCRIPTION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${UFO_METHODS_DESCRIPTION_SUMMARY})
set(CPACK_PACKAGE_NAME ${TARNAME})

set(CPACK_PACKAGE_CONTACT "cupmathii@gmail.com")
set(CPACK_PACKAGE_VENDOR "Karlsruhe Institute of Technology/IPE")
set(CPACK_PACKAGE_VERSION_MAJOR ${PACKAGE_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PACKAGE_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PACKAGE_VERSION_PATCH})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PACKAGE_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")
set(VERSION ${PACKAGE_VERSION})

set(CPACK_GENERATOR "DEB;RPM;")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES "/build/;.git/;tags;.swp;${CPACK_SOURCE_IGNORE_FILES}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PACKAGE_NAME}-${PACKAGE_VERSION}" CACHE INTERNAL "tarball basename")

#{{{ RPM
set(CPACK_RPM_PACKAGE_AUTOREQPROV " no")
set(CPACK_RPM_PACKAGE_REQUIRES "libjson-glib-1_0-0")
#}}}

include(CPack)
#}}}
