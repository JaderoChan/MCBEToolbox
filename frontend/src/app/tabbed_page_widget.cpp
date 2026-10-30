#include "tabbed_page_widget.h"

#include <qbuttongroup.h>
#include <qlayout.h>
#include <qstackedwidget.h>
#include <qtoolbutton.h>

TabbedPageWidget::TabbedPageWidget(QWidget* parent)
    : AdaptiveWidget(parent)
{
    auto* headerContainer = new QWidget(this);
    headerLayout_ = new QHBoxLayout(headerContainer);
    headerLayout_->setContentsMargins(0, 0, 0, 0);
    headerLayout_->setSpacing(2);
    headerLayout_->addStretch();
    headerLayout_->addStretch();

    buttonGroup_ = new QButtonGroup(this);
    buttonGroup_->setExclusive(true);

    stackWidget_ = new QStackedWidget(this);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(headerContainer);
    mainLayout->addWidget(stackWidget_, 1);
}

int TabbedPageWidget::addTab(QWidget* widget, const QString& label, const QIcon& icon)
{
    return insertTab(count(), widget, label, icon);
}

int TabbedPageWidget::insertTab(int index, QWidget* widget, const QString& label, const QIcon& icon)
{
    if (!widget)
        return -1;

    index = qBound(0, index, count());

    auto* button = new QToolButton(this);
    button->setCheckable(true);
    button->setAutoRaise(true);
    button->setText(label);
    button->setIcon(icon);
    connect(button, &QToolButton::clicked, this, [this, button]() { setCurrentIndex(headers_.indexOf(button)); });

    headerLayout_->insertWidget(index + 1, button);
    headers_.insert(index, button);
    buttonGroup_->addButton(button);

    stackWidget_->insertWidget(index, widget);

    if (headers_.size() == 1)
        setCurrentIndex(index);

    return index;
}

void TabbedPageWidget::removeTab(int index)
{
    if (index < 0 || index >= headers_.size())
        return;

    const bool isCurrent = (index == currentIndex());

    auto* button = headers_.takeAt(index);
    headerLayout_->removeWidget(button);
    buttonGroup_->removeButton(button);
    button->deleteLater();

    stackWidget_->removeWidget(stackWidget_->widget(index));

    if (headers_.isEmpty())
        return;

    if (isCurrent)
        setCurrentIndex(qMin(index, headers_.size() - 1));
}

void TabbedPageWidget::clearTabs()
{
    while (!headers_.isEmpty())
        removeTab(0);
}

void TabbedPageWidget::setTabText(int index, const QString& text)
{
    if (auto* button = headers_.value(index))
        button->setText(text);
}

void TabbedPageWidget::setTabIcon(int index, const QIcon& icon)
{
    if (auto* button = headers_.value(index))
        button->setIcon(icon);
}

void TabbedPageWidget::setTabToolTip(int index, const QString& text)
{
    if (auto* button = headers_.value(index))
        button->setToolTip(text);
}

QWidget* TabbedPageWidget::tabHeader(int index) const
{
    return headers_.value(index);
}

QWidget* TabbedPageWidget::tabWidget(int index) const
{
    return stackWidget_->widget(index);
}

void TabbedPageWidget::setCurrentIndex(int index)
{
    if (index < 0 || index >= headers_.size())
        return;

    const bool changed = index != stackWidget_->currentIndex() || !headers_[index]->isChecked();

    stackWidget_->setCurrentIndex(index);
    headers_[index]->setChecked(true);

    if (changed)
        emit currentIndexChanged(index);
}

void TabbedPageWidget::setCurrentWidget(QWidget* widget)
{
    setCurrentIndex(indexOf(widget));
}

int TabbedPageWidget::currentIndex() const
{
    return stackWidget_->currentIndex();
}

QWidget* TabbedPageWidget::currentHeader() const
{
    const int index = stackWidget_->currentIndex();
    return index == -1 ? nullptr : headers_[index];
}

QWidget* TabbedPageWidget::currentWidget() const
{
    return stackWidget_->currentWidget();
}

int TabbedPageWidget::count() const
{
    return headers_.size();
}

int TabbedPageWidget::indexOf(QWidget* widget) const
{
    return stackWidget_->indexOf(widget);
}
