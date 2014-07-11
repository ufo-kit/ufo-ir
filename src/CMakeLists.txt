cmake_minimum_required(VERSION 2.6)


set(PROJECT_SRCS
    main.c
)

set(PROJECT_LIBS
    m
    ${UFO_LIBRARIES}
    ${GLIB2_LIBRARIES}
    ${GOBJECT2_LIBRARIES}
    ${OPENCL_LIBRARIES}
    )

find_package(TIFF)
if (TIFF_FOUND)
    set(PROJECT_LIBS ${PROJECT_LIBS} ${TIFF_LIBRARIES})
    include_directories(${TIFF_INCLUDE_DIRS})
endif ()

include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${OPENCL_INCLUDE_DIRS})

link_directories(${UFO_LIBRARY_DIRS})

add_executable(${TARNAME} ${PROJECT_SRCS})
target_link_libraries(${TARNAME}  ${PROJECT_LIBS})
