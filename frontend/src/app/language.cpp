#include "language.h"

#include <qapplication.h>
#include <qdir.h>
#include <qevent.h>

#include <easy_translate.hpp>

#include <config.h>
#include <utils/debug_output.h>

QString getLanguageStringId(Language lang)
{
    switch (lang)
    {
        case LANG_EN:   return "EN";
        case LANG_ZH:   return "ZH";
        default:        return "";
    }
}

Language getCurrentSystemLang()
{
    switch (QLocale::system().language())
    {
        case QLocale::Language::English:    return LANG_EN;
        case QLocale::Language::Chinese:    return LANG_ZH;
        default:                            return LANG_EN;
    }
}

class DirectoryScope
{
public:
    explicit DirectoryScope(const QString& path)
    {
        originDir_ = QDir::currentPath();
        QDir::setCurrent(path);
    }

    ~DirectoryScope()
    {
        QDir::setCurrent(originDir_);
    }

private:
    QString originDir_;
};

bool setLanguage(Language lang)
{
    {
        DirectoryScope dirScope(APP_RESOURCES_DIRPATH);
        easytr::setLanguageMapping(easytr::LanguageMapping::fromFile(APP_LANGMAP_FILEPATH));
    }

    if (easytr::languageMapping().empty())
    {
        debugOut(qWarning(), "[Language] Failed to load language mapping or language mapping is empty.");
        return false;
    }

    std::string id = getLanguageStringId(lang).toStdString();
    if (!easytr::hasLanguage(id))
    {
        debugOut(qWarning(), "[Language] Language %1 is not available.", QString::fromStdString(id));
        return false;
    }

    {
        DirectoryScope dirScope(APP_RESOURCES_DIRPATH);
        if (!easytr::setCurrentLanguage(id))
        {
            debugOut(qWarning(), "[Language] Failed to set the current language to %1.", QString::fromStdString(id));
            return false;
        }
    }

    QEvent event(QEvent::Type::LanguageChange);
    qApp->sendEvent(qApp, &event);

    return true;
}
