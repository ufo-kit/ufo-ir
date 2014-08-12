cmake_minimum_required(VERSION 2.6)

#{{{ sources
set(ufo_ir_core_SRCS
    ir/ufo-geometry.c
    ir/ufo-projector.c
    ir/ufo-processor.c
    ir/ufo-prior-knowledge.c
    ir/ufo-method-iface.c
    ir/ufo-sparsity-iface.c
    ir/ufo-transform-iface.c
    ir/ufo-ir-method.c
)
#}}}
#{{{ headers
set(ufo_ir_core_HDRS
    ir/ufo-geometry.h
    ir/ufo-projector.h
    ir/ufo-processor.h
    ir/ufo-prior-knowledge.h
    ir/ufo-method-iface.h
    ir/ufo-sparsity-iface.h
    ir/ufo-transform-iface.h
    ir/ufo-ir-method.h
)
#}}}
#{{{ libufoir
if(CMAKE_BUILD_TYPE MATCHES "Release")
    add_definitions(-DG_DISABLE_ASSERT)
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR})

add_library(ufo_ir SHARED
            ${ufo_ir_core_SRCS}
            ${CMAKE_CURRENT_BINARY_DIR}/ufo-ir-enums.c)

set_target_properties(ufo_ir PROPERTIES
    VERSION ${PACKAGE_VERSION}
    SOVERSION ${UFO_IR_VERSION_MAJOR})

target_link_libraries(ufo_ir ${UFO_IR_CORE_DEPS})

#{{{ install target
include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${OPENCL_INCLUDE_DIRS}
                    ${UFO_INCLUDE_DIRS})

install(TARGETS ufo_ir
        ARCHIVE DESTINATION ${UFO_IR_LIBDIR}
        LIBRARY DESTINATION ${UFO_IR_LIBDIR})

install(FILES ${ufo_ir_core_HDRS}
              ${CMAKE_CURRENT_SOURCE_DIR}/ufo-ir.h
              ${CMAKE_CURRENT_BINARY_DIR}/ufo-ir-enums.h
        DESTINATION ${UFO_IR_INCLUDEDIR}/ir)
#}}}
#}}}
#{{{ glib-mkenums
find_program(GLIB2_MKENUMS glib-mkenums REQUIRED)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ufo-ir-enums.h
    COMMAND ${GLIB2_MKENUMS}
    ARGS
        --template ufo-ir-enums.h.template
        ${ufo_ir_core_HDRS} > ${CMAKE_CURRENT_BINARY_DIR}/ufo-ir-enums.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${ufo_ir_core_HDRS}
            ${CMAKE_CURRENT_SOURCE_DIR}/ufo-ir-enums.h.template
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ufo-ir-enums.c
    COMMAND ${GLIB2_MKENUMS}
    ARGS
        --template ufo-ir-enums.c.template
        ${ufo_ir_core_HDRS} > ${CMAKE_CURRENT_BINARY_DIR}/ufo-ir-enums.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${ufo_ir_core_HDRS} ${CMAKE_CURRENT_BINARY_DIR}/ufo-ir-enums.h
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

        foreach(_src ${ufo_ir_core_SRCS} ${ufo_ir_core_HDRS})
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
                    -I${OPENCL_INCLUDE_DIRS}
                    -I${JSON_GLIB_INCLUDE_DIRS}
                    -I${UFO_INCLUDE_DIRS}
                    -I${CMAKE_CURRENT_SOURCE_DIR}/..
                    -DUFO_COMPILATION
                    --output ${GIR_XML}
                    --warn-all
                    --quiet
                    ${_gir_input}
            DEPENDS ${ufo_ir_core_SRCS}
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
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/ufo-ir.pc.in"
               "${CMAKE_CURRENT_BINARY_DIR}/ufo-ir.pc" @ONLY IMMEDIATE)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ufo-ir.pc
        DESTINATION ${UFO_IR_PKGCONFIGDIR})
#}}}