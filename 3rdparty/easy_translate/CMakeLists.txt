project(easy_translate VERSION 1.2.0 LANGUAGES CXX)

add_library(${PROJECT_NAME} INTERFACE ./include/easy_translate.hpp)

target_include_directories(
    ${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
