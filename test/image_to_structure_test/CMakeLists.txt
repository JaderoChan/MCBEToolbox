project(image_or_video_to_structure)

set(SRCS ./main.cpp)
add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries(
    ${PROJECT_NAME} PRIVATE
    MCBEToolboxBackend
)

copy_runtime_dependencies(${PROJECT_NAME})
