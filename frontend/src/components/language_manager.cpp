#include "language_manager.h"

#include <qapplication.h>
#include <qlocale.h>

#include <easy_translate.hpp>

#include <app_defines.h>
#include <utils/debug_output.h>
#include <utils/directory_scope.h>

LanguageManager& LanguageManager::instance()
{
    static LanguageManager instance;
    return instance;
}

LanguageManager::Language LanguageManager::systemLanguage()
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
        const auto& loadedLanguage = language_ == LANGUAGE_AUTO ? systemLanguage() : language_;
        if (loadedLanguage_ != loadedLanguage)
        {
            loadedLanguage_ = loadedLanguage;
            loadLanguage();
            emit loadedLanguageChanged(loadedLanguage_);
        }
        emit languageChanged(language_);
    }
}

LanguageManager::Language LanguageManager::language() const
{
    return language_;
}

LanguageManager::Language LanguageManager::loadedLanguage() const
{
    return loadedLanguage_;
}

bool LanguageManager::eventFilter(QObject* obj, QEvent* e)
{
    if (obj == qApp)
    {
        if (e->type() == QEvent::LocaleChange && language_ == LANGUAGE_AUTO)
        {
            const auto& loadedLanguage = systemLanguage();
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
    loadedLanguage_ = systemLanguage();
    loadLanguage();
}

QString LanguageManager::languageId(Language language)
{
    switch (language)
    {
        case LANGUAGE_EN: return QStringLiteral("EN");
        case LANGUAGE_ZH: return QStringLiteral("ZH");
        default:          return QStringLiteral("");
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

    const auto& id = languageId(loadedLanguage_).toStdString();
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

    QEvent e(QEvent::LanguageChange);
    qApp->sendEvent(qApp, &e);
}
