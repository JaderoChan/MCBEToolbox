project(MCBEToolboxBackend)

set(
    HEADERS
    ./block_image_factory.hpp
    ./block.hpp
    ./color_kd_tree.hpp
    ./color.hpp
    ./frames_ostream.hpp
    ./image_utilities.hpp
    ./mcbe_toolbox_api.hpp
    ./mcpack_factory.hpp
    ./structure_factory.hpp
    ./thread_pool.hpp
    ./version.hpp
)
set(
    SRCS
    ./block_image_factory.cpp
    ./block.cpp
    ./color_kd_tree.cpp
    ./color.cpp
    ./frames_ostream.cpp
    ./image_utilities.cpp
    ./mcbe_toolbox_api.cpp
    ./mcpack_factory.cpp
    ./structure_factory.cpp
    ./thread_pool.cpp
    ./version.cpp
)
set(PROJECT_SOURCES ${HEADERS} ${SRCS})

add_library(${PROJECT_NAME} STATIC ${PROJECT_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC
    opencv_core
    opencv_imgcodecs
    opencv_videoio
    mcnbt::mcnbt
    PRIVATE
    json
    nanoflann
    MINIZIP::minizip
)
