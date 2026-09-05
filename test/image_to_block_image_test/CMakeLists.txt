set(TARGET_NAME image_to_block_image_test)
set(SRCS ./main.cpp)
add_executable(${TARGET_NAME} ${SRCS})
target_link_libraries(
    ${TARGET_NAME} PRIVATE
    MCBEToolboxBackend
)
copy_runtime_dependencies(${TARGET_NAME})
