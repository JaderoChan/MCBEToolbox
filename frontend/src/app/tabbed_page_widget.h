#pragma once

#include <qicon.h>
#include <qlist.h>
#include <qstring.h>

#include <components/adaptive_widget.h>

class QButtonGroup;
class QHBoxLayout;
class QStackedWidget;
class QToolButton;

class TabbedPageWidget : public AdaptiveWidget
{
    Q_OBJECT

public:
    explicit TabbedPageWidget(QWidget* parent = nullptr);

    int  addTab(QWidget* widget, const QString& label = QString(), const QIcon& icon = QIcon());
    int  insertTab(int index, QWidget* widget, const QString& label = QString(), const QIcon& icon = QIcon());
    void removeTab(int index);
    void clearTabs();

    void setTabText(int index, const QString& text);
    void setTabIcon(int index, const QIcon& icon);
    void setTabToolTip(int index, const QString& text);

    QWidget* tabHeader(int index) const;
    QWidget* tabWidget(int index) const;

    void     setCurrentIndex(int index);
    void     setCurrentWidget(QWidget* widget);
    int      currentIndex()  const;
    QWidget* currentHeader() const;
    QWidget* currentWidget() const;

    int count() const;
    int indexOf(QWidget* widget) const;

signals:
    void currentIndexChanged(int index);

private:
    QHBoxLayout*        headerLayout_ = nullptr;
    QButtonGroup*       buttonGroup_  = nullptr;
    QStackedWidget*     stackWidget_  = nullptr;
    QList<QToolButton*> headers_;
};

