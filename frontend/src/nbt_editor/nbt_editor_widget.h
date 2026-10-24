#pragma once

#include <qstringlist.h>

#include <components/mywidget.h>

class QTabWidget;
class QToolButton;
class NbtEditorTabPage;

class NbtEditorWidget : public MyWidget
{
public:
    explicit NbtEditorWidget(QWidget* parent = nullptr);

protected:
    void updateText(
        LanguageManager::Language loaded = LanguageManager::instance().loadedLanguage()) override;
    void updateColor(
        ColorSchemeManager::ColorScheme loaded = ColorSchemeManager::instance().loadedColorScheme()) override;

    bool eventFilter(QObject* obj, QEvent* e) override;

private:
    void newNbt();
    void openNbt();
    void saveCurrentPage();
    void saveCurrentPageAs();
    void parseSnbt();

    void addPage(NbtEditorTabPage* page, const QString& title);
    void removePage(int index);
    void updateButtonState();

    void onFilesDropped(const QStringList& filePaths);

    NbtEditorTabPage* currentPage() const;

    QToolButton* newButton_       = nullptr;
    QToolButton* openButton_      = nullptr;
    QToolButton* saveButton_      = nullptr;
    QToolButton* saveAsButton_    = nullptr;
    QToolButton* parseSnbtButton_ = nullptr;

    QTabWidget*  tabWidget_       = nullptr;
};
