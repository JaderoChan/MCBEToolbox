project(nanoflann)

add_library(${PROJECT_NAME} INTERFACE ./include/nanoflann.hpp)

target_include_directories(
    ${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
