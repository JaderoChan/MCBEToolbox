#pragma once

#include <components/adaptive_widget.h>

class QStackedWidget;
class SidebarWidget;

class MainWidget : public AdaptiveWidget
{
public:
    explicit MainWidget(QWidget* parent = nullptr);

protected:
    void refreshText()  override;
    void refreshColor() override;

    void closeEvent(QCloseEvent* e) override;

private:
    SidebarWidget*  sidebarWidget_ = nullptr;
    QStackedWidget* stackedWidget_ = nullptr;
};
