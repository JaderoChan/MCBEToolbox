#pragma once

#include <qdialog.h>
#include <qmainwindow.h>
#include <qwidget.h>

#include <easy_translate.hpp>

#include "color_scheme_manager.h"
#include "language_manager.h"

class MyWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MyWidget(QWidget* parent = nullptr);

public slots:
    /** \p loaded cannot be \ref LANGUAGE_AUTO. */
    virtual void updateText(
        LanguageManager::Language loaded = LanguageManager::instance().loadedLanguage());
    /** \p loaded cannot be \ref COLOR_SCHEME_AUTO. */
    virtual void updateColor(
        ColorSchemeManager::ColorScheme loaded = ColorSchemeManager::instance().loadedColorScheme());
};

class MyMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MyMainWindow(QWidget* parent = nullptr);

public slots:
    /** \p loaded cannot be \ref LANGUAGE_AUTO. */
    virtual void updateText(
        LanguageManager::Language loaded = LanguageManager::instance().loadedLanguage());
    /** \p loaded cannot be \ref COLOR_SCHEME_AUTO. */
    virtual void updateColor(
        ColorSchemeManager::ColorScheme loaded = ColorSchemeManager::instance().loadedColorScheme());
};

class MyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MyDialog(QWidget* parent = nullptr);

public slots:
    /** \p loaded cannot be \ref LANGUAGE_AUTO. */
    virtual void updateText(
        LanguageManager::Language loaded = LanguageManager::instance().loadedLanguage());
    /** \p loaded cannot be \ref COLOR_SCHEME_AUTO. */
    virtual void updateColor(
        ColorSchemeManager::ColorScheme loaded = ColorSchemeManager::instance().loadedColorScheme());
};
