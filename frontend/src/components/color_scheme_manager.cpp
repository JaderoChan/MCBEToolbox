#include "color_scheme_manager.h"

#include <qapplication.h>
#include <qstylehints.h>

ColorSchemeManager& ColorSchemeManager::getInstance()
{
    static ColorSchemeManager instance;
    return instance;
}

ColorSchemeManager::ColorScheme ColorSchemeManager::getSystemColorScheme()
{
    switch (QApplication::styleHints()->colorScheme())
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

bool ColorSchemeManager::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == qApp)
    {
        if (event->type() == QEvent::ThemeChange)
        {
            const ColorScheme loadedColorScheme = getSystemColorScheme();
            if (loadedColorScheme_ != loadedColorScheme)
            {
                loadedColorScheme_ = loadedColorScheme;
                loadColorScheme();
                emit loadedColorSchemeChanged(loadedColorScheme_);
            }
            return true;
        }
    }
    return false;
}

ColorSchemeManager::ColorSchemeManager()
    : QObject()
{
    qApp->installEventFilter(this);
    colorScheme_       = COLOR_SCHEME_AUTO;
    loadedColorScheme_ = getSystemColorScheme();
    loadColorScheme();
}

void ColorSchemeManager::loadColorScheme()
{
    switch (loadedColorScheme_)
    {
        case COLOR_SCHEME_LIGHT: qApp->styleHints()->setColorScheme(Qt::ColorScheme::Light); break;
        case COLOR_SCHEME_DARK:  qApp->styleHints()->setColorScheme(Qt::ColorScheme::Dark);  break;
        default:                 return;
    }

    QEvent event(QEvent::StyleChange);
    qApp->sendEvent(qApp, &event);
}
