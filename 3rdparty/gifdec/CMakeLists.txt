project(gifdec)

add_library(${PROJECT_NAME} STATIC ./gifdec.c)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
