###############
# 设置程序信息 #
###############

set(APP_ORGANIZATION            "JaderoChan")
set(APP_ORGANIZATION_DOMAIN     "io.github.jaderochan")
set(APP_TITLE                   "MCBE Tool Box")
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
set(CMAKE_DIR                   ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
# CMake 模板文件夹
set(CMAKE_TEMPLATES_DIR         ${CMAKE_DIR}/templates)
# 脚本文件夹路径
set(SCRIPTS_DIR                 ${CMAKE_CURRENT_SOURCE_DIR}/scripts)
# 语言文件夹路径
set(LANGUAGES_DIR               ${CMAKE_CURRENT_SOURCE_DIR}/languages)
# 资源文件夹路径
set(RESOURCES_DIR               ${CMAKE_CURRENT_SOURCE_DIR}/resources)
# 应用图标文件夹路径
set(APP_ICON_DIR                ${CMAKE_CURRENT_SOURCE_DIR}/icon)

add_subdirectory(src)
