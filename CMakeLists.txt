cmake_minimum_required(VERSION 3.26)

project(MCBEToolbox VERSION 1.0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_SKIP_INSTALL_RULES ON)

# 默认使用 Release 构建
if(NOT DEFINED CMAKE_BUILD_TYPE OR NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build with Release when not build type be specified." FORCE)
endif()

###############
# 优化构建配置 #
###############

# 仅作用于 Release 配置

# 启用 LTO/IPO
option(MCBT_ENABLE_LTO "Enable link-time optimization (LTO/IPO) for Release builds." ON)
if(MCBT_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT MCBT_IPO_SUPPORTED OUTPUT MCBT_IPO_ERROR LANGUAGES CXX)
    if(MCBT_IPO_SUPPORTED)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    else()
        message(WARNING "LTO/IPO is not supported by this toolchain, skipped: ${MCBT_IPO_ERROR}")
    endif()
endif()

option(MCBT_OPTIMIZE_FOR_SIZE "Prefer code size over speed for Release builds." OFF)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options($<$<CONFIG:Release>:-ffunction-sections> $<$<CONFIG:Release>:-fdata-sections>)
    if(APPLE)
        add_link_options($<$<CONFIG:Release>:-Wl,-dead_strip>)
    else()
        add_link_options($<$<CONFIG:Release>:-Wl,--gc-sections> $<$<CONFIG:Release>:-s>)
    endif()
    if(MCBT_OPTIMIZE_FOR_SIZE)
        add_compile_options($<$<CONFIG:Release>:-Os>)
    endif()
elseif(MSVC)
    add_compile_options($<$<CONFIG:Release>:/Gw>)
    add_link_options($<$<CONFIG:Release>:/OPT:REF> $<$<CONFIG:Release>:/OPT:ICF>)
    if(MCBT_OPTIMIZE_FOR_SIZE)
        string(REPLACE "/O2" "/O1" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
        string(REPLACE "/Ob2" "/Ob1" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    endif()
endif()

###############
# 配置第三方库 #
###############

include(FetchContent)
set(3RDPARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty")

# 配置 zlib 库
if(NOT TARGET ZLIB::ZLIB)
    set(ZLIB_BUILD_SHARED ON)
    set(ZLIB_BUILD_STATIC OFF)
    add_subdirectory("${3RDPARTY_DIR}/zlib")
endif()

# 配置 OpenCV 库
set(BUILD_SHARED_LIBS       OFF CACHE BOOL "" FORCE)
set(BUILD_WITH_STATIC_CRT   OFF CACHE BOOL "" FORCE)
set(BUILD_opencv_apps       OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES          OFF CACHE BOOL "" FORCE)
set(BUILD_DOCS              OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS             OFF CACHE BOOL "" FORCE)
set(BUILD_PERF_TESTS        OFF CACHE BOOL "" FORCE)
set(BUILD_JAVA              OFF CACHE BOOL "" FORCE)
set(BUILD_ZLIB              OFF CACHE BOOL "" FORCE)
set(BUILD_JPEG              ON  CACHE BOOL "" FORCE)
set(BUILD_PNG               ON  CACHE BOOL "" FORCE)
set(BUILD_TIFF              ON  CACHE BOOL "" FORCE)
set(BUILD_WEBP              ON  CACHE BOOL "" FORCE)
set(WITH_JPEG               ON  CACHE BOOL "" FORCE)
set(WITH_PNG                ON  CACHE BOOL "" FORCE)
set(WITH_TIFF               ON  CACHE BOOL "" FORCE)
set(WITH_WEBP               ON  CACHE BOOL "" FORCE)
set(VIDEOIO_ENABLE_PLUGINS  OFF CACHE BOOL "" FORCE)
set(WITH_MSMF               OFF CACHE BOOL "" FORCE)
set(WITH_DSHOW              OFF CACHE BOOL "" FORCE)
set(WITH_AVFOUNDATION       OFF CACHE BOOL "" FORCE)
set(WITH_FFMPEG             OFF CACHE BOOL "" FORCE)
if(WIN32)
    set(WITH_MSMF           ON  CACHE BOOL "" FORCE)
    set(WITH_DSHOW          ON  CACHE BOOL "" FORCE)
elseif(APPLE)
    set(WITH_AVFOUNDATION   ON  CACHE BOOL "" FORCE)
else()
    # Linux 平台上，需自行安装 ffmpeg 开发库，例如 Debian/Ubuntu 的 libavcodec-dev。
    set(WITH_FFMPEG         ON  CACHE BOOL "" FORCE)
endif()
set(WITH_GSTREAMER          OFF CACHE BOOL "" FORCE)
set(WITH_1394               OFF CACHE BOOL "" FORCE)
set(WITH_OBSENSOR           OFF CACHE BOOL "" FORCE)
set(WITH_OPENEXR            OFF CACHE BOOL "" FORCE)
set(WITH_JASPER             OFF CACHE BOOL "" FORCE)
set(WITH_OPENJPEG           OFF CACHE BOOL "" FORCE)
set(WITH_AVIF               OFF CACHE BOOL "" FORCE)
set(WITH_SPNG               OFF CACHE BOOL "" FORCE)
set(WITH_GDCM               OFF CACHE BOOL "" FORCE)
set(WITH_GDAL               OFF CACHE BOOL "" FORCE)
set(WITH_IPP                OFF CACHE BOOL "" FORCE)
set(WITH_ITT                OFF CACHE BOOL "" FORCE)
set(WITH_TBB                OFF CACHE BOOL "" FORCE)
set(WITH_EIGEN              OFF CACHE BOOL "" FORCE)
set(WITH_LAPACK             OFF CACHE BOOL "" FORCE)
set(WITH_OPENCL             OFF CACHE BOOL "" FORCE)
set(WITH_DIRECTX            OFF CACHE BOOL "" FORCE)
set(WITH_VTK                OFF CACHE BOOL "" FORCE)
set(WITH_PROTOBUF           OFF CACHE BOOL "" FORCE)
set(WITH_QUIRC              OFF CACHE BOOL "" FORCE)
set(WITH_FLATBUFFERS        OFF CACHE BOOL "" FORCE)
set(WITH_CAROTENE           OFF CACHE BOOL "" FORCE)
add_subdirectory("${3RDPARTY_DIR}/opencv")
if(APPLE AND TARGET libpng)
    # 新版 macOS SDK 下 clang 会预定义 TARGET_OS_MAC，使 pngpriv.h 误判为经典 macOS 工具链，
    # 进而 #include <fp.h>（该头文件已不存在于现代 SDK）。强制预包含 math.h 使其判定分支短路。
    target_compile_options(libpng PRIVATE -include math.h)
endif()
foreach(_mcbt_ocv_module core imgproc imgcodecs videoio)
    target_include_directories(
        opencv_${_mcbt_ocv_module}
        SYSTEM PUBLIC
        "${3RDPARTY_DIR}/opencv/modules/${_mcbt_ocv_module}/include"
        "${CMAKE_BINARY_DIR}"
    )
endforeach()

# 配置 Json 库
add_subdirectory("${3RDPARTY_DIR}/json")
# 配置 Easy Translate 库
add_subdirectory("${3RDPARTY_DIR}/easy_translate")
# 配置 MC NBT 库
set(MCNBT_USE_BUILTIN_ZLIB OFF)
add_subdirectory("${3RDPARTY_DIR}/mcnbt")
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
add_subdirectory("${3RDPARTY_DIR}/minizip-ng")

# 重定向目标文件输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")

function(copy_runtime_dependencies TARGET_NAME)
    if(WIN32)
        add_custom_command(
            TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${TARGET_NAME}>
            "$<TARGET_FILE_DIR:${TARGET_NAME}>"
            COMMAND_EXPAND_LISTS
        )
    endif()
endfunction()

if(MSVC)
    add_compile_options(/utf-8)
endif()

#############
# 添加子目录 #
#############

add_subdirectory("./backend")
add_subdirectory("./frontend")
