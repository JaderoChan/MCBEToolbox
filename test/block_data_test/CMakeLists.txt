set(TARGET_NAME MCBEToolBoxTestBlockData)
set(SRCS ./main.cpp)
add_executable(${TARGET_NAME} ${SRCS})
target_link_libraries(
    ${TARGET_NAME} PRIVATE
    MCBEToolBoxBackend
    json
)
