cmake_minimum_required(VERSION 2.6)

#{{{ Sources
set(ufoirmodules_SRCS
    ufo-parallel-geometry.c
    ufo-cl-projector.c
    ufo-custom-sparsity.c
    ufo-gradient-sparsity.c
    ufo-ir-sart.c
    ufo-ir-asdpocs.c
)

set(ufoirmodules_LIBS
    m
    ${UFO_IR_LIBRARIES}
    ${GLIB2_LIBRARIES}
    ${GOBJECT2_LIBRARIES}
    ${OPENCL_LIBRARIES}
    )

file(GLOB ufo_ir_modules_KERNELS "kernels/*.cl")
#}}}
#{{{ Variables
set(ufoirmodules_LIBS
    ${UFO_LIBRARIES}
    oclfft)

if (CMAKE_COMPILER_IS_GNUCC OR ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang"))
    add_definitions("-Wcast-align -Wcast-qual -Winline -Wmissing-declarations "
                    "-Wmissing-prototypes -Wnested-externs -Wno-long-long "
                    "-Wno-missing-field-initializers -Wpointer-arith "
                    "-Wredundant-decls -Wshadow -Wstrict-prototypes -Wwrite-strings")
endif()
#}}}
#{{{ Dependency checks
find_package(TIFF)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp")

if (TIFF_FOUND)
    list(APPEND ufoirmodules_LIBS ${TIFF_LIBRARIES})
    include_directories(${TIFF_INCLUDE_DIRS})
    link_directories(${TIFF_LIBRARY_DIRS})
endif ()
#}}}
#{{{ Plugin targets
include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${OPENCL_INCLUDE_DIRS}
                    ${UFO_INCLUDE_DIRS})

foreach(_src ${ufoirmodules_SRCS})
    # find plugin suffix
    string(REGEX REPLACE "ufo-?([^ \\.]+).*" "\\1" module "${_src}")

    # build string to get miscalleanous sources
    string(REPLACE "-" "_" _misc ${module})
    string(TOUPPER ${_misc} _misc_upper)

    # create an option name and add this to disable filters
    set(target_option "ENABLE_${_misc_upper}")
    option(${target_option} "Build filter ${module}" ON)

    if (${target_option})
        set(_misc "${_misc}_misc_SRCS")

        string(REPLACE "-" "" _targetname ${module})
        set(target "ufoirmodule_${_targetname}")
        message ("target: ${target}")

        # build single shared library per filter
        if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
            add_library(${target} MODULE ${_src} ${${_misc}})
        else()
            add_library(${target} SHARED ${_src} ${${_misc}})
        endif()

        target_link_libraries(${target} ${ufofilter_LIBS})

        list(APPEND all_targets ${target})

        install(TARGETS ${target}
                ARCHIVE DESTINATION ${UFO_FILTERS_PLUGINDIR}
                LIBRARY DESTINATION ${UFO_FILTERS_PLUGINDIR})
    endif()
endforeach()

# copy kernels
foreach(_kernel ${ufofilter_KERNELS})
    install(FILES ${_kernel} DESTINATION ${UFO_FILTERS_KERNELDIR})
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