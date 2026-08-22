set(TARGET_NAME MCBEToolBoxBackend)
file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(${TARGET_NAME} STATIC ${SOURCES})
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    ${TARGET_NAME} PRIVATE
    opencv_core
    opencv_imgcodecs
    opencv_videoio
)
