#include "nbt_editor_tab_page.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <tuple>

#include <qbuttongroup.h>
#include <qfiledialog.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qstackedwidget.h>
#include <qstandarditemmodel.h>
#include <qtoolbutton.h>

#include <easy_translate.hpp>

#include <components/code_text_edit.h>
#include <components/stylesheet_table.h>
#include "nbt_utils.h"
#include "snbt_syntax_highlighter.h"

namespace
{

// 用于配置保存 NBT 文件信息的对话框。
class NbtFileInfoDialog : public MyDialog
{
    Q_OBJECT

public:
    explicit NbtFileInfoDialog(QWidget* parent = nullptr)
        : MyDialog(parent)
    {
        // Endian
        endianLabel_       = new QLabel(this);
        littleEndianRadio_ = new QRadioButton(this);
        bigEndianRadio_    = new QRadioButton(this);
        littleEndianRadio_->setChecked(true);

        auto* endianButtonGroup = new QButtonGroup(this);
        endianButtonGroup->addButton(littleEndianRadio_);
        endianButtonGroup->addButton(bigEndianRadio_);

        // Compressed
        compressedLabel_   = new QLabel(this);
        uncompressedRadio_ = new QRadioButton(this);
        compressedRadio_   = new QRadioButton(this);
        uncompressedRadio_->setChecked(true);

        auto* compressedButtonGroup = new QButtonGroup(this);
        compressedButtonGroup->addButton(uncompressedRadio_);
        compressedButtonGroup->addButton(compressedRadio_);

        // File path
        filePathLabel_ = new QLabel(this);
        filePathEdit_  = new QLineEdit(this);
        browseButton_  = new QToolButton(this);
        browseButton_->setText(QStringLiteral("..."));

        connect(browseButton_, &QToolButton::clicked, this, &NbtFileInfoDialog::browseFilePath);

        auto* filePathWidget = new QWidget(this);
        auto* filePathLayout = new QHBoxLayout(filePathWidget);
        filePathLayout->setContentsMargins(0, 0, 0, 0);
        filePathLayout->addWidget(filePathEdit_);
        filePathLayout->addWidget(browseButton_);

        // Container
        auto* containerWidget = new QWidget(this);
        auto* containerLayout = new QGridLayout(containerWidget);
        containerLayout->addWidget(endianLabel_,       0, 0);
        containerLayout->addWidget(littleEndianRadio_, 0, 1);
        containerLayout->addWidget(bigEndianRadio_,    0, 2);
        containerLayout->addWidget(compressedLabel_,   1, 0);
        containerLayout->addWidget(uncompressedRadio_, 1, 1);
        containerLayout->addWidget(compressedRadio_,   1, 2);
        containerLayout->addWidget(filePathLabel_,     2, 0);
        containerLayout->addWidget(filePathWidget,     2, 1, 1, 2);

        // Button
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

        // Main layout
        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->addWidget(containerWidget);
        mainLayout->addWidget(buttonWidget);

        updateText();
    }

    void setBigEndian(bool bigEndian)
    {
        bigEndianRadio_->setChecked(bigEndian);
        littleEndianRadio_->setChecked(!bigEndian);
    }

    bool isBigEndian() const
    {
        return bigEndianRadio_->isChecked();
    }

    void setCompressed(bool compressed)
    {
        compressedRadio_->setChecked(compressed);
        uncompressedRadio_->setChecked(!compressed);
    }

    bool isCompressed() const
    {
        return compressedRadio_->isChecked();
    }

    void setFilePath(const QString& filePath) { filePathEdit_->setText(filePath); }
    QString filePath() const { return filePathEdit_->text(); }

protected:
    void updateText() override
    {
        setWindowTitle(EASYTR("Save NBT Options"));
        endianLabel_->setText(EASYTR("Endianness:"));
        bigEndianRadio_->setText(EASYTR("Big Endian"));
        littleEndianRadio_->setText(EASYTR("Little Endian"));
        compressedLabel_->setText(EASYTR("Compression:"));
        compressedRadio_->setText(EASYTR("GZip Compressed"));
        uncompressedRadio_->setText(EASYTR("Uncompressed"));
        filePathLabel_->setText(EASYTR("File Path:"));
        confirmButton_->setText(EASYTR("Confirm"));
        cancelButton_->setText(EASYTR("Cancel"));
    }

private slots:
    void browseFilePath()
    {
        const auto& filePath = QFileDialog::getSaveFileName(this, EASYTR("Select File"), filePathEdit_->text());
        if (!filePath.isEmpty())
            filePathEdit_->setText(filePath);
    }

private:
    QLabel*       endianLabel_       = nullptr;
    QRadioButton* littleEndianRadio_ = nullptr;
    QRadioButton* bigEndianRadio_    = nullptr;

