#include <qapplication.h>
#include <qsettings.h>

#include <easy_translate.hpp>

#include <config.h>
#include "logo_icon.h"
#include "mainwidget.h"
#include "settings.h"

#include <test/block_list_example_widget.h>
#include <test/nbt_editor_example_widget.h>

int main(int argc, char* argv[])
{
    qRegisterMetaType<LanguageManager::Language>("Language");
    qRegisterMetaType<ColorSchemeManager::ColorScheme>("ColorScheme");

    // 设置程序全局属性
    QApplication a(argc, argv);
    a.setOrganizationDomain(APP_ORGANIZATION_DOMAIN);
    a.setOrganizationName(APP_ORGANIZATION);
    a.setApplicationName(APP_TITLE);
    a.setApplicationVersion(APP_VERSION);
#ifndef Q_OS_MAC
    a.setWindowIcon(getLogoIcon());
#else
    QSettings(QSettings::NativeFormat, QSettings::UserScope, QStringLiteral(APP_BUNDLE_IDENTIFIER))
        .setValue(QStringLiteral("ApplePressAndHoldEnabled"), false);
#endif

    // 设置语言
    {
        Settings settings = loadSettings();
        LanguageManager::getInstance().setLanguage(settings.language);
    }

    BlockListExampleWidget w1;
    NbtEditorExampleWidget w2;
    w1.show();
    w2.show();

    int ret = a.exec();

    // 更新翻译文件（实际上由编译选项 UPDATE_TRANSLATION_MAPPING_FILE 决定是否真正更新）
    easytr::updateTranslationMappingFile();

    return ret;
}
