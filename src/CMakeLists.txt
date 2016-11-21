cmake_minimum_required(VERSION 2.6)

#{{{ Sources
set(ufoircore_SRCS
    core/ufo-ir-method-task.c
    core/ufo-ir-state-dependent-task.c
    core/ufo-ir-projector-task.c
    core/ufo-ir-basic-ops.c
    core/ufo-ir-basic-ops-processor.c
    core/ufo-ir-gradient-processor.c
    core/ufo-ir-debug.c
)

set(ufoir_SRCS
    tasks/ufo-ir-dummy-task.c
    tasks/ufo-ir-parallel-projector-task.c
    tasks/ufo-ir-sirt-task.c
    tasks/ufo-ir-sart-task.c
    tasks/ufo-ir-asdpocs-task.c
    tasks/ufo-ir-sbtv-task.c
)

file(GLOB ufoir_KERNELS "kernels/*.cl")
#}}}
#{{{ Variables
set(ufoir_LIBS ${UFO_LIBRARIES})
#}}}
#{{{ Plugin targets
include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${OPENCL_INCLUDE_DIRS}
                    ${UFO_INCLUDE_DIRS})

add_library(${TARNAME} SHARED ${ufoircore_SRCS})

install(TARGETS ${TARNAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

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
            LIBRARY DESTINATION ${CMAKE_INSTALL_PLUGINDIR}/ir)
endforeach()

# copy kernels
foreach(_kernel ${ufoir_KERNELS})
    install(FILES ${_kernel} DESTINATION ${CMAKE_INSTALL_KERNELDIR})
endforeach()
#}}}
#{{{ pkg-config
# FIXME: inside the ufo.pc.in we should set the lib names that we found out, not
# hard coded values
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/ufoir.pc.in"
               "${CMAKE_CURRENT_BINARY_DIR}/ufoir.pc" @ONLY IMMEDIATE)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ufoir.pc
        DESTINATION ${CMAKE_INSTALL_PKGCONFIGDIR})
#}}}