    QLabel*       compressedLabel_   = nullptr;
    QRadioButton* uncompressedRadio_ = nullptr;
    QRadioButton* compressedRadio_   = nullptr;

    QLabel*       filePathLabel_     = nullptr;
    QLineEdit*    filePathEdit_      = nullptr;
    QToolButton*  browseButton_      = nullptr;

    QPushButton*  confirmButton_     = nullptr;
    QPushButton*  cancelButton_      = nullptr;
};

} // namespace

NbtEditorTabPage::NbtEditorTabPage(QWidget* parent)
    : MyWidget(parent)
{
    // 构建顶部工具栏 UI。
    auto* toolbarWidget = new QWidget(this);
    auto* toolbarLayout = new QHBoxLayout(toolbarWidget);
    toolbarLayout->setContentsMargins(3, 0, 3, 0);
    toolbarLayout->setSpacing(2);

    // 基础 Tag 操作按钮。
    renameButton_ = new QToolButton(this);
    editButton_   = new QToolButton(this);
    removeButton_ = new QToolButton(this);
    for (auto& button : {renameButton_, editButton_, removeButton_})
    {
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        button->setFocusPolicy(Qt::ClickFocus);
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        button->setFixedHeight(24);

        toolbarLayout->addWidget(button);
    }
    connect(
        removeButton_, &QToolButton::clicked,
        this, [this]() { nbtView_->removeItem(nbtView_->selectedItem());          }
    );
    connect(
        renameButton_, &QToolButton::clicked,
        this, [this]() { nbtView_->invokeEditItemKey(nbtView_->selectedItem());   }
    );
    connect(
        editButton_,   &QToolButton::clicked,
        this, [this]() { nbtView_->invokeEditItemValue(nbtView_->selectedItem()); }
    );

    toolbarLayout->addSpacing(3);

    // Tag 增添按钮。
    auto* addButtonWidget = new QWidget(this);
    auto* addButtonLayout = new QHBoxLayout(addButtonWidget);
    addButtonLayout->setContentsMargins(0, 0, 0 ,0);
    addButtonLayout->setSpacing(2);
    for (auto type : nbt_utils::addableTagTypes())
    {
        auto* button = new QToolButton(this);
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        button->setFocusPolicy(Qt::ClickFocus);
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        button->setIcon(nbt_utils::tagTypeIcon(type));
        button->setIconSize(QSize(24, 24));
        button->setStyleSheet(QStringLiteral("border: none; background: transparent; padding: 0px;"));

        addButtonLayout->addWidget(button);
        addButtons_[type] = button;

        connect(
            button, &QToolButton::clicked,
            this, [this, type]() { nbtView_->invokeAddItem(nbtView_->selectedItem(), type); }
        );
    }

    toolbarLayout->addWidget(addButtonWidget);
    toolbarLayout->addStretch();

    // SNBT 按钮。
    snbtButton_ = new QToolButton(this);
    snbtButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    snbtButton_->setFocusPolicy(Qt::ClickFocus);
    snbtButton_->setToolButtonStyle(Qt::ToolButtonTextOnly);
    snbtButton_->setFixedHeight(24);
    snbtButton_->setCheckable(true);
    snbtButton_->setStyleSheet(toolButtonStyleSheet());

    connect(snbtButton_, &QToolButton::clicked, this, &NbtEditorTabPage::togglePage);

    toolbarLayout->addWidget(snbtButton_);

    // NBT 视图。
    nbtView_ = new NbtTreeView(this);
    connect(nbtView_, &NbtTreeView::selectedItemChanged, this, &NbtEditorTabPage::updateButtonState);
    connect(nbtView_, &NbtTreeView::tagChanged, this, [this]() { needUpdateSnbt_ = true; updateButtonState(); });

    // SNBT 页面。
    snbtWidget_ = new CodeTextEdit(this);
    snbtWidget_->setReadOnly(true);
    snbtWidget_->setContextMenuPolicy(Qt::NoContextMenu);
    auto* snbtHighlighter = new SnbtSyntaxHighlighter(snbtWidget_->document());

    // 页面容器。
    stackedWidget_ = new QStackedWidget(this);
    stackedWidget_->setContentsMargins(0, 0, 0, 0);
    stackedWidget_->addWidget(nbtView_);
    stackedWidget_->addWidget(snbtWidget_);
    stackedWidget_->setCurrentIndex(0);

    // 构建底部状态栏 UI。
    auto* stateBarWidget = new QWidget(this);
    auto* stateBarLayout = new QHBoxLayout(stateBarWidget);
    auto font = stateBarWidget->font();
    font.setPointSizeF(font.pointSizeF() * 0.95);
    stateBarWidget->setFont(font);
    stateBarWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    stateBarLayout->setContentsMargins(3, 0, 3, 0);
    stateBarLayout->setSpacing(18);

    filePathLabel_   = new QLabel(this);
    endianLabel_     = new QLabel(this);
    compressedLabel_ = new QLabel(this);
    filePathLabel_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    endianLabel_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    compressedLabel_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    stateBarLayout->addWidget(filePathLabel_);
    stateBarLayout->addStretch();
    stateBarLayout->addWidget(endianLabel_);
    stateBarLayout->addWidget(compressedLabel_);

    // 主布局。
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(2);
    mainLayout->addWidget(toolbarWidget);
    mainLayout->addWidget(stackedWidget_);
    mainLayout->addWidget(stateBarWidget);

    updateButtonState();
    updateText();
    updateColor();
}

