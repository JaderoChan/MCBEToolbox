#############
# 收集源文件 #
#############

# 配置 config 头文件。
configure_file(
    "${CMAKE_TEMPLATES_DIR}/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/config.h"
    @ONLY
)

# 头文件（可用于在 Visual Studio 等 IDE 的项目文件树中显示头文件）
set(HEADERS
    "${CMAKE_CURRENT_BINARY_DIR}/include/config.h"
    ./app/block_table_widget.h
    ./app/color_scheme_manager.h
    ./app/language_manager.h
    ./app/logo_icon.h
    ./app/mainwidget.h
    ./app/settings.h
    ./app/trwidget.h
    ./components/sidebar_widget.h
    ./utils/debug_output.h
    ./utils/file_io.h
    ./utils/format_string.h
)

# 源文件
set(SRCS
    ./app/block_table_widget.cpp
    ./app/color_scheme_manager.cpp
    ./app/language_manager.cpp
    ./app/logo_icon.cpp
    ./app/main.cpp
    ./app/mainwidget.cpp
    ./app/settings.cpp
    ./app/trwidget.cpp
    ./components/sidebar_widget.cpp
)

# UI 文件
set(UIS
)

# QRC 文件
set(QRC "${RESOURCES_DIR}/app.qrc")

set(PROJECT_SOURCES ${HEADERS} ${SRCS} ${UIS} "${QRC}")

###########
# 构建程序 #
###########

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_standard_project_setup()
    qt_add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
else()
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)
    add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
endif()

###########
# 配置依赖 #
###########

