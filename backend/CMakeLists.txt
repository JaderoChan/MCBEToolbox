project(MCBEToolboxBackend)

set(
    HEADERS

    ./include/base_factory.hpp
    ./include/block_image_factory.hpp
    ./include/block.hpp
    ./include/color.hpp
    ./include/frames_ostream.hpp
    ./include/gif_frames_ostream.hpp
    ./include/image_frames_ostream.hpp
    ./include/image_utilities.hpp
    ./include/mcbe_toolbox_api.hpp
    ./include/mcfunction_factory.hpp
    ./include/mcpack.hpp
    ./include/mcstructure_factory.hpp
    ./include/version.hpp
    ./include/video_frames_ostream.hpp

    ./src/color_kd_tree.hpp
    ./src/frame_pipeline.hpp
    ./src/nanoflann.hpp
    ./src/thread_pool.hpp
)
set(
    SRCS
    ./src/base_factory.cpp
    ./src/block_image_factory.cpp
    ./src/block.cpp
    ./src/color_kd_tree.cpp
    ./src/color.cpp
    ./src/frames_ostream.cpp
    ./src/gif_frames_ostream.cpp
    ./src/image_frames_ostream.cpp
    ./src/image_utilities.cpp
    ./src/mcbe_toolbox_api.cpp
    ./src/mcfunction_factory.cpp
    ./src/mcpack.cpp
    ./src/mcstructure_factory.cpp
    ./src/thread_pool.cpp
    ./src/version.cpp
    ./src/video_frames_ostream.cpp
)
set(PROJECT_SOURCES ${HEADERS} ${SRCS})

add_library(${PROJECT_NAME} STATIC ${PROJECT_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC
    opencv_core
    opencv_imgcodecs
    opencv_videoio
    mcnbt::mcnbt
    PRIVATE
    gifdec
    json
    MINIZIP::minizip
)