NbtEditorTabPage* NbtEditorTabPage::createFromFile(const QString& filePath, QWidget* parent)
{
    // 读取文件。
    std::filesystem::path path;
#ifdef Q_OS_WIN
    path = std::filesystem::path(filePath.toStdWString());
#else
    path = std::filesystem::path(filePath.toStdString());
#endif // Q_OS_WIN
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open())
    {
        QMessageBox(
            QMessageBox::Critical,
            EASYTR("Error"),
            QStringLiteral("%1: %2").arg(EASYTR("Can't open the file")).arg(filePath),
            QMessageBox::Close,
            parent
        ).exec();
        return nullptr;
    }

    std::string rootName;
    nbt::Tag    rootTag;
    NbtFileInfo fileInfo;

    fileInfo.filePath = filePath;

    std::istringstream iss;
    // 判断是否经过压缩。
    {
        std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        if (nbt::gzip::isCompressed(content))
        {
            fileInfo.compressed = true;
            content = nbt::gzip::decompress(content);
        }
        iss = std::istringstream(content);
        ifs.close();
    }

    // 猜测字节序。
    iss.clear();
    iss.seekg(0, std::ios::beg);
    const int result = nbt::Tag::guessByteOrder(iss);
    if (result != -1)
    {
        fileInfo.valid     = true;
        fileInfo.bigEndian = result == 1;
        std::tie(rootName, rootTag) = nbt::Tag::parse(iss, fileInfo.bigEndian, 0);
        return new NbtEditorTabPage(QString::fromStdString(rootName), rootTag, fileInfo, parent);
    }
    else
    {
        // 尝试使用小端序，读取 8 字节文件头。
        iss.clear();
        iss.seekg(0, std::ios::beg);
        try
        {
            std::tie(rootName, rootTag) = nbt::Tag::parse(iss, false, 8);

            // 读取 4 字节文件头。
            iss.clear();
            iss.seekg(0, std::ios::beg);
            char buf[4] = {0};
            iss.read(buf, sizeof(buf));

            // 保存 4 字节文件头。
            memcpy(&fileInfo.headerNumber, buf, sizeof(fileInfo.headerNumber));

            fileInfo.valid     = true;
            fileInfo.bigEndian = false;
            fileInfo.hasHeader = true;
            return new NbtEditorTabPage(QString::fromStdString(rootName), rootTag, fileInfo, parent);
        }
        catch (...)
        {
            QMessageBox(
                QMessageBox::Critical,
                EASYTR("Error"),
                QStringLiteral("%1: %2").arg(EASYTR("Can't parse the NBT file")).arg(filePath),
                QMessageBox::Close,
                parent
            ).exec();
            return nullptr;
        }
    }
}

