project(block_entries_parse_test)

set(SRCS ./main.cpp)
add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries(
    ${PROJECT_NAME} PRIVATE
    MCBEToolboxBackend
    json
)

copy_runtime_dependencies(${PROJECT_NAME})
