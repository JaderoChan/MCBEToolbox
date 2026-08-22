cmake_minimum_required(VERSION 3.26)

project(MCBEToolBox VERSION 0.1.0 LANGUAGES CXX)
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

# 配置 MC NBT 库
FetchContent_Declare(
    mcnbt
    SOURCE_DIR ${3RDPARTY_DIR}/mcnbt
)
FetchContent_MakeAvailable(mcnbt)

# 配置 Easy Translate 库
FetchContent_Declare(
    easy_translate
    SOURCE_DIR ${3RDPARTY_DIR}/easy_translate
)
FetchContent_MakeAvailable(easy_translate)

# 重定向目标文件输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

#############
# 添加子目录 #
#############

add_subdirectory(backend)
add_subdirectory(frontend)
