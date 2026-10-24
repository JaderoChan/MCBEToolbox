#pragma once

#include <qdialog.h>
#include <qmainwindow.h>
#include <qwidget.h>

#include <easy_translate.hpp>

#include "color_scheme_manager.h"
#include "language_manager.h"

/** 自动监听语言/颜色模式变化并调用可重写的 updateText()/updateColor() 虚函数的 QWidget 基类。 */
class MyWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MyWidget(QWidget* parent = nullptr);

public slots:
    /** \p loaded 不可能是 #LANGUAGE_AUTO。 */
    virtual void updateText(
        LanguageManager::Language loaded = LanguageManager::instance().loadedLanguage());
    /** \p loaded 不可能是 #COLOR_SCHEME_AUTO。 */
    virtual void updateColor(
        ColorSchemeManager::ColorScheme loaded = ColorSchemeManager::instance().loadedColorScheme());
};

/** 自动监听语言/颜色模式变化并调用可重写的 updateText()/updateColor() 虚函数的 QMainWindow 基类。 */
class MyMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MyMainWindow(QWidget* parent = nullptr);

public slots:
    /** \p loaded 不可能是 #LANGUAGE_AUTO。 */
    virtual void updateText(
        LanguageManager::Language loaded = LanguageManager::instance().loadedLanguage());
    /** \p loaded 不可能是 #COLOR_SCHEME_AUTO。 */
    virtual void updateColor(
        ColorSchemeManager::ColorScheme loaded = ColorSchemeManager::instance().loadedColorScheme());
};

/** 自动监听语言/颜色模式变化并调用可重写的 updateText()/updateColor() 虚函数的 QDialog 基类。 */
class MyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MyDialog(QWidget* parent = nullptr);

public slots:
    /** \p loaded 不可能是 #LANGUAGE_AUTO。 */
    virtual void updateText(
        LanguageManager::Language loaded = LanguageManager::instance().loadedLanguage());
    /** \p loaded 不可能是 #COLOR_SCHEME_AUTO。 */
    virtual void updateColor(
        ColorSchemeManager::ColorScheme loaded = ColorSchemeManager::instance().loadedColorScheme());
};
