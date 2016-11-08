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

#{{{ Package version
set(PACKAGE_VERSION_MAJOR ${UFO_IR_VERSION_MAJOR})
set(PACKAGE_VERSION_MINOR ${UFO_IR_VERSION_MINOR})
set(PACKAGE_VERSION_PATCH ${UFO_IR_VERSION_PATCH})
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
add_definitions("-std=c99 -pedantic -Wall -Wextra -fPIC")

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

set(UFO_IR_KERNELDIR "${UFO_IR_DATADIR}/ufo")
file(GLOB ufoir_KERNELS "kernels/*.cl")
# copy kernels
foreach(_kernel ${ufoir_KERNELS})
    install(FILES ${_kernel} DESTINATION ${UFO_IR_KERNELDIR})
endforeach()

#{{{ CPack
set(CPACK_PACKAGE_DESCRIPTION ${UFO_IR_DESCRIPTION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${UFO_IR_DESCRIPTION_SUMMARY})
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
#}}}

include(CPack)
#}}}
