#include <qapplication.h>
#include <qfont.h>
#include <qsettings.h>
#include <qstringlist.h>

#include <easy_translate.hpp>

#include <app_defines.h>
#include "app_configuration.h"
#include "app_logo_icon.h"
#include "mainwidget.h"

int main(int argc, char* argv[])
{
    qRegisterMetaType<LanguageManager::Language>("Language");
    qRegisterMetaType<ColorSchemeManager::ColorScheme>("ColorScheme");

    // 设置程序全局属性。
    QApplication a(argc, argv);
    a.setOrganizationDomain(APP_ORGANIZATION_DOMAIN);
    a.setOrganizationName(APP_ORGANIZATION);
    a.setApplicationName(APP_TITLE);
    a.setApplicationVersion(APP_VERSION);
#ifndef Q_OS_MAC
    a.setWindowIcon(getAppLogoIcon());
#else
    // 启用长按键重复功能。
    QSettings(QSettings::NativeFormat, QSettings::UserScope, QStringLiteral(APP_BUNDLE_IDENTIFIER))
        .setValue(QStringLiteral("ApplePressAndHoldEnabled"), false);
#endif // Q_OS_MAC

    // 设置字体
    {
        QFont       font;
        QStringList families;

    #ifdef Q_OS_WIN
        families << "Microsoft YaHei" << "SimHei";
    #elif defined(Q_OS_MACOS)
        families << "PingFang SC" << "Heiti SC";
    #else
        families << "Noto Sans CJK SC" << "WenQuanYi Micro Hei";
    #endif // Q_OS_WIN

        families << "Arial" << "sans-serif";

        font.setFamilies(families);
        a.setFont(font);
    }

    // 加载程序设置。
    setAppConfiguration(loadConfiguration());

    // 设置语言与色彩主题。
    LanguageManager::instance().setLanguage(getAppConfiguration().language);
    ColorSchemeManager::instance().setColorScheme(getAppConfiguration().colorScheme);

    MainWidget w;
    w.show();

    int ret = a.exec();

    // 更新翻译文件（实际上由编译选项 UPDATE_TRANSLATION_MAPPING_FILE 决定是否真正更新）。
    easytr::updateTranslationMappingFile();

    return ret;
}
