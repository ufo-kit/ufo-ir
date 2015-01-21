cmake_minimum_required(VERSION 2.6)

#{{{ Sources
set(ufo_math_plugins_SRCS
    ufo-math-shrinkage-method.c
    ufo-math-tvstd-method.c
)

set(ufo_math_plugins_LIBS
    ${GLIB2_LIBRARIES}
    ${GOBJECT2_LIBRARIES}
    ${OPENCL_LIBRARIES}
)

file(GLOB ufo_math_plugins_KERNELS "kernels/*.cl")
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
                    ${UFO_INCLUDE_DIRS})

foreach(_src ${ufo_math_plugins_SRCS})
    # find plugin suffix
    string(REGEX REPLACE "ufo-math-?([^ \\.]+).*" "\\1" plugin "${_src}")

    # build string to get miscalleanous sources
    string(REPLACE "-" "_" target ${plugin})
    string(TOUPPER ${target} _misc_upper)

    # create an option name and add this to disable filters
    set(target_option "ENABLE_${_misc_upper}")
    option(${target_option} "Build plugin ${plugin}" ON)

    if (${target_option})
        set(_misc "${target}_misc_SRCS")
        set(target "ufo_math_${target}")

        # build single shared library per filter
        if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
            add_library(${target} MODULE ${_src} ${${_misc}})
        else()
            add_library(${target} SHARED ${_src} ${${_misc}})
        endif()

        target_link_libraries(${target} ${ufo_math_plugins_LIBS})

        list(APPEND all_targets ${target})

        install(TARGETS ${target}
                ARCHIVE DESTINATION ${UFO_MATH_PLUGINS_PLUGINDIR}
                LIBRARY DESTINATION ${UFO_MATH_PLUGINS_PLUGINDIR})
    endif()
endforeach()

# copy kernels
foreach(_kernel ${ufo_math_plugins_KERNELS})
    install(FILES ${_kernel} DESTINATION ${UFO_MATH_PLUGINS_KERNELDIR})
endforeach()
#}}}