#pragma once

#include <qevent.h>
#include <qtreeview.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qset.h>

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

    const nbt::Tag& tag() const { return tag_; }
    void setTag(const nbt::Tag& tag, const QString& rootName = QString());

    QString rootName() const { return rootName_; }
    void setRootName(const QString& name);

    bool removeItem(QStandardItem* item);
    bool renameKey(QStandardItem* item, const QString& newKey);

signals:
    void tagChanged();

protected:
    void changeEvent(QEvent* e) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool edit(const QModelIndex& index, EditTrigger trigger, QEvent* event) override;

private slots:
    void showContextMenu(const QPoint& position);
    void handleFocusChanged(QWidget* oldWidget, QWidget* nowWidget);

private:
    friend class NbtTreeItemDelegate;

    enum EditTarget
    {
        EditValue, EditKey
    };

    enum DataRole
    {
        PathRole = Qt::UserRole + 1,
        ArrayIndexRole,
        ValueStartRole,
        SummaryStyleRole
    };

    struct EditCapability
    {
        bool keyEditable   = false;
        bool valueEditable = false;
    };

    enum class PendingKind
    {
        None,
        CompoundEntry,
        Element
    };

    QStandardItem* rootItem() const;

    void rebuild();
    QStandardItem* buildItem(QStandardItem* parent, const QString& key, nbt::Tag& value, const QStringList& path);
    void updateItemText(QStandardItem* item, const QString& key, const nbt::Tag& value) const;
    void updateArrayElementText(QStandardItem* item, const nbt::Tag& array, nbt::Tag::size_type index) const;
    void retranslateItem(QStandardItem* item);

    nbt::Tag* resolveTag(const QStringList& path);
    QStandardItem* findItemByPath(QStandardItem* item, const QStringList& path) const;
    QStandardItem* findItemFor(const QStringList& path, int arrayIndex) const;
    void collectExpandedPaths(QStandardItem* item, QSet<QString>& out) const;
    void restoreExpandedPaths(QStandardItem* item, const QSet<QString>& paths);
    QString pathKey(const QStringList& path) const;

    EditTarget editTargetForClick(QStandardItem* item, const QModelIndex& index, EditTrigger trigger, QEvent* event);
    EditCapability editCapabilityFor(QStandardItem* item);
    PendingKind pendingKindFor(const QStringList& path, int arrayIndex) const;

    QIcon iconForType(nbt::TagType type) const;
    QString summaryText(const nbt::Tag& value) const;
    QString valueText(const nbt::Tag& value) const;
    QString arrayElementText(const nbt::Tag& array, nbt::Tag::size_type index) const;

    void commitValueEdit(QStandardItem* item, const QString& text);
    void commitKeyEdit(QStandardItem* item, const QString& text);
    bool setScalarValue(nbt::Tag& target, const QString& text);
    bool setArrayElementValue(nbt::Tag& array, nbt::Tag::size_type index, const QString& text);
    bool renameCompoundKey(nbt::Tag& container, const QString& oldKey, const QString& newKey);
    qlonglong parseIntegerLiteral(const QString& text, bool* ok) const;

    void appendAddTypeActions(QMenu* menu, QStandardItem* containerItem);

    void beginRowEdit(QStandardItem* item, EditTarget focusTarget);
    void closeRowEdit(bool commit);

    void beginAddCompoundEntry(QStandardItem* containerItem, nbt::TagType type);
    void finalizePendingNewEntry(const QString& key, const QString& value);

    void beginAddListElement(QStandardItem* containerItem, nbt::Tag value);
    void beginAddArrayElement(QStandardItem* containerItem);
    void finalizePendingNewElement(const QString& value);
    void cancelPendingEntry();

    nbt::Tag    tag_;
    QString     rootName_;
    bool        rebuilding_ = false;
    QStringList pendingNewEntryPath_;
    QStringList pendingNewElementContainer_;
    int         pendingNewElementIndex_ = -1;

    NbtRowEditor*  activeEditor_   = nullptr;
    QStandardItem* activeEditItem_ = nullptr;

    QStandardItemModel*  model_    = nullptr;
    NbtTreeItemDelegate* delegate_ = nullptr;
};
