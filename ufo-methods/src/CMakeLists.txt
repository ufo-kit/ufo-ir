cmake_minimum_required(VERSION 2.6)

#{{{ sources
set(ufomethods_SRCS
    ufo-processor.c
    ufo-method-iface.c
    ufo-transform-iface.c
)
#}}}
#{{{ headers
set(ufomethods_HDRS
    ufo-processor.h
    ufo-method-iface.h
    ufo-transform-iface.h
)
#}}}
#{{{ libufomethods
add_definitions(-DUFO_METHODS_COMPILATION)

if(CMAKE_BUILD_TYPE MATCHES "Release")
    add_definitions(-DG_DISABLE_ASSERT)
endif()

add_library(ufomethods SHARED
            ${ufomethods_SRCS}
            ${CMAKE_CURRENT_BINARY_DIR}/ufo-methods-enums.c)

set_target_properties(ufomethods PROPERTIES
    VERSION ${PACKAGE_VERSION}
    SOVERSION ${UFO_METHODS_VERSION_MAJOR})

target_link_libraries(ufomethods ${UFO_METHODS_DEPS})

#{{{ install target
include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${UFO_INCLUDE_DIRS})

install(TARGETS ufomethods
        ARCHIVE DESTINATION ${UFO_METHODS_LIBDIR}
        LIBRARY DESTINATION ${UFO_METHODS_LIBDIR})

install(FILES ${ufomethods_HDRS}
        DESTINATION ${UFO_METHODS_INCLUDE_DIR}/ufo/methods)
#}}}
#}}}
#{{{ glib-mkenums
find_program(GLIB2_MKENUMS glib-mkenums REQUIRED)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ufo-methods-enums.h
    COMMAND ${GLIB2_MKENUMS}
    ARGS
        --template ufo-methods-enums.h.template
        ${ufomethods_HDRS} > ${CMAKE_CURRENT_BINARY_DIR}/ufo-methods-enums.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${ufomethods_HDRS}
            ${CMAKE_CURRENT_SOURCE_DIR}/ufo-methods-enums.h.template
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ufo-methods-enums.c
    COMMAND ${GLIB2_MKENUMS}
    ARGS
        --template ufo-methods-enums.c.template
        ${ufomethods_HDRS} > ${CMAKE_CURRENT_BINARY_DIR}/ufo-methods-enums.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${ufomethods_HDRS} ${CMAKE_CURRENT_BINARY_DIR}/ufo-methods-enums.h
            ${CMAKE_CURRENT_SOURCE_DIR}/ufo-methods-enums.c.template
)
#}}}
#{{{ g-ir-scanner and g-ir-compiler
find_program(INTROSPECTION_SCANNER "g-ir-scanner")
find_program(INTROSPECTION_COMPILER "g-ir-compiler")

if (0 AND INTROSPECTION_SCANNER AND INTROSPECTION_COMPILER)
    option(WITH_GIR "Build introspection files" ON)
    if (WITH_GIR)
        set(GIR_PREFIX "UfoMethods-${UFO_METHODS_GIR_VERSION}")
        set(GIR_XML "${GIR_PREFIX}.gir")
        set(GIR_TYPELIB "${GIR_PREFIX}.typelib")
        set(_gir_input)

        foreach(_src ${ufomethods_SRCS} ${ufomethods_HDRS})
            list(APPEND _gir_input "${CMAKE_CURRENT_SOURCE_DIR}/${_src}")
        endforeach()

        add_custom_command(OUTPUT ${GIR_XML}
            COMMAND ${INTROSPECTION_SCANNER}
                    --namespace=UfoMethods
                    --nsversion=${UFO_METHODS_GIR_VERSION}
                    --library=ufomethods
                    --no-libtool
                    --include=GObject-2.0
                    --include=GModule-2.0
                    --include=GLib-2.0
                    --include=Json-1.0
                    -I${OPENCL_INCLUDE_DIRS}
                    -I${JSON_GLIB_INCLUDE_DIRS}
                    -I${UFO_INCLUDE_DIR}
                    -I${CMAKE_CURRENT_SOURCE_DIR}/..
                    -DUFO_METHODS_COMPILATION
                    --output ${GIR_XML}
                    --warn-all
                    --quiet
                    ${_gir_input}
            DEPENDS ${ufomethods_SRCS}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

        add_custom_command(OUTPUT ${GIR_TYPELIB}
            COMMAND ${INTROSPECTION_COMPILER}
                    -o ${GIR_TYPELIB}
                    ${GIR_XML}
            DEPENDS ${GIR_XML}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

        add_custom_target(gir ALL DEPENDS ${GIR_XML} ${GIR_TYPELIB})
        add_dependencies(gir ufomethods)

        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${GIR_XML}
                DESTINATION ${UFO_METHODS_GIRDIR})

        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${GIR_TYPELIB}
                DESTINATION ${UFO_METHODS_TYPELIBDIR})
    endif()
endif()
#}}}
#{{{ pkg-config
# FIXME: inside the ufo.pc.in we should set the lib names that we found out, not
# hard coded values
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/ufomethods.pc.in"
               "${CMAKE_CURRENT_BINARY_DIR}/ufomethods.pc" @ONLY IMMEDIATE)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ufomethods.pc
        DESTINATION ${UFO_METHODS_PKGCONFIGDIR})
#}}}
