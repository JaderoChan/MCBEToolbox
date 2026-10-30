#pragma once

#include <components/adaptive_widget.h>

class QStackedWidget;
class SidebarWidget;

class MainWidget : public AdaptiveWidget
{
public:
    explicit MainWidget(QWidget* parent = nullptr);

protected:
    void updateText(
        LanguageManager::Language loaded = LanguageManager::instance().loadedLanguage()) override;
    void updateColor(
        ColorSchemeManager::ColorScheme loaded = ColorSchemeManager::instance().loadedColorScheme()) override;

private:
    SidebarWidget*  sidebarWidget_ = nullptr;
    QStackedWidget* stackedWidget_ = nullptr;
};
