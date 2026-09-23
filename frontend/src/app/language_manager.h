#pragma once

#include <qevent.h>
#include <qmetatype.h>
#include <qobject.h>
#include <qstring.h>

class LanguageManager : public QObject
{
    Q_OBJECT

public:
    enum Language : int
    {
        LANGUAGE_AUTO,
        LANGUAGE_EN,
        LANGUAGE_ZH
    };
    static constexpr Language LANGUAGE_FIRST    = LANGUAGE_AUTO;
    static constexpr Language LANGUAGE_LAST     = LANGUAGE_ZH;
    static constexpr Language FALLBACK_LANGUAGE = LANGUAGE_EN;

    static LanguageManager& getInstance();
    /** 获得系统语言，如果是不支持的语言返回 #FALLBACK_LANGUAGE。 */
    static Language         getSystemLanguage();

    /** 设置程序语言。 */
    void     setLanguage(Language language);
    /** 获得程序语言。 */
    Language getLanguage()       const;
    /** 获得实际加载中的语言（不包含 #LANGUAGE_AUTO）。 */
    Language getLoadedLanguage() const;

signals:
    /** 程序语言发生变化时发出。 */
    void languageChanged(Language);
    /** 仅当实际加载的语言发生变化时才发出（不包含 #LANGUAGE_AUTO）。 */
    void loadedLanguageChanged(Language);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    LanguageManager();

    LanguageManager(const LanguageManager&)           = delete;
    LanguageManager operator=(const LanguageManager&) = delete;

    static QString getLanguageId(Language language);

    void loadLanguage();

    Language language_;
    Language loadedLanguage_;
};

Q_DECLARE_METATYPE(LanguageManager::Language)
