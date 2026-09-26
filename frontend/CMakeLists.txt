cmake_minimum_required(VERSION 3.26)

project(MCBEToolbox VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 默认使用 Release 构建
if(NOT DEFINED CMAKE_BUILD_TYPE OR NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build with Release when not build type be specified." FORCE)
endif()

# 查找 Qt 库
find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} 6.5.0 REQUIRED COMPONENTS Widgets)

###############
# 设置程序信息 #
###############

set(APP_ORGANIZATION            "JaderoChan")
set(APP_ORGANIZATION_DOMAIN     "io.github.jaderochan")
set(APP_TITLE                   "MCBE Toolbox")
set(APP_VERSION_MAJOR           ${PROJECT_VERSION_MAJOR})
set(APP_VERSION_MINOR           ${PROJECT_VERSION_MINOR})
set(APP_VERSION_PATCH           ${PROJECT_VERSION_PATCH})
set(APP_VERSION                 "${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH}")
set(APP_AUTHOR                  "頔珞 JaderoChan")
set(APP_CONTACT_EMAIL           "c_dl_cn@outlook.com")
set(APP_COPYRIGHT_TEXT          "Copyright (c) 2026 JaderoChan")

###############
# 定义常用路径 #
###############

# CMake 文件夹
set(CMAKE_DIR                   "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
# CMake 模板文件夹
set(CMAKE_TEMPLATES_DIR         "${CMAKE_DIR}/templates")
# 脚本文件夹路径
set(SCRIPTS_DIR                 "${CMAKE_CURRENT_SOURCE_DIR}/scripts")
# 语言文件夹路径
set(LANGUAGES_DIR               "${CMAKE_CURRENT_SOURCE_DIR}/languages")
# 数据文件夹路径
set(DATA_DIR                    "${CMAKE_SOURCE_DIR}/data")
# 资源文件夹路径
set(RESOURCES_DIR               "${CMAKE_CURRENT_SOURCE_DIR}/resources")
# 应用图标文件夹路径
set(APP_ICON_DIR                "${CMAKE_CURRENT_SOURCE_DIR}/icon")

# 重定向目标文件输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")

#############
# 添加子目录 #
#############

add_subdirectory("./src")
