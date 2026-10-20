#include "color_scheme_manager.h"

#include <qapplication.h>
#include <qobject.h>
#include <qstylehints.h>

ColorSchemeManager& ColorSchemeManager::getInstance()
{
    static ColorSchemeManager instance;
    return instance;
}

ColorSchemeManager::ColorScheme ColorSchemeManager::getSystemColorScheme()
{
    auto* hints = qApp->styleHints();
    const Qt::ColorScheme overridden = hints->colorScheme();

    // 若当前 styleHints 已被显式覆盖（非跟随系统模式），需暂时恢复为未知以读取真实系统值。
    Qt::ColorScheme system = overridden;
    if (overridden != Qt::ColorScheme::Unknown)
    {
        const QSignalBlocker blocker(hints);
        hints->setColorScheme(Qt::ColorScheme::Unknown);
        system = hints->colorScheme();
        hints->setColorScheme(overridden);
    }

    switch (system)
    {
        case Qt::ColorScheme::Light: return COLOR_SCHEME_LIGHT;
        case Qt::ColorScheme::Dark:  return COLOR_SCHEME_DARK;
        default:                     return FALLBACK_COLOR_SCHEME;
    }
}

void ColorSchemeManager::setColorScheme(ColorScheme colorScheme)
{
    if (colorScheme_ != colorScheme)
    {
        colorScheme_ = colorScheme;
        const ColorScheme loadedColorScheme = colorScheme_ == COLOR_SCHEME_AUTO ? getSystemColorScheme() : colorScheme_;
        if (loadedColorScheme_ != loadedColorScheme)
        {
            loadedColorScheme_ = loadedColorScheme;
            loadColorScheme();
            emit loadedColorSchemeChanged(loadedColorScheme_);
        }
        emit colorSchemeChanged(colorScheme_);
    }
}

ColorSchemeManager::ColorScheme ColorSchemeManager::getColorScheme() const
{
    return colorScheme_;
}

ColorSchemeManager::ColorScheme ColorSchemeManager::getLoadedColorScheme() const
{
    return loadedColorScheme_;
}

ColorSchemeManager::ColorSchemeManager()
    : QObject()
{
    connect(
        qApp->styleHints(), &QStyleHints::colorSchemeChanged, this,
        [this](Qt::ColorScheme)
        {
            if (colorScheme_ != COLOR_SCHEME_AUTO)
                return;

            const ColorScheme loadedColorScheme = getSystemColorScheme();
            if (loadedColorScheme_ != loadedColorScheme)
            {
                loadedColorScheme_ = loadedColorScheme;
                loadColorScheme();
                emit loadedColorSchemeChanged(loadedColorScheme_);
            }
        }
    );

    colorScheme_       = COLOR_SCHEME_AUTO;
    loadedColorScheme_ = getSystemColorScheme();
    loadColorScheme();
}

void ColorSchemeManager::loadColorScheme()
{
    auto* hints = qApp->styleHints();

    // 跟随系统时必须清除显式覆盖，否则 Qt 会停止追踪系统主题变化。
    if (colorScheme_ == COLOR_SCHEME_AUTO)
    {
        hints->setColorScheme(Qt::ColorScheme::Unknown);
    }
    else
    {
        switch (loadedColorScheme_)
        {
            case COLOR_SCHEME_LIGHT: hints->setColorScheme(Qt::ColorScheme::Light); break;
            case COLOR_SCHEME_DARK:  hints->setColorScheme(Qt::ColorScheme::Dark);  break;
            default:                 return;
        }
    }

    QEvent e(QEvent::StyleChange);
    qApp->sendEvent(qApp, &e);
}
