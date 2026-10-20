#include "nbt_editor_widget.h"

#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qlayout.h>
#include <qmessagebox.h>
#include <qpushbutton.h>
#include <qtabwidget.h>
#include <qtoolbutton.h>

#include <easy_translate.hpp>

#include <mcnbt/mcnbt.hpp>

#include <components/code_text_edit.h>
#include "nbt_editor_tab_page.h"
#include "nbt_tree_view.h"

namespace
{

// 用于输入 SNBT 文本进行解析的对话框。
class SnbtInputDialog : public TrDialog
{
public:
    explicit SnbtInputDialog(QWidget* parent = nullptr)
        : TrDialog(parent)
    {
        textEdit_ = new CodeTextEdit(this);

        confirmButton_ = new QPushButton(this);
        cancelButton_  = new QPushButton(this);
        confirmButton_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        cancelButton_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        connect(confirmButton_, &QPushButton::clicked, this, &QDialog::accept);
        connect(cancelButton_,  &QPushButton::clicked, this, &QDialog::reject);

        auto* buttonWidget = new QWidget(this);
        auto* buttonLayout = new QHBoxLayout(buttonWidget);
        buttonLayout->setContentsMargins(3, 0, 3, 0);
        buttonLayout->setSpacing(6);
        buttonLayout->addWidget(confirmButton_);
        buttonLayout->addWidget(cancelButton_);

        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->addWidget(textEdit_);
        mainLayout->addWidget(buttonWidget);
        resize(600, 480);

        updateText();
    }

    QString text() const
    {
        return textEdit_->toPlainText();
    }

protected:
    void updateText() override
    {
        setWindowTitle(EASYTR("Parse SNBT"));
        confirmButton_->setText(EASYTR("Confirm"));
        cancelButton_->setText(EASYTR("Cancel"));
    }

private:
    CodeTextEdit* textEdit_      = nullptr;
    QPushButton*  confirmButton_ = nullptr;
    QPushButton*  cancelButton_  = nullptr;
};

} // namespace

NbtEditorWidget::NbtEditorWidget(QWidget* parent)
    : TrWidget(parent)
{
    // 构建顶部工具栏 UI。
    auto* toolbarWidget = new QWidget(this);
    auto* toolbarLayout = new QHBoxLayout(toolbarWidget);
    toolbarLayout->setContentsMargins(3, 0, 3, 0);
    toolbarLayout->setSpacing(3);

    newButton_       = new QToolButton(this);
    openButton_      = new QToolButton(this);
    saveButton_      = new QToolButton(this);
    saveAsButton_    = new QToolButton(this);
    parseSnbtButton_ = new QToolButton(this);
    for (auto& button : {newButton_, openButton_, saveButton_, saveAsButton_, parseSnbtButton_})
    {
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        button->setFocusPolicy(Qt::ClickFocus);
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        button->setFixedHeight(24);

        toolbarLayout->addWidget(button);
    }
    toolbarLayout->addStretch();

    connect(newButton_,       &QToolButton::clicked, this, &NbtEditorWidget::newNbt);
    connect(openButton_,      &QToolButton::clicked, this, &NbtEditorWidget::openNbt);
    connect(saveButton_,      &QToolButton::clicked, this, &NbtEditorWidget::saveCurrentPage);
    connect(saveAsButton_,    &QToolButton::clicked, this, &NbtEditorWidget::saveCurrentPageAs);
    connect(parseSnbtButton_, &QToolButton::clicked, this, &NbtEditorWidget::parseSnbt);

    // 标签页容器。
    tabWidget_ = new QTabWidget(this);
    tabWidget_->setTabsClosable(true);
    tabWidget_->setMovable(true);
    connect(
        tabWidget_, &QTabWidget::tabCloseRequested,
        this, [this](int index)
        {
            auto* page = tabWidget_->widget(index);
            tabWidget_->removeTab(index);
            page->deleteLater();
        }
    );
    connect(tabWidget_, &QTabWidget::currentChanged, this, &NbtEditorWidget::updateButtonState);

    // 主布局。
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(3);
    mainLayout->addWidget(toolbarWidget);
    mainLayout->addWidget(tabWidget_);

    updateButtonState();
    updateText();
}

void NbtEditorWidget::updateText()
{
    newButton_->setText(EASYTR("New"));
    openButton_->setText(EASYTR("Open"));
    saveButton_->setText(EASYTR("Save"));
    saveAsButton_->setText(EASYTR("Save As"));
    parseSnbtButton_->setText(EASYTR("Parse SNBT"));
}

void NbtEditorWidget::newNbt()
{
    auto* page = new NbtEditorTabPage(this);
    page->nbtView()->setRootTag(nbt::Tag::compound());
    addPage(page, EASYTR("Untitled"));
}

void NbtEditorWidget::openNbt()
{
    const auto filePath =
        QFileDialog::getOpenFileName(
            this,
            EASYTR("Select File"),
            QString(),
            QStringLiteral("%1 (*.dat *.db *.mcstructure *.nbt *.schem *.schematic);;%2 (*)")
                .arg(EASYTR("NBT File"))
                .arg(EASYTR("All File"))
        );
    if (filePath.isEmpty())
        return;

    auto* page = NbtEditorTabPage::createFromFile(filePath, this);
    if (!page)
        return;

    addPage(page, QFileInfo(filePath).fileName());
}

void NbtEditorWidget::saveCurrentPage()
{
    if (auto* page = currentPage())
        page->saveNbt();
}

void NbtEditorWidget::saveCurrentPageAs()
{
    if (auto* page = currentPage())
        page->saveNbtAs();
}

void NbtEditorWidget::parseSnbt()
{
    SnbtInputDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    nbt::Tag tag;
    try
    {
        tag = nbt::Tag::fromSnbt(dialog.text().toStdString());
    }
    catch (...)
    {
        QMessageBox(
            QMessageBox::Critical,
            EASYTR("Error"),
            EASYTR("Can't parse the SNBT"),
            QMessageBox::Close,
            this
        ).exec();
        return;
    }

    auto* page = new NbtEditorTabPage(this);
    page->nbtView()->setRootTag(std::move(tag));
    addPage(page, EASYTR("Untitled"));
}

void NbtEditorWidget::addPage(NbtEditorTabPage* page, const QString& title)
{
    const int index = tabWidget_->addTab(page, title);
    connect(page->nbtView(), &NbtTreeView::tagChanged, this, &NbtEditorWidget::updateButtonState);
    connect(
        page, &NbtEditorTabPage::filePathChanged,
        this, [this, page](const QString& filePath)
        {
            const int index = tabWidget_->indexOf(page);
            if (index >= 0)
                tabWidget_->setTabText(index, filePath.isEmpty() ? EASYTR("Untitled") : QFileInfo(filePath).fileName());
        }
    );
    tabWidget_->setCurrentIndex(index);
}

void NbtEditorWidget::updateButtonState()
{
    auto*      page    = currentPage();
    const bool enabled = page && page->hasValidRootTag();
    saveButton_->setEnabled(enabled);
    saveAsButton_->setEnabled(enabled);
}

NbtEditorTabPage* NbtEditorWidget::currentPage() const
{
    return static_cast<NbtEditorTabPage*>(tabWidget_->currentWidget());
}
