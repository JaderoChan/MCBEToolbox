#pragma once

#include <map>

#include <qstring.h>

#include <mcnbt/mcnbt.hpp>

#include <components/mywidget.h>
#include "nbt_tree_view.h"

class QLabel;
class QToolButton;
class QStackedWidget;
class QStandardItem;
class CodeTextEdit;

class NbtEditorTabPage : public MyWidget
{
    Q_OBJECT

public:
    explicit NbtEditorTabPage(QWidget* parent = nullptr);

    // 从指定文件中解析 NBT 并构造 Widget；如果文件打开/解析失败返回 nullptr。
    static NbtEditorTabPage* createFromFile(const QString& filePath, QWidget* parent = nullptr);

    NbtTreeView* nbtView() const { return nbtView_; }
    bool hasValidRootTag() const { return nbtView_->rootTag().isCompound(); }

    // 根据 NbtFileInfo 保存 NBT，如果 NbtFileInfo 无效则先弹出配置对话框设置文件信息。
    void saveNbt();
    // 弹出配置对话框以指定配置保存 NBT。
    void saveNbtAs();

signals:
    // 关联文件的路径发生变化时发出。
    void filePathChanged(const QString& filePath);

protected:
    void updateText(
        LanguageManager::Language loaded = LanguageManager::instance().loadedLanguage()) override;
    void updateColor(
        ColorSchemeManager::ColorScheme loaded = ColorSchemeManager::instance().loadedColorScheme()) override;

private:
    // 有关 NBT 文件的更多信息可以参见 https://wiki.bedrock.dev/nbt/nbt-in-depth。
    struct NbtFileInfo
    {
        NbtFileInfo() noexcept {}

        bool valid                = false; // 如果当前页面的 NBT 没有关联至文件则置 false（例如由用户创建的空 NBT）。
        bool bigEndian            = false;
        bool compressed           = false;
        bool hasHeader            = false; // 是否包含 8 字节文件头。
        std::int32_t headerNumber = 0;     // 文件头的第一个数字值。
        QString filePath;
    };

    explicit NbtEditorTabPage(QString rootName, nbt::Tag rootTag, NbtFileInfo fileInfo, QWidget* parent = nullptr);

    static NbtFileInfo popupDialogForFileInfo(const NbtFileInfo& defaultValue = NbtFileInfo());

    void togglePage();
    void updateButtonState();
    void updateStateBarText();

    void setFileInfo(NbtFileInfo fileInfo);
    void saveNbt(const NbtFileInfo& fileInfo);

    NbtFileInfo fileInfo_;

    QToolButton* renameButton_ = nullptr;
    QToolButton* editButton_   = nullptr;
    QToolButton* removeButton_ = nullptr;
    QToolButton* snbtButton_   = nullptr;

    std::map<nbt::TagType, QToolButton*> addButtons_;

    QLabel* filePathLabel_   = nullptr;
    QLabel* endianLabel_     = nullptr;
    QLabel* compressedLabel_ = nullptr;

    QStackedWidget* stackedWidget_  = nullptr;
    NbtTreeView*    nbtView_        = nullptr;
    CodeTextEdit*   snbtWidget_     = nullptr;
    bool            needUpdateSnbt_ = true;
};