void NbtEditorTabPage::saveNbt()
{
    if (!hasValidRootTag())
        return;

    if (!fileInfo_.valid)
    {
        auto fileInfo = popupDialogForFileInfo(fileInfo_);
        if (!fileInfo.valid)
            return;
        setFileInfo(std::move(fileInfo));
    }

    saveNbt(fileInfo_);
}

void NbtEditorTabPage::saveNbtAs()
{
    if (!hasValidRootTag())
        return;

    auto fileInfo = popupDialogForFileInfo(fileInfo_);
    if (!fileInfo.valid)
        return;
    saveNbt(fileInfo);
}

void NbtEditorTabPage::updateText()
{
    renameButton_->setText(EASYTR("Rename"));
    editButton_->setText(EASYTR("Edit"));
    removeButton_->setText(EASYTR("Remove"));
    snbtButton_->setText(EASYTR("SNBT"));
    addButtons_[nbt::TT_BYTE]->setToolTip(EASYTR("Add Byte Tag"));
    addButtons_[nbt::TT_SHORT]->setToolTip(EASYTR("Add Short Tag"));
    addButtons_[nbt::TT_INT]->setToolTip(EASYTR("Add Int Tag"));
    addButtons_[nbt::TT_LONG]->setToolTip(EASYTR("Add Long Tag"));
    addButtons_[nbt::TT_FLOAT]->setToolTip(EASYTR("Add Float Tag"));
    addButtons_[nbt::TT_DOUBLE]->setToolTip(EASYTR("Add Double Tag"));
    addButtons_[nbt::TT_STRING]->setToolTip(EASYTR("Add String Tag"));
    addButtons_[nbt::TT_BYTE_ARRAY]->setToolTip(EASYTR("Add Byte Array Tag"));
    addButtons_[nbt::TT_INT_ARRAY]->setToolTip(EASYTR("Add Int Array Tag"));
    addButtons_[nbt::TT_LONG_ARRAY]->setToolTip(EASYTR("Add Long Array Tag"));
    addButtons_[nbt::TT_LIST]->setToolTip(EASYTR("Add List Tag"));
    addButtons_[nbt::TT_COMPOUND]->setToolTip(EASYTR("Add Compound Tag"));
    updateStateBarText();
}

void NbtEditorTabPage::updateColor()
{
    for (auto& button : {renameButton_, editButton_, removeButton_, snbtButton_})
        button->setStyleSheet(toolButtonStyleSheet());
}

NbtEditorTabPage::NbtEditorTabPage(QString rootName, nbt::Tag rootTag, NbtFileInfo fileInfo, QWidget* parent)
    : NbtEditorTabPage(parent)
{
    nbtView_->setRootName(std::move(rootName));
    nbtView_->setRootTag(std::move(rootTag));
    setFileInfo(std::move(fileInfo));
}

NbtEditorTabPage::NbtFileInfo NbtEditorTabPage::popupDialogForFileInfo(const NbtFileInfo& defaultValue)
{
    NbtFileInfoDialog dialog;
    dialog.setBigEndian(defaultValue.bigEndian);
    dialog.setCompressed(defaultValue.compressed);
    dialog.setFilePath(defaultValue.filePath);

    if (dialog.exec() != QDialog::Accepted || dialog.filePath().isEmpty())
        return NbtFileInfo();

    NbtFileInfo fileInfo;
    fileInfo.valid      = true;
    fileInfo.bigEndian  = dialog.isBigEndian();
    fileInfo.compressed = dialog.isCompressed();
    fileInfo.filePath   = dialog.filePath();
    return fileInfo;
}

void NbtEditorTabPage::togglePage()
{
    if (stackedWidget_->currentIndex() == 0)
    {
        stackedWidget_->setCurrentIndex(1);
        if (needUpdateSnbt_)
        {
            QString snbt;
            if (nbtView_->rootTag().isCompound())
                snbt = QString::fromStdString(nbtView_->rootTag().toSnbt(2));
            snbtWidget_->setPlainText(snbt);
            needUpdateSnbt_ = false;
        }
    }
    else
    {
        stackedWidget_->setCurrentIndex(0);
    }

    updateButtonState();
}

