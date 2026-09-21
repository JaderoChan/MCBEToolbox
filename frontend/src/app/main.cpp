#include <qapplication.h>

#include <easy_translate.hpp>

#include <config.h>
#include "logo_icon.h"
#include "settings.h"

int main(int argc, char* argv[])
{
    qRegisterMetaType<Language>("Language");

    // 设置程序全局属性
    QApplication a(argc, argv);
    a.setOrganizationDomain(APP_ORGANIZATION_DOMAIN);
    a.setOrganizationName(APP_ORGANIZATION);
    a.setApplicationName(APP_TITLE);
    a.setApplicationVersion(APP_VERSION);
#ifndef Q_OS_MAC
    a.setWindowIcon(getLogoIcon());
#endif

    // 设置语言
    {
        Settings settings = loadSettings();
        setLanguage(settings.language);
    }

    int ret = a.exec();

    // 更新翻译文件（实际上由编译选项 UPDATE_TRANSLATION_MAPPING_FILE 决定是否真正更新）
    easytr::updateTranslationMappingFile();

    return ret;
}
