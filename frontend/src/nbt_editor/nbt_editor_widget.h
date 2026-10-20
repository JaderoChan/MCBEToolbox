#pragma once

#include <components/trwidget.h>

class QTabWidget;
class QToolButton;
class NbtEditorTabPage;

class NbtEditorWidget : public TrWidget
{
public:
    explicit NbtEditorWidget(QWidget* parent = nullptr);

protected:
    void updateText() override;

private:
    void newNbt();
    void openNbt();
    void saveCurrentPage();
    void saveCurrentPageAs();
    void parseSnbt();

    void addPage(NbtEditorTabPage* page, const QString& title);
    void removePage(int index);
    void updateButtonState();

    NbtEditorTabPage* currentPage() const;

    QToolButton* newButton_       = nullptr;
    QToolButton* openButton_      = nullptr;
    QToolButton* saveButton_      = nullptr;
    QToolButton* saveAsButton_    = nullptr;
    QToolButton* parseSnbtButton_ = nullptr;

    QTabWidget*  tabWidget_       = nullptr;
};