void NbtEditorTabPage::updateButtonState()
{
    // 先全部禁用。
    for (auto& button : {renameButton_, editButton_, removeButton_, snbtButton_})
        button->setDisabled(true);
    for (auto& [type, button] : addButtons_)
        button->setDisabled(true);

    // 如果具有有效 NBT 则显示 SNBT 按钮。
    if (hasValidRootTag())
        snbtButton_->setEnabled(true);

    // 如果当前位于 SNBT 页面则直接返回。（仅启用 SNBT 按钮）
    if (stackedWidget_->currentIndex() == 1)
        return;

    auto* item = nbtView_->selectedItem();
    // 如果没有选中项直接返回。
    if (!item)
        return;

    if (nbtView_->isArrayElement(item))
    {
        // 数组子元素只启用数值编辑与删除按钮。
        editButton_->setEnabled(true);
        removeButton_->setEnabled(true);
    }
    else
    {
        // 先获取 item 对应的 Tag。
        const auto* tag = const_cast<const NbtTreeView*>(nbtView_)->itemTag(item);
        if (!tag)
            return;

        // 根据 Tag 类型决定是否启用重命名与数值编辑按钮。
        const auto type         = tag->type();
        const bool isNamingTag  = nbtView_->isNamingTag(item);
        if (nbt_utils::tagKeyEditable(type, isNamingTag))   renameButton_->setEnabled(true);
        if (nbt_utils::tagValueEditable(type, isNamingTag)) editButton_->setEnabled(true);
        // 如果当前 item 不是根项则启用删除按钮。
        if (item != nbtView_->rootItem())                   removeButton_->setEnabled(true);

        // 根据当前 item 对应的 Tag 启用不同增添按钮。
        for (auto type : nbt_utils::addableTagTypes(tag))
            addButtons_[type]->setEnabled(true);
    }
}

void NbtEditorTabPage::updateStateBarText()
{
    if (fileInfo_.valid)
    {
        filePathLabel_->setText(fileInfo_.filePath);
        endianLabel_->setText(fileInfo_.bigEndian ? EASYTR("Big Endian") : EASYTR("Little Endian"));
        compressedLabel_->setText(fileInfo_.compressed ? EASYTR("GZip Compressed") : EASYTR("Uncompressed"));
    }
    else
    {
        filePathLabel_->setText(QStringLiteral(" "));
        filePathLabel_->setToolTip(QString());
        endianLabel_->setText(QString());
        compressedLabel_->setText(QString());
    }
}

void NbtEditorTabPage::setFileInfo(NbtFileInfo fileInfo)
{
    const bool pathChanged = fileInfo_.filePath != fileInfo.filePath;
    fileInfo_ = std::move(fileInfo);
    updateStateBarText();
    if (pathChanged)
        emit filePathChanged(fileInfo_.filePath);
}

void NbtEditorTabPage::saveNbt(const NbtFileInfo& fileInfo)
{
    const auto& rootName = nbtView_->rootName();
    const auto& rootTag  = nbtView_->rootTag();
    if (!rootTag.isCompound() || !fileInfo.valid)
        return;

    std::filesystem::path path;
#ifdef Q_OS_WIN
    path = std::filesystem::path(fileInfo.filePath.toStdWString());
#else
    path = std::filesystem::path(fileInfo.filePath.toStdString());
#endif // Q_OS_WIN
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs.is_open())
    {
        QMessageBox(
            QMessageBox::Critical,
            EASYTR("Error"),
            QStringLiteral("%1: %2").arg(EASYTR("Can't open the file")).arg(fileInfo.filePath),
            QMessageBox::Close,
            this
        ).exec();
        return;
    }

    if (fileInfo.hasHeader)
    {
        // 写入文件头第一部分。
        char buf[4] = {0};
        memcpy(buf, &fileInfo.headerNumber, sizeof(buf));
        ofs.write(buf, sizeof(buf));

        // 获取文件数据大小。
        std::int32_t size = 0;
        {
            std::ostringstream oss;
            rootTag.dump(oss, fileInfo.bigEndian, rootName.toStdString());
            size = static_cast<std::int32_t>(oss.str().size());
        }

        // 写入文件头第二部分。
        memcpy(buf, &size, sizeof(buf));
        ofs.write(buf, sizeof(buf));
    }

    // 写入 NBT。
    if (fileInfo.compressed) rootTag.dumpCompressed(ofs, fileInfo.bigEndian, rootName.toStdString());
    else                     rootTag.dump(ofs, fileInfo.bigEndian, rootName.toStdString());
}

#include "nbt_editor_tab_page.moc"
