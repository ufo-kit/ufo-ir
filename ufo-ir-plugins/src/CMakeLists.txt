cmake_minimum_required(VERSION 2.6)

#{{{ Sources
set(ufoirplugins_SRCS
    ufo-ir-parallel-geometry.c
    ufo-ir-cl-projector.c
    ufo-ir-custom-sparsity.c
    ufo-ir-gradient-sparsity.c
    ufo-ir-sart.c
    ufo-ir-asdpocs.c
)

set(ufoirplugins_LIBS
    ${UFO_IR_LIBRARIES}
    ${GLIB2_LIBRARIES}
    ${GOBJECT2_LIBRARIES}
    ${OPENCL_LIBRARIES}
)

file(GLOB ufoirplugins_KERNELS "kernels/*.cl")
#}}}
#{{{ Variables

if (CMAKE_COMPILER_IS_GNUCC OR ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang"))
    add_definitions("-Wcast-align -Wcast-qual -Winline -Wmissing-declarations "
                    "-Wmissing-prototypes -Wnested-externs -Wno-long-long "
                    "-Wno-missing-field-initializers -Wpointer-arith "
                    "-Wredundant-decls -Wshadow -Wstrict-prototypes -Wwrite-strings")
endif()
#}}}
#{{{ Plugin targets
include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${OPENCL_INCLUDE_DIRS}
                    ${UFO_IR_INCLUDE_DIRS})

foreach(_src ${ufoirplugins_SRCS})
    # find plugin suffix
    string(REGEX REPLACE "ufo-ir-?([^ \\.]+).*" "\\1" plugin "${_src}")

    # build string to get miscalleanous sources
    string(REPLACE "-" "_" _misc ${plugin})
    string(TOUPPER ${_misc} _misc_upper)

    # create an option name and add this to disable filters
    set(target_option "ENABLE_${_misc_upper}")
    option(${target_option} "Build filter ${plugin}" ON)

    if (${target_option})
        set(_misc "${_misc}_misc_SRCS")

        string(REPLACE "-" "" _targetname ${plugin})
        set(target "ufoir_${_targetname}")
        message ("target: ${target}")

        # build single shared library per filter
        if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
            add_library(${target} MODULE ${_src} ${${_misc}})
        else()
            add_library(${target} SHARED ${_src} ${${_misc}})
        endif()

        target_link_libraries(${target} ${ufoirplugins_LIBS})

        list(APPEND all_targets ${target})

        install(TARGETS ${target}
                ARCHIVE DESTINATION ${UFO_IR_PLUGINS_PLUGINDIR}
                LIBRARY DESTINATION ${UFO_IR_PLUGINS_PLUGINDIR})
    endif()
endforeach()

# copy kernels
foreach(_kernel ${ufoirplugins_KERNELS})
    install(FILES ${_kernel} DESTINATION ${UFO_IR_PLUGINS_KERNELDIR})
endforeach()
#}}}