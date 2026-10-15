#pragma once

#include <qicon.h>
#include <qstring.h>
#include <qtreeview.h>

#include <mcnbt/mcnbt.hpp>

class QMenu;
class QStandardItem;
class QStandardItemModel;
class NbtRowEditor;
class NbtTreeItemDelegate;

class NbtTreeView : public QTreeView
{
    Q_OBJECT

public:
    explicit NbtTreeView(QWidget* parent = nullptr);

    void setRootName(QString rootName);
    QString rootName() const;

    // 设置根 Tag 值，如果给定 Tag 不是 nbt::TT_COMPOUND 则不做任何事。
    void setRootTag(nbt::Tag rootTag);
    // 获取根 Tag 值。
    const nbt::Tag& rootTag() const;

    // 判断给定项的 Tag 是否是 Array 元素。
    bool isArrayElement(const QStandardItem* item) const;
    // 判断给定项的 Tag 是否是具名 Tag。
    bool isNamingTag(const QStandardItem* item)    const;

    // 获取给定项的 Tag 名，如果给定项不是具名 Tag 返回无效值。
    QVariant        itemTagKey(const QStandardItem* item)    const;
    // 获取给定项在 Array 或 List（不具名 Tag）中的索引，如果给定项是具名 Tag 返回无效值。
    QVariant        itemTagIndex(const QStandardItem* item)  const;
    // 获取给定项的父 Tag 值，如果给定项无父项返回 nullptr。
    const nbt::Tag* itemParentTag(const QStandardItem* item) const;
    // 获取给定项的 Tag 值，如果给定项为 Array 元素则返回 nullptr。
    const nbt::Tag* itemTag(const QStandardItem* item)       const;

    // 添加子项至给定项。
    // 如果给定项不支持添加元素、给定子项不符合父项类型、给定名称已存在均返回 false。
    // 对于给定项是 Array 类型，则子项仅保留标量值。
    // 对于给定项是 Array 或 List 类型，则参数 key 是无效的。
    bool addItem(QStandardItem* parentItem, nbt::Tag value, const QString& key = QString());
    // 删除给定项。
    bool removeItem(QStandardItem* item);

    // 更改给定项名，如果给定项非具名 Tag，返回 false。
    bool changeItemKey(QStandardItem* item, const QString& newKey);
    // 更改给定项值，如果给定值不符合父项类型返回 false。
    bool changeItemValue(QStandardItem* item, nbt::Tag newValue);

    // 在给定项上唤起 GUI 式元素添加，如果给定类型不符合给定项的元素类型返回 false。
    bool invokeAddItem(QStandardItem* parentItem, nbt::TagType type);
    // 在给定项上唤起 GUI 式元素编辑，如果给定项不可编辑返回 false。
    bool invokeEditItem(QStandardItem* item);
    // 在给定项上唤起 GUI 式元素重命名，如果给定项不可重命名返回 false。
    bool invokeRenameItem(QStandardItem* item);

signals:
    void rootNameChanged();
    void tagChanged();

protected:
    void changeEvent(QEvent* e) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool edit(const QModelIndex& index, EditTrigger trigger, QEvent* event) override;

private:
    friend class NbtTreeItemDelegate;

    enum DataRole
    {
        IsNamingTagRole = Qt::UserRole + 1,
        TagKeyRole,
        TagIndexRole,
        IsCompoundSummaryRole,
        SummaryValueRole
    };

    enum class PendingRowEdit
    {
        None,
        AddArrayElement,
        AddListElement,
        AddCompoundEntry,
        EditRow
    };

    static QIcon        iconForTagType(nbt::TagType type);
    static QString      valueTextForTag(const nbt::Tag& tag);
    static QString      valueTextForArrayElement(const nbt::Tag& arrayTag, qulonglong index);
    static nbt::TagType arrayElementTagType(nbt::TagType arrayType);

    nbt::Tag* itemParentTag(const QStandardItem* item);
    nbt::Tag* itemTag(const QStandardItem* item);

    void updateSummaryValue(QStandardItem* item, const nbt::Tag& tag);

    static QString  defaultValueText(nbt::TagType type);
    static nbt::Tag parseScalarTag(nbt::TagType type, const QString& text, bool* ok);

    void rebuild();
    QStandardItem* createArrayItem(nbt::Tag* parentArrayTag, qulonglong index);
    QStandardItem* createItem(bool isNamingTag, const QString& key, qulonglong index, const nbt::Tag& value);
    QStandardItem* buildItem(
        QStandardItem* parentItem, bool isNamingTag, const QString& key, qulonglong index, nbt::Tag& value);

    void finishActiveRowEdit(bool commit, bool restoreFocus = true);
    void beginRowEdit(
        QStandardItem* parentItem, QStandardItem* editItem, nbt::TagType type, bool isNamingTag,
        const QString& initialKeyText, const QString& initialValueText, bool focusKey, PendingRowEdit pending);
    void rowEditFinished(NbtRowEditor* editor, bool confirm, bool restoreFocus = true);
    void handleFocusChanged(QWidget* oldWidget, QWidget* nowWidget);

    void showContextMenu(const QPoint& position);

    QString  rootName_;
    nbt::Tag rootTag_;

    QStandardItemModel*  model_    = nullptr;
    NbtTreeItemDelegate* delegate_ = nullptr;

    QStandardItem* activeEditItem_    = nullptr;
    QStandardItem* pendingParentItem_ = nullptr;
    nbt::TagType   pendingTagType_    = nbt::TT_END;
    PendingRowEdit pendingRowEdit_    = PendingRowEdit::None;
};
