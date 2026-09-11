cmake_minimum_required(VERSION 3.26)

project(MCBEToolbox VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 默认使用 Release 构建
if(NOT DEFINED CMAKE_BUILD_TYPE OR NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build with Release when not build type be specified." FORCE)
endif()

###############
# 配置第三方库 #
###############

include(FetchContent)
set(3RDPARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)

# 配置 OpenCV 库
find_package(OpenCV REQUIRED COMPONENTS core imgcodecs videoio)

# 查找 OpenCV FFmpeg 插件
if(WIN32)
    get_filename_component(OPENCV_BIN_DIR "${OpenCV_DIR}/../bin" ABSOLUTE)
    file(GLOB OPENCV_FFMPEG_DLLS "${OPENCV_BIN_DIR}/opencv_videoio_ffmpeg*.dll")
endif()

# 配置 zlib 库
set(ZLIB_BUILD_SHARED ON)
set(ZLIB_BUILD_STATIC OFF)
add_subdirectory(${3RDPARTY_DIR}/zlib)

# 配置 Json 库
add_subdirectory(${3RDPARTY_DIR}/json)
# 配置 nanoflann 库
add_subdirectory(${3RDPARTY_DIR}/nanoflann)
# 配置 Easy Translate 库
add_subdirectory(${3RDPARTY_DIR}/easy_translate)
# 配置 MC NBT 库
set(MCNBT_USE_BUILTIN_ZLIB OFF)
add_subdirectory(${3RDPARTY_DIR}/mcnbt)

# 配置 minizip 库
set(MZ_COMPAT OFF)
set(MZ_ZLIB ON)
set(MZ_BZIP2 OFF)
set(MZ_LZMA OFF)
set(MZ_PPMD OFF)
set(MZ_ZSTD OFF)
set(MZ_PKCRYPT OFF)
set(MZ_WZAES OFF)
set(MZ_OPENSSL OFF)
set(MZ_ICONV OFF)
set(MZ_COMPRESS_ONLY ON)
set(MZ_FETCH_LIBS OFF)
add_subdirectory(${3RDPARTY_DIR}/minizip-ng)

# 重定向目标文件输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

function(copy_runtime_dependencies TARGET_NAME)
    if(WIN32)
        add_custom_command(
            TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${TARGET_NAME}>
            $<TARGET_FILE_DIR:${TARGET_NAME}>
            COMMAND_EXPAND_LISTS
        )
        if(OPENCV_FFMPEG_DLLS)
            add_custom_command(
                TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${OPENCV_FFMPEG_DLLS}
                $<TARGET_FILE_DIR:${TARGET_NAME}>
            )
        endif()
    endif()
endfunction()

if(MSVC)
    add_compile_options(/utf-8)
endif()

#############
# 添加子目录 #
#############

add_subdirectory(backend)
add_subdirectory(frontend)

option(MCBETOOLBOX_BUILD_TEST "Build MCBE Toolbox test programs" ON)
if(MCBETOOLBOX_BUILD_TEST)
    add_subdirectory(test)
endif()
