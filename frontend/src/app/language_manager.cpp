#include "language_manager.h"

#include <qapplication.h>
#include <qdir.h>
#include <qlocale.h>

#include <easy_translate.hpp>

#include <config.h>
#include <utils/debug_output.h>

namespace
{

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

} // namespace

LanguageManager& LanguageManager::getInstance()
{
    static LanguageManager instance;
    return instance;
}

LanguageManager::Language LanguageManager::getSystemLanguage()
{
    switch (QLocale::system().language())
    {
        case QLocale::Language::English: return LANGUAGE_EN;
        case QLocale::Language::Chinese: return LANGUAGE_ZH;
        default:                         return FALLBACK_LANGUAGE;
    }
}

void LanguageManager::setLanguage(Language language)
{
    if (language_ != language)
    {
        language_ = language;
        const Language loadedLanguage = language_ == LANGUAGE_AUTO ? getSystemLanguage() : language_;
        if (loadedLanguage_ != loadedLanguage)
        {
            loadedLanguage_ = loadedLanguage;
            loadLanguage();
            emit loadedLanguageChanged(loadedLanguage_);
        }
        emit languageChanged(language_);
    }
}

LanguageManager::Language LanguageManager::getLanguage() const
{
    return language_;
}

LanguageManager::Language LanguageManager::getLoadedLanguage() const
{
    return loadedLanguage_;
}

bool LanguageManager::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == qApp)
    {
        if (event->type() == QEvent::LocaleChange && language_ == LANGUAGE_AUTO)
        {
            const Language loadedLanguage = getSystemLanguage();
            if (loadedLanguage_ != loadedLanguage)
            {
                loadedLanguage_ = loadedLanguage;
                loadLanguage();
                emit loadedLanguageChanged(loadedLanguage_);
            }
            return true;
        }
    }
    return false;
}

LanguageManager::LanguageManager()
    : QObject()
{
    qApp->installEventFilter(this);
    language_       = LANGUAGE_AUTO;
    loadedLanguage_ = getSystemLanguage();
    loadLanguage();
}

QString LanguageManager::getLanguageId(Language language)
{
    switch (language)
    {
        case LANGUAGE_EN: return "EN";
        case LANGUAGE_ZH: return "ZH";
        default:          return "";
    }
}

void LanguageManager::loadLanguage()
{
    DirectoryScope dirScope(APP_RESOURCES_DIRPATH);

    easytr::setLanguageMapping(easytr::LanguageMapping::fromFile(APP_LANGMAP_FILEPATH));
    if (easytr::languageMapping().empty())
    {
        debugOut(qWarning(), "[LanguageManager] Failed to load language mapping or language mapping is empty.");
        return;
    }

    const std::string id = getLanguageId(loadedLanguage_).toStdString();
    if (!easytr::hasLanguage(id))
    {
        debugOut(qWarning(), "[LanguageManager] Language %1 is not available.", QString::fromStdString(id));
        return;
    }

    if (!easytr::setCurrentLanguage(id))
    {
        debugOut(qWarning(), "[LanguageManager] Failed to set the language to %1.", QString::fromStdString(id));
        return;
    }

    QEvent event(QEvent::LanguageChange);
    qApp->sendEvent(qApp, &event);
}
