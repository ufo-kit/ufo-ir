cmake_minimum_required(VERSION 2.6)

#{{{ sources
set(ufoir_SRCS
    ufo-ir-dummy-task.c
    ufo-ir-method-task-node.c
)
#}}}

#{{{ headers
set(ufoir_HDRS
    ufo-ir-dummy-task.h
    ufo-ir-method-task-node.h
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

foreach(_src ${ufoir_SRCS})
    # find plugin suffix
    string(REGEX REPLACE "ufo-ir-([^ \\.]+)-task.*" "\\1" task "${_src}")

    string(REPLACE "-" "" _targetname ${task})
    set(target "ufo${_targetname}")

    # build single shared library per filter
    if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        add_library(${target} MODULE ${_src} )
    else()
        add_library(${target} SHARED ${_src} )
    endif()

    target_link_libraries(${target} ${ufoir_LIBS})

    list(APPEND all_targets ${target})

    install(TARGETS ${target}
            ARCHIVE DESTINATION ${UFO_PLUGINDIR}/ir
            LIBRARY DESTINATION ${UFO_PLUGINDIR}/ir)
endforeach()

# copy kernels
foreach(_kernel ${ufofilter_KERNELS})
    install(FILES ${_kernel} DESTINATION ${UFO_KERNELDIR})
endforeach()
#}}}
#{{{ Python tests
find_program(NOSETESTSBIN nosetests)
if (NOSETESTSBIN)
    add_custom_target(test
        COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/../run_tests.py
        ${CMAKE_CURRENT_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../)
    add_dependencies(test ${all_targets})
endif()
#}}}
