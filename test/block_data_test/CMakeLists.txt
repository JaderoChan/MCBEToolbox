set(TARGET_NAME MCBEToolboxTestBlockData)
set(SRCS ./main.cpp)
add_executable(${TARGET_NAME} ${SRCS})
target_link_libraries(
    ${TARGET_NAME} PRIVATE
    MCBEToolboxBackend
    json
)
copy_runtime_dependencies(${TARGET_NAME})
