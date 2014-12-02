cmake_minimum_required(VERSION 2.6)

#{{{ sources
set(ufoir_SRCS
    ufo-ir-method.c
    ufo-ir-prior-knowledge.c
    ufo-ir-sparsity-iface.c
    ufo-ir-geometry.c
    ufo-ir-projector.c
)
#}}}
#{{{ headers
set(ufoir_HDRS
    ufo-ir-method.h
    ufo-ir-prior-knowledge.h
    ufo-ir-sparsity-iface.h
    ufo-ir-geometry.h
    ufo-ir-projector.h
)
#}}}
#{{{ libufoir
add_definitions(-DUFO_IR_COMPILATION)

if(CMAKE_BUILD_TYPE MATCHES "Release")
    add_definitions(-DG_DISABLE_ASSERT)
endif()

add_library(ufoir SHARED
            ${ufoir_SRCS}
            ${CMAKE_CURRENT_BINARY_DIR}/ufo-ir-enums.c)

set_target_properties(ufoir PROPERTIES
    VERSION ${PACKAGE_VERSION}
    SOVERSION ${UFO_IR_VERSION_MAJOR})

target_link_libraries(ufoir ${UFO_IR_DEPS})

#{{{ install target
include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${UFO_INCLUDE_DIRS})

install(TARGETS ufoir
        ARCHIVE DESTINATION ${UFO_IR_LIBDIR}
        LIBRARY DESTINATION ${UFO_IR_LIBDIR})

install(FILES ${ufoir_HDRS}
              ${CMAKE_CURRENT_SOURCE_DIR}/ufo-ir.h
              ${CMAKE_CURRENT_BINARY_DIR}/ufo-ir-enums.h
        DESTINATION ${UFO_IR_INCLUDE_DIR}/ufo/ir)
#}}}
#}}}
#{{{ glib-mkenums
find_program(GLIB2_MKENUMS glib-mkenums REQUIRED)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ufo-ir-enums.h
    COMMAND ${GLIB2_MKENUMS}
    ARGS
        --template ufo-ir-enums.h.template
        ${ufoir_HDRS} > ${CMAKE_CURRENT_BINARY_DIR}/ufo-ir-enums.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${ufoir_HDRS}
            ${CMAKE_CURRENT_SOURCE_DIR}/ufo-ir-enums.h.template
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ufo-ir-enums.c
    COMMAND ${GLIB2_MKENUMS}
    ARGS
        --template ufo-ir-enums.c.template
        ${ufoir_HDRS} > ${CMAKE_CURRENT_BINARY_DIR}/ufo-ir-enums.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${ufoir_HDRS} ${CMAKE_CURRENT_BINARY_DIR}/ufo-ir-enums.h
            ${CMAKE_CURRENT_SOURCE_DIR}/ufo-ir-enums.c.template
)
#}}}
#{{{ g-ir-scanner and g-ir-compiler
find_program(INTROSPECTION_SCANNER "g-ir-scanner")
find_program(INTROSPECTION_COMPILER "g-ir-compiler")
if (INTROSPECTION_SCANNER AND INTROSPECTION_COMPILER)
    option(WITH_GIR "Build introspection files" ON)
    if (WITH_GIR)
        set(GIR_PREFIX "UfoIr-${UFO_IR_GIR_VERSION}")
        set(GIR_XML "${GIR_PREFIX}.gir")
        set(GIR_TYPELIB "${GIR_PREFIX}.typelib")
        set(_gir_input)

        foreach(_src ${ufoir_SRCS} ${ufoir_HDRS})
            list(APPEND _gir_input "${CMAKE_CURRENT_SOURCE_DIR}/${_src}")
        endforeach()

        add_custom_command(OUTPUT ${GIR_XML}
            COMMAND ${INTROSPECTION_SCANNER}
                    --namespace=UfoIr
                    --nsversion=${UFO_IR_GIR_VERSION}
                    --library=ufoir
                    --no-libtool
                    --include=GObject-2.0
                    --include=GModule-2.0
                    --include=GLib-2.0
                    --include=Json-1.0
                    --include=Ufo-0.0
                    -I${OPENCL_INCLUDE_DIRS}
                    -I${JSON_GLIB_INCLUDE_DIRS}
                    -I${UFO_METHODS_INCLUDE_DIR}
                    -I${UFO_INCLUDE_DIR}
                    -I${CMAKE_CURRENT_SOURCE_DIR}/..
                    -DUFO_IR_COMPILATION
                    --output ${GIR_XML}
                    --warn-all
                    --quiet
                    ${_gir_input}
            DEPENDS ${ufoir_SRCS}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

        add_custom_command(OUTPUT ${GIR_TYPELIB}
            COMMAND ${INTROSPECTION_COMPILER}
                    -o ${GIR_TYPELIB}
                    ${GIR_XML}
            DEPENDS ${GIR_XML}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

        add_custom_target(gir ALL DEPENDS ${GIR_XML} ${GIR_TYPELIB})
        add_dependencies(gir ufoir)

        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${GIR_XML}
                DESTINATION ${UFO_IR_GIRDIR})

        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${GIR_TYPELIB}
                DESTINATION ${UFO_IR_TYPELIBDIR})
    endif()
endif()
#}}}
#{{{ pkg-config
# FIXME: inside the ufo.pc.in we should set the lib names that we found out, not
# hard coded values
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/ufoir.pc.in"
               "${CMAKE_CURRENT_BINARY_DIR}/ufoir.pc" @ONLY IMMEDIATE)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ufoir.pc
        DESTINATION ${UFO_IR_PKGCONFIGDIR})
#}}}
