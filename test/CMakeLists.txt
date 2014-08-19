cmake_minimum_required(VERSION 2.6)

set(TARNAME "ufoirtest")

set(PROJECT_DESCRIPTION "Iterative reconstruction for UFO")
set(PROJECT_DESCRIPTION_SUMMARY "Iterative reconstruction for UFO")

#{{{ Library version
set(PROJECT_VERSION_MAJOR "0")
set(PROJECT_VERSION_MINOR "1")
set(PROJECT_VERSION_PATCH "0")
set(PROJECT_GIR_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
set(PROJECT_SO_VERSION "1")
#}}}
#{{{ Package version
set(PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(PACKAGE_VERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}.${PACKAGE_VERSION_PATCH}")
set(PACKAGE_NAME ${TARNAME})
set(PACKAGE_TARNAME ${TARNAME})
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "http://ufo.kit.edu/ufo/newticket")
#}}}
#{{{ CMake
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/common/cmake")
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")
#{{{ Options
option(WITH_PROFILING "Enable profiling" OFF)

if (WITH_PROFILING)
    add_definitions("-pg")
    set(CMAKE_C_FLAGS "-pg")
endif ()

#}}}
#{{{ Dependencies
set(PKG_GLIB2_MIN_REQUIRED "2.22")
set(PKG_UFO_MIN_REQUIRED "0.2")
set(PKG_UFO_IR_MIN_REQUIRED "0.1")

find_package(OpenCL REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB2 glib-2.0>=${PKG_GLIB2_MIN_REQUIRED} REQUIRED)
pkg_check_modules(GOBJECT2 gobject-2.0>=${PKG_GLIB2_MIN_REQUIRED} REQUIRED)
pkg_check_modules(UFO ufo>=${PKG_UFO_MIN_REQUIRED} REQUIRED)
pkg_check_modules(UFO_IR ufoir>=${PKG_UFO_IR_MIN_REQUIRED} REQUIRED)

set(DEPS
    ${OPENCL_LIBRARIES}
    ${GLIB2_LIBRARIES}
    ${UFO_LIBRARIES}
    ${UFO_IR_LIBRARIES}
    )

#{{{ Add include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${GLIB2_INCLUDE_DIRS}
    ${OPENCL_INCLUDE_DIRS}
    ${UFO_INCLUDE_DIRS}
    ${UFO_IR_INCLUDE_DIRS})
#}}}
#}}}
#{{{ Definitions
add_definitions("-std=c99 -pedantic -Wall -Wextra -fPIC")

if (CMAKE_COMPILER_IS_GNUCC  OR ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang"))
    add_definitions("-Wmissing-declarations
             -Wshadow
             -Wpointer-arith
             -Wcast-align
             -Wwrite-strings
             -Wredundant-decls
             -Wcast-qual
             -Winline
             -Wno-long-long
             -Wconversion
             -Wno-unused-parameter
             -Wno-missing-field-initializers")
endif()

add_definitions(-DG_LOG_DOMAIN=\"ufo-ir-test\")
#}}}
#{{{ Subdirectories
add_subdirectory(src)
#}}}
#{{{ CPack
set(CPACK_PACKAGE_DESCRIPTION ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION_SUMMARY})
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

include(CPack)
#}}}