# 配置头文件路径。
target_include_directories(
    ${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/include"
)

# 链接依赖库。
target_link_libraries(
    ${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    easy_translate::easy_translate
    MCBEToolboxBackend
)

###############
# 配置生成目标 #
###############

# 如果为 MSVC 编译器，显式指定编码，防止乱码情况。
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

# 指定为 GUI 应用程序。
if(WIN32)
    option(KEEP_TERMINAL "Keep terminal window" OFF)
    set_target_properties(
        ${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE $<NOT:$<BOOL:${KEEP_TERMINAL}>>
    )
endif()

# 程序退出后自动更新语言文件的翻译ID列表。（开发时使用，用于提取翻译ID）
option(UPDATE_TRANSLATION_MAPPING_FILE "Update the translation mapping files. Only for dev dump Text ID list." OFF)
target_compile_definitions(
    ${PROJECT_NAME} PRIVATE
    $<$<BOOL:${UPDATE_TRANSLATION_MAPPING_FILE}>:EASY_TRANSLATE_UPDATE_TRANSLATION_MAPPING_FILE>
)

# 配置应用资源/属性。
if(WIN32)
    # Windows
    set(APP_ICON "${APP_ICON_DIR}/app.ico")

    set(APP_MANIFEST "${CMAKE_CURRENT_BINARY_DIR}/app.manifest")
    configure_file(
        "${CMAKE_TEMPLATES_DIR}/app.manifest.in"
        "${APP_MANIFEST}"
        @ONLY
    )

    # 是否将 app.manifest 文件加入到资源文件中。
    option(EMBED_APP_MANIFEST "Add the app.manifest file to the rc file." OFF)
    if(EMBED_APP_MANIFEST)
        # MSVC 默认会自动嵌入一个 manifest，禁用以避免与资源文件中的 RT_MANIFEST 冲突。
        target_link_options(${PROJECT_NAME} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MANIFEST:NO>)
    endif()

    set(RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/app.rc")
    configure_file(
        "${CMAKE_TEMPLATES_DIR}/app.rc.in"
        "${RC_FILE}"
        @ONLY
    )
    target_sources(${PROJECT_NAME} PRIVATE "${RC_FILE}")
elseif(APPLE)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        message(FATAL_ERROR "Unsupported platform.")
    endif()

    # macOS
    set(APP_ICON "${APP_ICON_DIR}/app.icns")

    # Info Plist Strings
    set(
        INFO_PLIST_STRINGS_NAMES
        en.lproj
        zh-Hans.lproj
    )
    foreach(INFO_PLIST_STRINGS_NAME IN LISTS INFO_PLIST_STRINGS_NAMES)
        configure_file(
            "${CMAKE_TEMPLATES_DIR}/InfoPlistStrings/${INFO_PLIST_STRINGS_NAME}/InfoPlist.strings.in"
            "${CMAKE_CURRENT_BINARY_DIR}/${INFO_PLIST_STRINGS_NAME}/InfoPlist.strings"
            @ONLY
        )
    endforeach()
    foreach(INFO_PLIST_STRINGS_NAME IN LISTS INFO_PLIST_STRINGS_NAMES)
        set(INFO_PLIST_STRINGS_FILE "${CMAKE_CURRENT_BINARY_DIR}/${INFO_PLIST_STRINGS_NAME}/InfoPlist.strings")
        set_source_files_properties(
            "${INFO_PLIST_STRINGS_FILE}" PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources/${INFO_PLIST_STRINGS_NAME}"
        )
        target_sources(${PROJECT_NAME} PRIVATE "${INFO_PLIST_STRINGS_FILE}")
    endforeach()

    # Info Plist
    set(MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist")
    configure_file(
        "${CMAKE_TEMPLATES_DIR}/Info.plist.in"
        "${MACOSX_BUNDLE_INFO_PLIST}"
        @ONLY
    )

    # 应用图标
    set_source_files_properties(
        ${APP_ICON} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(${PROJECT_NAME} PRIVATE "${APP_ICON}")

    set_target_properties(
        ${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_INFO_PLIST "${MACOSX_BUNDLE_INFO_PLIST}"
    )

    # 语言文件
    file(GLOB LANG_FILES CONFIGURE_DEPENDS "${LANGUAGES_DIR}/*")
    set_source_files_properties(
        ${LANG_FILES} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources/languages"
    )
    target_sources(${PROJECT_NAME} PRIVATE ${LANG_FILES})

    # 数据文件
    file(GLOB_RECURSE DATA_FILES CONFIGURE_DEPENDS "${DATA_DIR}/*")
    foreach(DATA_FILE IN LISTS DATA_FILES)
        file(RELATIVE_PATH DATA_FILE_REL_PATH "${DATA_DIR}" "${DATA_FILE}")
        get_filename_component(DATA_FILE_REL_DIR "${DATA_FILE_REL_PATH}" DIRECTORY)
        if(DATA_FILE_REL_DIR)
            set_source_files_properties(
                "${DATA_FILE}" PROPERTIES
                MACOSX_PACKAGE_LOCATION "Resources/data/${DATA_FILE_REL_DIR}"
            )
        else()
            set_source_files_properties(
                "${DATA_FILE}" PROPERTIES
                MACOSX_PACKAGE_LOCATION "Resources/data"
            )
        endif()
        target_sources(${PROJECT_NAME} PRIVATE "${DATA_FILE}")
    endforeach()
elseif(LINUX)
    # TODO
else()
    message(FATAL_ERROR "Unsupported platform.")
endif()

#################
# 生成后额外命令 #
#################

# 复制运行时依赖文件至目标目录
if(WIN32)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMAND_EXPAND_LISTS
    )
endif()

# 复制语言文件至目标目录。
if(WIN32 OR LINUX)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${LANGUAGES_DIR}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/languages"
    )
endif()

# 复制数据文件至目标目录。
if(WIN32 OR LINUX)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${DATA_DIR}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/data"
    )
endif()

# Qt 部署（需要保证 *deployqt 可见）
option(DEPLOYQT_AFTER_BUILD "Execute *deployqt program to install Qt dependencies after build." ON)
if(DEPLOYQT_AFTER_BUILD)
    if(WIN32)
        add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND windeployqt
            $<$<CONFIG:Debug>:--debug>$<$<NOT:$<CONFIG:Debug>>:--release>
            "$<TARGET_FILE:${PROJECT_NAME}>"
        )
    elseif(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND macdeployqt
            $<$<CONFIG:Debug>:--debug>$<$<NOT:$<CONFIG:Debug>>:--release>
            "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>"
            -codesign=-
        )
    elseif(LINUX)
        # TODO
    else()
        message(FATAL_ERROR "Unsupported platform.")
    endif()
endif()

# macOS DMG 打包
option(CREATE_DMG "Create DMG for macOS, create-dmg is required." OFF)
if(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND CREATE_DMG)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND /bin/sh "${SCRIPTS_DIR}/create_dmg.sh"
        ${PROJECT_NAME}
        "${APP_ICON}"
        "$<PATH:GET_PARENT_PATH,$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>>"
    )
endif()
