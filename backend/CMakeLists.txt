set(TARGET_NAME MCBEToolboxBackend)

set(
    HEADERS
    ./block.hpp
    ./color.hpp
    ./mcbe_toolbox_api.hpp
    ./version.hpp
)
set(
    SRCS
    ./block.cpp
    ./color.cpp
    ./mcbe_toolbox_api.cpp
    ./version.cpp
)
set(PROJECT_SOURCES ${HEADERS} ${SRCS})

add_library(${TARGET_NAME} STATIC ${PROJECT_SOURCES})

target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    ${TARGET_NAME} PRIVATE
    opencv_core
    opencv_imgcodecs
    opencv_videoio
    json
    nanoflann
    mcnbt::mcnbt
)
