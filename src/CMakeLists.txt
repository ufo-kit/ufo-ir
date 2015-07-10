cmake_minimum_required(VERSION 2.6)

#{{{ sources
set(ufoircore_SRCS
    core/ufo-ir-method-task.c
    core/ufo-ir-state-dependent-task.c
    core/ufo-ir-projector-task.c
    core/ufo-ir-basic-ops.c
)
#}}}

#{{{ headers
set(ufoircore_HDRS
    core/ufo-ir-method-task-node.h
    core/ufo-ir-state-dependent-task-node.h
    core/ufo-ir-projector-task.h
    core/ufo-ir-basic-ops.h
)
#}}}


#{{{ sources
set(ufoir_SRCS
    tasks/ufo-ir-dummy-task.c
    tasks/ufo-ir-parallel-projector-task.c
    tasks/ufo-ir-sirt-task.c
    tasks/ufo-ir-sart-task.c
    tasks/ufo-ir-asdpocs-task.c
    tasks/ufo-ir-splitbregman-task.c
#    tasks/ufo-ir-gradient-task.c
)
#}}}

#{{{ headers
set(ufoir_HDRS
    tasks/ufo-ir-dummy-task.h
    tasks/ufo-ir-parallel-projector-task.h
    tasks/ufo-ir-sirt-task.h
    tasks/ufo-ir-sart-task.h
    tasks/ufo-ir-asdpocs-task.h
    tasks/ufo-ir-splitbregman-task.h
#    tasks/ufo-ir-gradient-task.h
)
#}}}

file(GLOB ufoir_KERNELS "kernels/*.cl")
#}}}

#{{{ Variables
set(ufoir_LIBS ${UFO_LIBRARIES})

if (CMAKE_COMPILER_IS_GNUCC OR ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang"))
    add_definitions("-Wcast-align -Wcast-qual -Winline -Wmissing-declarations "
                    "-Wmissing-prototypes -Wnested-externs -Wno-long-long "
                    "-Wno-missing-field-initializers -Wpointer-arith "
                    "-Wredundant-decls -Wshadow -Wstrict-prototypes -Wwrite-strings")
endif()
#}}}

#{{{ Dependency checks
#}}}

#{{{ Plugin targets
include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${OPENCL_INCLUDE_DIRS}
                    ${UFO_INCLUDE_DIRS})

#configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
#               ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_library(${TARNAME} SHARED ${ufoircore_SRCS})
install(TARGETS ${TARNAME}
        ARCHIVE DESTINATION ${UFO_LIBDIR}
        LIBRARY DESTINATION ${UFO_LIBDIR})

foreach(_src ${ufoir_SRCS})
    # find plugin suffix
    string(REGEX REPLACE "tasks/ufo-ir-([^ \\.]+)-task.*" "\\1" task "${_src}")

    string(REPLACE "-" "" _targetname ${task})
    set(target "ufo${_targetname}")

    # build single shared library per filter
    if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        add_library(${target} MODULE ${_src} )
    else()
        add_library(${target} SHARED ${_src} )
    endif()

    target_link_libraries(${target} ${ufoir_LIBS} ${TARNAME})

    list(APPEND all_targets ${target})

    install(TARGETS ${target}
            ARCHIVE DESTINATION ${UFO_PLUGINDIR}/ir
            LIBRARY DESTINATION ${UFO_PLUGINDIR}/ir)
endforeach()

# copy kernels
foreach(_kernel ${ufoir_KERNELS})
    install(FILES ${_kernel} DESTINATION ${UFO_KERNELDIR})
endforeach()
#}}}
#{{{ Python tests
#find_program(NOSETESTSBIN nosetests)
#if (NOSETESTSBIN)
#    add_custom_target(test
#        COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/../run_tests.py
#        ${CMAKE_CURRENT_BINARY_DIR}
#        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../)
#    add_dependencies(test ${all_targets})
#endif()
#}}}

#{{{ pkg-config
# FIXME: inside the ufo.pc.in we should set the lib names that we found out, not
# hard coded values
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/ufoir.pc.in"
               "${CMAKE_CURRENT_BINARY_DIR}/ufoir.pc" @ONLY IMMEDIATE)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ufoir.pc
        DESTINATION ${UFO_IR_PKGCONFIGDIR})
#}}}
