#############
# 收集源文件 #
#############

# 配置 config 头文件。
configure_file(
    ${CMAKE_TEMPLATES_DIR}/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/config.h
    @ONLY
)

# 头文件（可用于在 Visual Studio 等 IDE 的项目文件树中显示头文件）
file(
    GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
# 源文件
file(
    GLOB_RECURSE SRCS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
set(PROJECT_SOURCES ${HEADERS} ${SRCS})

###########
# 构建程序 #
###########

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

###########
# 配置依赖 #
###########

# 配置头文件路径。
target_include_directories(
    ${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/include
)

# 链接依赖库。
target_link_libraries(
    ${PROJECT_NAME} PRIVATE
    mcnbt::mcnbt
    easy_translate
    MCBEToolboxBackend
)

###############
# 配置生成目标 #
###############

# 指定为 GUI 应用程序。
if(WIN32)
    option(KEEP_TERMINAL "Keep terminal window" ON)
    set_target_properties(
        ${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE $<NOT:$<BOOL:${KEEP_TERMINAL}>>
    )
endif()

# 程序退出后自动更新语言文件的翻译ID列表。（开发时使用，用于提取翻译ID）
option(UPDATE_TRANSLATIONS_MAPPING_FILES "Update the translations mapping files. Only for dev dump Text ID list." OFF)
target_compile_definitions(
    ${PROJECT_NAME} PRIVATE
    $<$<BOOL:${UPDATE_TRANSLATIONS_MAPPING_FILES}>:EASY_TRANSLATE_UPDATE_TRANSLATIONS_MAPPING_FILES>
)

# 配置应用资源/属性。
if(WIN32)
    # Windows
    set(APP_ICON ${APP_ICON_DIR}/app.ico)

    set(APP_MANIFEST ${CMAKE_CURRENT_BINARY_DIR}/app.manifest)
    configure_file(
        ${CMAKE_TEMPLATES_DIR}/app.manifest.in
        ${APP_MANIFEST}
        @ONLY
    )

    # 是否将 app.manifest 文件加入到资源文件中。
    option(EMBED_APP_MANIFEST "Add the app.manifest file to the rc file." OFF)
    if(EMBED_APP_MANIFEST)
        # MSVC 默认会自动嵌入一个 manifest，禁用以避免与资源文件中的 RT_MANIFEST 冲突。
        target_link_options(${PROJECT_NAME} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MANIFEST:NO>)
    endif()

    set(RC_FILE ${CMAKE_CURRENT_BINARY_DIR}/app.rc)
    configure_file(
        ${CMAKE_TEMPLATES_DIR}/app.rc.in
        ${RC_FILE}
        @ONLY
    )
    target_sources(${PROJECT_NAME} PRIVATE ${RC_FILE})
elseif(APPLE)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        message(FATAL_ERROR "Unsupported platform.")
    endif()

    # macOS
    set(APP_ICON ${APP_ICON_DIR}/app.icns)

    # Info Plist Strings
    set(
        INFO_PLIST_STRINGS_NAMES
        en.lproj
        zh-Hans.lproj
    )
    foreach(INFO_PLIST_STRINGS_NAME IN LISTS INFO_PLIST_STRINGS_NAMES)
        configure_file(
            ${CMAKE_TEMPLATES_DIR}/InfoPlistStrings/${INFO_PLIST_STRINGS_NAME}/InfoPlist.strings.in
            ${CMAKE_CURRENT_BINARY_DIR}/${INFO_PLIST_STRINGS_NAME}/InfoPlist.strings
            @ONLY
        )
    endforeach()
    foreach(INFO_PLIST_STRINGS_NAME IN LISTS INFO_PLIST_STRINGS_NAMES)
        set(INFO_PLIST_STRINGS_FILE ${CMAKE_CURRENT_BINARY_DIR}/${INFO_PLIST_STRINGS_NAME}/InfoPlist.strings)
        set_source_files_properties(
            ${INFO_PLIST_STRINGS_FILE} PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources/${INFO_PLIST_STRINGS_NAME}"
        )
        target_sources(${PROJECT_NAME} PRIVATE ${INFO_PLIST_STRINGS_FILE})
    endforeach()

    # Info Plist
    set(MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/Info.plist)
    configure_file(
        ${CMAKE_TEMPLATES_DIR}/Info.plist.in
        ${MACOSX_BUNDLE_INFO_PLIST}
        @ONLY
    )

    # 应用图标
    set_source_files_properties(
        ${APP_ICON} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(${PROJECT_NAME} PRIVATE ${APP_ICON})

    set_target_properties(
        ${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_INFO_PLIST ${MACOSX_BUNDLE_INFO_PLIST}
    )

    # 语言文件
    file(GLOB LANG_FILES CONFIGURE_DEPENDS ${LANGUAGES_DIR}/*)
    set_source_files_properties(
        ${LANG_FILES} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources/languages"
    )
    target_sources(${PROJECT_NAME} PRIVATE ${LANG_FILES})
elseif(LINUX)
    # TODO
else()
    message(FATAL_ERROR "Unsupported platform.")
endif()

#################
# 生成后额外命令 #
#################

# 复制语言文件至输出目录。
if(WIN32 OR LINUX)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${LANGUAGES_DIR} $<TARGET_FILE_DIR:${PROJECT_NAME}>/languages
    )
endif()

# macOS DMG 打包
option(CREATE_DMG "Create DMG for macOS, create-dmg is required." OFF)
if(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND CREATE_DMG)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND /bin/sh ${SCRIPTS_DIR}/create_dmg.sh
        ${PROJECT_NAME}
        ${APP_ICON}
        $<PATH:GET_PARENT_PATH,$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>>
    )
endif()
