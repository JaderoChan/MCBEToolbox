#include "nbt_tree_view.h"

#include <utility>

#include <qevent.h>
#include <qmenu.h>
#include <qmimedata.h>
#include <qstandarditemmodel.h>
#include <qurl.h>

#include <easy_translate.hpp>

#include "nbt_row_editor.h"
#include "nbt_tree_item_delegate.h"
#include "nbt_utils.h"

NbtTreeView::NbtTreeView(QWidget* parent)
    : QTreeView(parent)
{
    model_ = new QStandardItemModel(this);
    model_->setColumnCount(1);
    setModel(model_);

    setHeaderHidden(true);
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DropOnly);
    delegate_ = new NbtTreeItemDelegate(this);
    setItemDelegate(delegate_);

    setEditTriggers(QAbstractItemView::DoubleClicked);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QTreeView::customContextMenuRequested, this, &NbtTreeView::showContextMenu);

    connect(
        selectionModel(), &QItemSelectionModel::selectionChanged, this,
        [this]() { emit selectedItemChanged(selectedItem()); }
    );
    connect(
        this, &NbtTreeView::expanded, this, [this](const QModelIndex& index)
        {
            auto* item = model_->itemFromIndex(index);
            loadAllElement(item);
        }
    );
}

void NbtTreeView::setRootName(QString rootName)
{
    if (rootName_ == rootName)
        return;

    auto oldName = std::exchange(rootName_, std::move(rootName));
    if (model_->rowCount() > 0)
    {
        auto* rootItem = model_->item(0);
        const auto text = rootName_ + QStringLiteral(": ");
        rootItem->setText(text);
        rootItem->setToolTip(text);
        rootItem->setData(rootName_, TagKeyRole);

        EditStep step(EditActionType::Rename);
        step.key    = std::move(oldName);
        step.newKey = rootName_;
        recordStep(std::move(step));
    }
    emit rootNameChanged();
}

QString NbtTreeView::rootName() const
{
    return rootName_;
}

void NbtTreeView::setRootTag(nbt::Tag rootTag)
{
    if (rootTag.isCompound())
    {
        rootTag_ = std::move(rootTag);
        rebuild();
        clearHistory();
        emit tagChanged();
    }
}

const nbt::Tag& NbtTreeView::rootTag() const
{
    return rootTag_;
}

bool NbtTreeView::isArrayElement(const QStandardItem* item) const
{
    const auto* parentTag = itemParentTag(item);
    if (!parentTag) return false;
    return parentTag->isArray();
}

bool NbtTreeView::isNamingTag(const QStandardItem* item) const
{
    return item->data(IsNamingTagRole).toBool();
}

QVariant NbtTreeView::itemTagKey(const QStandardItem* item) const
{
    return item->data(TagKeyRole);
}

QVariant NbtTreeView::itemTagIndex(const QStandardItem* item) const
{
    return item->data(TagIndexRole);
}

const nbt::Tag* NbtTreeView::itemParentTag(const QStandardItem* item) const
{
    return const_cast<NbtTreeView*>(this)->itemParentTag(item);
}

const nbt::Tag* NbtTreeView::itemTag(const QStandardItem* item) const
{
    return const_cast<NbtTreeView*>(this)->itemTag(item);
}

bool NbtTreeView::addItem(QStandardItem* parentItem, nbt::Tag value, const QString& key)
{
    if (!parentItem)
        return false;

    loadAllElement(parentItem);

    const auto* parentTag = itemTag(parentItem);
    if (!parentTag)
        return false;

    const qulonglong index = parentTag->isCompound() ? 0 : static_cast<qulonglong>(parentTag->size());
    return insertItem(parentItem, std::move(value), key, index);
}

bool NbtTreeView::removeItem(QStandardItem* item)
{
    if (!item)
        return false;

    auto* parentTag = itemParentTag(item);
    if (!parentTag)
    {
        // 不允许直接删除根项。
        return false;
    }

    auto* parentItem = item->parent();
    loadAllElement(parentItem);

    EditStep step(EditActionType::Remove);
    if (!applyingHistory_)
    {
        step.path  = itemPath(item);
        step.key   = isNamingTag(item) ? itemTagKey(item).toString() : QString();
        step.value = itemValue(item);
    }

    if (isNamingTag(item))
    {
        parentTag->erase(itemTagKey(item).toString().toStdString());
    }
    else
    {
        parentTag->erase(static_cast<std::size_t>(itemTagIndex(item).toULongLong()));

        // 更新删除项之后项的索引值与文本。
        updateElementItemData(parentItem, item->row() + 1, static_cast<qulonglong>(parentItem->rowCount()), -1);

        // 如果删除元素后，列表为空，则将其置为初始状态以使其能够接受不同的 Tag 类型进行重新初始化。
        if (parentTag->isList() && parentTag->empty())
            *parentTag = nbt::Tag::list();
    }

    model_->removeRow(item->row(), item->index().parent());
    updateSummaryValue(parentItem, *parentTag);
    recordStep(std::move(step));
    emit tagChanged();
    return true;
}

bool NbtTreeView::changeItemKey(QStandardItem* item, const QString& newKey)
{
    if (!isNamingTag(item))
        return false;

    auto* parentTag = itemParentTag(item);
    if (!parentTag)
    {
        setRootName(newKey);
        return true;
    }

    // 如果新名称已存在则返回 false。
    const auto stdNewKey = newKey.toStdString();
    if (parentTag->contains(stdNewKey))
        return false;

    EditStep step(EditActionType::Rename);
    step.key  = itemTagKey(item).toString();
    step.path = itemPath(item);

    // 更改 Tag 中的名称。
    auto       tag       = std::move(*itemTag(item));
    const auto valueText = nbt_utils::makeTextFromTag(tag);
    parentTag->erase(itemTagKey(item).toString().toStdString());
    parentTag->insert(stdNewKey, std::move(tag));

    // 更改 Item 名称，并将其按照 Tag 顺序重新排列。
    const auto text = QStringLiteral("%1: %2").arg(newKey).arg(valueText);
    item->setText(text);
    item->setToolTip(text);
    item->setData(newKey, TagKeyRole);

    qulonglong i = 0;
    for (const auto& [k, v] : parentTag->items())
    {
        if (k == stdNewKey) break;
        ++i;
    }
    item->parent()->insertRow(i, item->parent()->takeRow(item->row())[0]);

    step.newPath = itemPath(item);
    step.newKey  = newKey;
    recordStep(std::move(step));
    emit tagChanged();
    return true;
}

bool NbtTreeView::changeItemValue(QStandardItem* item, nbt::Tag newValue)
{
    auto* parentTag = itemParentTag(item);
    if (!parentTag)
    {
        // 不允许直接更改根项值。
        return false;
    }

    EditStep step(EditActionType::Change);
    step.path  = itemPath(item);
    step.value = itemValue(item);
    if (nbt_utils::isEqualScalarTag(step.value, newValue))
        return true;
    step.newValue = newValue;

    if (parentTag->isArray())
    {
        QString text;
        switch (parentTag->type())
        {
            case nbt::TT_BYTE_ARRAY:
            {
                if (!newValue.isByte()) return false;
                const auto i = itemTagIndex(item).toULongLong();
                parentTag->getByteArray()[static_cast<std::size_t>(i)] = newValue.getByte();
                text = QStringLiteral("[%1]: %2").arg(i).arg(newValue.getByte());
                break;
            }
            case nbt::TT_INT_ARRAY:
            {
                if (!newValue.isInt()) return false;
                const auto i = itemTagIndex(item).toULongLong();
                parentTag->getIntArray()[static_cast<std::size_t>(i)] = newValue.getInt();
                text = QStringLiteral("[%1]: %2").arg(i).arg(newValue.getInt());
                break;
            }
            case nbt::TT_LONG_ARRAY:
            {
                if (!newValue.isLong()) return false;
                const auto i = itemTagIndex(item).toULongLong();
                parentTag->getLongArray()[static_cast<std::size_t>(i)] = newValue.getLong();
                text = QStringLiteral("[%1]: %2").arg(i).arg(newValue.getLong());
                break;
            }
            default:
                return false;
        }

        item->setText(text);
        item->setToolTip(text);
    }
    else if (parentTag->isList())
    {
        if (parentTag->listItemType() != newValue.type())
            return false;

        auto& target = parentTag->at(static_cast<std::size_t>(itemTagIndex(item).toULongLong()));
        target = std::move(newValue);

        auto*     parentItem = item->parent();
        const int row        = item->row();
        parentItem->removeRow(row);
        parentItem->insertRow(row, createItem(false, QString(), row, target));
    }
    else if (parentTag->isCompound())
    {
        const auto key    = itemTagKey(item).toString();
        auto&      target = parentTag->at(key.toStdString());
        target = std::move(newValue);

        auto*     parentItem = item->parent();
        const int row        = item->row();
        parentItem->removeRow(row);
        parentItem->insertRow(row, createItem(true, key, 0, target));
    }
    else
    {
        return false;
    }

    recordStep(std::move(step));
    emit tagChanged();
    return true;
}

bool NbtTreeView::invokeAddItem(QStandardItem* parentItem, nbt::TagType type)
{
    auto* parentTag = itemTag(parentItem);
    if (!parentTag)
        return false;

    if (parentTag->isArray() || parentTag->isContainer())
    {
        loadAllElement(parentItem);

        bool isNamingTag = true;
        if (parentTag->isArray())
        {
            if (type != nbt_utils::getArrayElementTagType(parentTag->type()))
                return false;
            isNamingTag = false;
        }
        else if (parentTag->isList())
        {
            if (parentTag->listItemType() != nbt::TT_END && type != parentTag->listItemType())
                return false;

            if (!nbt_utils::isTagEditable(type, false))
                return addItem(parentItem, nbt::Tag(type));

            isNamingTag = false;
        }

        auto* item = new QStandardItem();
        item->setIcon(nbt_utils::getTagTypeIcon(type));
        parentItem->appendRow(item);
        beginRowEdit(item, type, isNamingTag, QString(), defaultValueText(type), isNamingTag, RowEditType::Append);

        return true;
    }
    else
    {
        return false;
    }
}

bool NbtTreeView::invokeEditItemKey(QStandardItem* item)
{
    if (!isNamingTag(item))
        return false;

    auto* tag = itemTag(item);
    if (!tag)
        return false;

    const auto keyText   = itemTagKey(item).toString();
    const auto valueText = nbt_utils::makeTextFromTag(*tag);

    beginRowEdit(item, tag->type(), true, keyText, valueText, true, RowEditType::Modify);
    return true;
}

bool NbtTreeView::invokeEditItemValue(QStandardItem* item)
{
    nbt::TagType type;
    QString      valueText;

    if (isArrayElement(item))
    {
        auto* parentTag = itemParentTag(item);
        if (!parentTag)
            return false;
        type      = nbt_utils::getArrayElementTagType(parentTag->type());
        valueText = nbt_utils::makeArrayElementText(*parentTag, itemTagIndex(item).toULongLong());
    }
    else
    {
        auto* tag = itemTag(item);
        if (!tag || !nbt_utils::isTagValueEditable(tag->type(), isNamingTag(item)))
            return false;
        type      = tag->type();
        valueText = nbt_utils::makeTextFromTag(*tag);
    }

    const auto keyText = isNamingTag(item) ? itemTagKey(item).toString() : QString();
    beginRowEdit(item, type, isNamingTag(item), keyText, valueText, false, RowEditType::Modify);
    return true;
}

bool NbtTreeView::canUndo() const
{
    return !undoStack_.empty();
}

bool NbtTreeView::canRedo() const
{
    return !redoStack_.empty();
}

bool NbtTreeView::undo()
{
    if (undoStack_.empty())
        return false;

    if (editor_)
        endRowEdit(false);

    auto action = std::move(undoStack_.back());
    undoStack_.pop_back();

    QList<int> focusPath;
    if (!applyAction(action, true, focusPath))
    {
        clearHistory(false);
        return false;
    }

    redoStack_.push_back(std::move(action));
    emitHistoryChanged();

    if (auto* item = itemFromPath(focusPath))
    {
        for (auto* p = item->parent(); p; p = p->parent())
            expand(p->index());
        setCurrentIndex(item->index());
        scrollTo(item->index());
    }
    return true;
}

bool NbtTreeView::redo()
{
    if (redoStack_.empty())
        return false;

    if (editor_)
        endRowEdit(false);

    auto action = std::move(redoStack_.back());
    redoStack_.pop_back();

    QList<int> focusPath;
    if (!applyAction(action, false, focusPath))
    {
        clearHistory(false);
        return false;
    }

    undoStack_.push_back(std::move(action));
    emitHistoryChanged();

    if (auto* item = itemFromPath(focusPath))
    {
        for (auto* p = item->parent(); p; p = p->parent())
            expand(p->index());
        setCurrentIndex(item->index());
        scrollTo(item->index());
    }
    return true;
}

QStandardItem* NbtTreeView::rootItem() const
{
    return model_->item(0);
}

bool NbtTreeView::isModified() const
{
    return savePointId_ != currentActionId();
}

void NbtTreeView::markSaved()
{
    savePointId_ = currentActionId();
    emitHistoryChanged();
}

QStandardItem* NbtTreeView::selectedItem() const
{
    const auto indexes = selectionModel()->selectedIndexes();
    if (indexes.isEmpty())
        return nullptr;
    return model_->itemFromIndex(indexes.first());
}

void NbtTreeView::changeEvent(QEvent* e)
{
    QTreeView::changeEvent(e);
    if (e->type() == QEvent::LanguageChange)
        update();
}

void NbtTreeView::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_PageUp)
    {
        scrollToTop();
        return;
    }
    if (e->key() == Qt::Key_PageDown)
    {
        scrollToBottom();
        return;
    }

    auto* item = selectedItem();
    if (item)
    {
        switch (e->key())
        {
            case Qt::Key_F2:
                if (invokeEditItemKey(item))
                    return;
                break;
            case Qt::Key_Return:
            case Qt::Key_Enter:
                if (invokeEditItemValue(item))
                    return;
                break;
            case Qt::Key_Delete:
            case Qt::Key_Backspace:
                if (removeItem(item))
                    return;
                break;
            default:
                break;
        }
    }

    QTreeView::keyPressEvent(e);
}

bool NbtTreeView::edit(const QModelIndex& index, EditTrigger trigger, QEvent* e)
{
    if (trigger == QAbstractItemView::DoubleClicked)
    {
        auto* item = model_->itemFromIndex(index);
        if (item && invokeEditItemValue(item))
            return true;
    }
    return QTreeView::edit(index, trigger, e);
}

void NbtTreeView::dragEnterEvent(QDragEnterEvent* e)
{
    if (e->mimeData()->hasUrls())
    {
        e->acceptProposedAction();
        return;
    }
    QTreeView::dragEnterEvent(e);
}

void NbtTreeView::dragMoveEvent(QDragMoveEvent* e)
{
    if (e->mimeData()->hasUrls())
    {
        e->acceptProposedAction();
        return;
    }
    QTreeView::dragMoveEvent(e);
}

void NbtTreeView::dropEvent(QDropEvent* e)
{
    if (!e->mimeData()->hasUrls())
    {
        QTreeView::dropEvent(e);
        return;
    }

    QStringList filePaths;
    for (const auto& url : e->mimeData()->urls())
    {
        if (url.isLocalFile())
            filePaths.append(url.toLocalFile());
    }

    if (filePaths.empty())
    {
        e->ignore();
        return;
    }

    e->acceptProposedAction();
    emit filesDropped(filePaths);
}

QString NbtTreeView::defaultValueText(nbt::TagType type)
{
    if      (nbt::details::isInteger(type))    return QStringLiteral("0");
    else if (nbt::details::isFloatPoint(type)) return QStringLiteral("0.0");
    else if (nbt::details::isString(type))     return QString();
    else                                       return QString();
}

nbt::Tag* NbtTreeView::itemParentTag(const QStandardItem* item)
{
    auto* parentItem = item->parent();
    if (!parentItem)
        return nullptr;
    return itemTag(parentItem);
}

nbt::Tag* NbtTreeView::itemTag(const QStandardItem* item)
{
    if (isArrayElement(item))
        return nullptr;

    auto* parentTag = itemParentTag(item);
    if (!parentTag)
        return &rootTag_;

    if (isNamingTag(item))
    {
        const auto key = itemTagKey(item).toString();
        return &parentTag->at(key.toStdString());
    }
    else
    {
        const auto index = itemTagIndex(item).toULongLong();
        return &parentTag->at(static_cast<std::size_t>(index));
    }
}

void NbtTreeView::rebuild()
{
    model_->clear();
    model_->setColumnCount(1);

    if (rootTag_.isCompound())
    {
        auto* rootItem = buildItem(nullptr, true, rootName_, 0, rootTag_);
        model_->appendRow(rootItem);
        setExpanded(rootItem->index(), true);
    }
}

QStandardItem* NbtTreeView::createArrayItem(nbt::Tag* parentArrayTag, qulonglong index)
{
    auto* item = new QStandardItem();
    item->setIcon(nbt_utils::getTagTypeIcon(nbt_utils::getArrayElementTagType(parentArrayTag->type())));
    const auto text = QStringLiteral("[%1]: %2")
        .arg(index)
        .arg(nbt_utils::makeArrayElementText(*parentArrayTag, index));
    item->setText(text);
    item->setToolTip(text);
    item->setData(false, IsNamingTagRole);
    item->setData(index, TagIndexRole);
    return item;
}

QStandardItem* NbtTreeView::createItem(bool isNamingTag, const QString& key, qulonglong index, const nbt::Tag& value)
{
    auto* item = new QStandardItem();
    item->setIcon(nbt_utils::getTagTypeIcon(value.type()));
    const auto text = isNamingTag
        ? QStringLiteral("%1: %2").arg(key).arg(nbt_utils::makeTextFromTag(value))
        : QStringLiteral("[%1]: %2").arg(index).arg(nbt_utils::makeTextFromTag(value));
    item->setText(text);
    item->setToolTip(text);
    item->setData(isNamingTag, IsNamingTagRole);
    isNamingTag ? item->setData(key, TagKeyRole) : item->setData(index, TagIndexRole);

    if (value.isArray() || value.isList())
    {
        item->setData(false, IsCompoundSummaryRole);
        item->setData(static_cast<qulonglong>(value.size()), SummaryValueRole);
    }
    else if (value.isCompound())
    {
        item->setData(true, IsCompoundSummaryRole);
        item->setData(static_cast<qulonglong>(value.size()), SummaryValueRole);
    }

    return item;
}

QStandardItem* NbtTreeView::buildItem(
    QStandardItem* parentItem, bool isNamingTag, const QString& key, qulonglong index, nbt::Tag& value)
{
    auto* item = createItem(isNamingTag, key, index, value);

    if (value.isArray())
    {
        std::size_t count = value.size();
        if (count > LAZY_LOADING_THRESHOLD)
        {
            count = LAZY_LOADING_THRESHOLD;
            item->setData(true, IsPartLoadingRole);
        }

        for (std::size_t i = 0; i < count; ++i)
        {
            auto* child = createArrayItem(&value, static_cast<qulonglong>(i));
            item->appendRow(child);
        }
    }
    else if (value.isList())
    {
        std::size_t count = value.size();
        if (count > LAZY_LOADING_THRESHOLD)
        {
            count = LAZY_LOADING_THRESHOLD;
            item->setData(true, IsPartLoadingRole);
        }

        for (std::size_t i = 0; i < count; ++i)
            (void)buildItem(item, false, QString(), static_cast<qulonglong>(i), value[i]);
    }
    else if (value.isCompound())
    {
        for (auto& [k, v] : value.items())
            (void)buildItem(item, true, QString::fromStdString(k), 0, v);
    }

    if (parentItem)
        parentItem->appendRow(item);

    return item;
}

bool NbtTreeView::isPartLoading(const QStandardItem* item) const
{
    const auto data = item->data(IsPartLoadingRole);
    return data.isValid() && data.toBool();
}

void NbtTreeView::loadAllElement(QStandardItem* parentItem)
{
    if (!parentItem || !isPartLoading(parentItem))
        return;

    auto* tag = itemTag(parentItem);
    if (!tag || (!tag->isArray() && !tag->isList()))
        return;

    if (tag->isArray())
    {
        for (std::size_t i = LAZY_LOADING_THRESHOLD; i < tag->size(); ++i)
        {
            auto* child = createArrayItem(tag, static_cast<qulonglong>(i));
            parentItem->appendRow(child);
        }
    }
    else
    {
        for (std::size_t i = LAZY_LOADING_THRESHOLD; i < tag->size(); ++i)
        {
            (void)buildItem(parentItem, false, QString(), static_cast<qulonglong>(i), tag->at(i));
        }
    }

    parentItem->setData(false, IsPartLoadingRole);
}

bool NbtTreeView::insertItem(QStandardItem* parentItem, nbt::Tag value, const QString& key, qulonglong index)
{
    if (!parentItem)
        return false;

    loadAllElement(parentItem);

    auto* parentTag = itemTag(parentItem);
    if (!parentTag)
        return false;

    EditStep step(EditActionType::Add);
    if (!applyingHistory_)
    {
        step.key   = key;
        step.value = value;
    }

    int row = 0;
    if (parentTag->isArray())
    {
        if (index > parentTag->size())
            return false;

        switch (parentTag->type())
        {
            case nbt::TT_BYTE_ARRAY:
            {
                if (!value.isByte()) return false;
                auto& array = parentTag->getByteArray();
                array.insert(array.begin() + static_cast<std::ptrdiff_t>(index), value.getByte());
                break;
            }
            case nbt::TT_INT_ARRAY:
            {
                if (!value.isInt()) return false;
                auto& array = parentTag->getIntArray();
                array.insert(array.begin() + static_cast<std::ptrdiff_t>(index), value.getInt());
                break;
            }
            case nbt::TT_LONG_ARRAY:
            {
                if (!value.isLong()) return false;
                auto& array = parentTag->getLongArray();
                array.insert(array.begin() + static_cast<std::ptrdiff_t>(index), value.getLong());
                break;
            }
            default:
                return false;
        }

        row = static_cast<int>(index);
        auto* child = createArrayItem(parentTag, index);
        updateElementItemData(parentItem, index, static_cast<qulonglong>(parentItem->rowCount()), 1);
        parentItem->insertRow(row, child);
    }
    else if (parentTag->isList())
    {
        if (index > parentTag->size())
            return false;
        if (parentTag->listItemType() != nbt::TT_END && value.type() != parentTag->listItemType())
            return false;

        parentTag->insert(static_cast<std::size_t>(index), std::move(value));

        row = static_cast<int>(index);
        auto* child = buildItem(nullptr, false, QString(), index, parentTag->at(static_cast<std::size_t>(index)));
        updateElementItemData(parentItem, index, static_cast<qulonglong>(parentItem->rowCount()), 1);
        parentItem->insertRow(row, child);
    }
    else if (parentTag->isCompound())
    {
        const auto stdKey = key.toStdString();
        if (parentTag->contains(stdKey))
            return false;
        parentTag->insert(stdKey, std::move(value));

        // 保证 QStandardItem 顺序与 Tag 顺序一致。
        for (const auto& [k, v] : parentTag->items())
        {
            if (k == stdKey) break;
            ++row;
        }

        auto* child = buildItem(nullptr, true, key, 0, parentTag->at(stdKey));
        parentItem->insertRow(row, child);
    }
    else
    {
        return false;
    }

    updateSummaryValue(parentItem, *parentTag);

    if (!applyingHistory_)
    {
        step.path = itemPath(parentItem);
        step.path.append(row);
        recordStep(std::move(step));
    }

    emit tagChanged();
    return true;
}

nbt::Tag NbtTreeView::itemValue(const QStandardItem* item) const
{
    if (const auto* tag = itemTag(item))
        return *tag;

    const auto* parentTag = itemParentTag(item);
    const auto  i         = static_cast<std::size_t>(itemTagIndex(item).toULongLong());
    switch (parentTag ? parentTag->type() : nbt::TT_END)
    {
        case nbt::TT_BYTE_ARRAY: return nbt::Tag(parentTag->getByteArray()[i]);
        case nbt::TT_INT_ARRAY:  return nbt::Tag(parentTag->getIntArray()[i]);
        case nbt::TT_LONG_ARRAY: return nbt::Tag(parentTag->getLongArray()[i]);
        default:                 return nbt::Tag();
    }
}

QList<int> NbtTreeView::itemPath(const QStandardItem* item) const
{
    QList<int> path;
    for (; item && item->parent(); item = item->parent())
        path.prepend(item->row());
    return path;
}

QStandardItem* NbtTreeView::itemFromPath(const QList<int>& path)
{
    auto* item = rootItem();
    for (int row : path)
    {
        if (!item)
            return nullptr;
        loadAllElement(item);
        item = item->child(row);
    }
    return item;
}

void NbtTreeView::updateSummaryValue(QStandardItem* item, const nbt::Tag& tag)
{
    if (!item)
        return;
    item->setData(static_cast<qulonglong>(tag.size()), SummaryValueRole);
}

void NbtTreeView::updateElementItemData(QStandardItem* parentItem, qulonglong begin, qulonglong end, int offset)
{
    if (!parentItem || begin >= end)
        return;

    const auto* parentTag = itemTag(parentItem);
    if (!parentTag || (!parentTag->isArray() && !parentTag->isList()))
        return;

    loadAllElement(parentItem);

    const bool blocked          = model_->blockSignals(true);
    const auto parentModelIndex = parentItem->index();
    for (qulonglong row = begin; row < end; ++row)
    {
        const auto index      = row + offset;
        const auto modelIndex = model_->index(row, 0, parentModelIndex);

        QString valueText;
        if (parentTag->isArray())
            valueText = nbt_utils::makeArrayElementText(*parentTag, index);
        else if (parentTag->isList())
            valueText = nbt_utils::makeTextFromTag((*parentTag)[static_cast<std::size_t>(index)]);
        const auto text = QStringLiteral("[%1]: %2").arg(index).arg(valueText);

        model_->setData(modelIndex, text,  Qt::DisplayRole);
        model_->setData(modelIndex, text,  Qt::ToolTipRole);
        model_->setData(modelIndex, index, TagIndexRole);
    }
    model_->blockSignals(blocked);
}

void NbtTreeView::beginRowEdit(
    QStandardItem* item, nbt::TagType type, bool isNamingTag,
    const QString& keyText, const QString& valueText, bool focusKey, RowEditType editType)
{
    if (editItem_ || editor_)
        endRowEdit(false);

    editItem_ = item;
    editType_ = editType;

    editor_ = new NbtRowEditor(type, isNamingTag, this);
    editor_->setKeyText(keyText);
    editor_->setValueText(valueText);

    setIndexWidget(editItem_->index(), editor_);
    scrollTo(editItem_->index());

    connect(editor_, &NbtRowEditor::confirmed, this, [this]() { endRowEdit(true);  });
    connect(editor_, &NbtRowEditor::cancelled, this, [this]() { endRowEdit(false); });
    connect(editor_, &NbtRowEditor::focusLost, this, [this]() { endRowEdit(false); });

    if (focusKey)
    {
        editor_->focusKeyEdit();
        editor_->selectKeyEditAll();
    }
    else
    {
        editor_->focusValueEdit();
        editor_->selectValueEditAll();
    }
}

void NbtTreeView::endRowEdit(bool confirm)
{
    if (!editItem_ || !editor_)
        return;

    setIndexWidget(editItem_->index(), nullptr);
    update(editItem_->index());

    if (confirm)
    {
        const auto keyText   = editor_->keyText();
        const auto valueText = editor_->valueText();

        bool ok = true;
        const auto value = editor_->valueEditable()
            ? nbt_utils::parseTextToTag(editor_->type(), valueText, ok)
            : nbt::Tag(editor_->type());

        if (!ok)
        {
            if (editType_ == RowEditType::Append)
                editItem_->parent()->removeRow(editItem_->row());
        }
        else if (editType_ == RowEditType::Append)
        {
            auto* parentItem = editItem_->parent();
            parentItem->removeRow(editItem_->row());
            addItem(parentItem, std::move(value), keyText);
        }
        else if (editType_ == RowEditType::Modify)
        {
            // 同时修改键与值时合并为一次撤销。
            beginEditGroup();
            changeItemKey(editItem_, keyText);

            if (editor_->valueEditable())
            {
                auto*     parentItem = editItem_->parent();
                const int row        = editItem_->row();
                const auto* currentTag = itemTag(editItem_);
                if (!currentTag || !nbt_utils::isEqualScalarTag(*currentTag, value))
                {
                    changeItemValue(editItem_, std::move(value));

                    if (parentItem)
                    {
                        if (auto* resultItem = parentItem->child(row))
                            setCurrentIndex(resultItem->index());
                    }
                    else
                    {
                        setCurrentIndex(editItem_->index());
                    }
                }
            }
            else
            {
                setCurrentIndex(editItem_->index());
            }
            endEditGroup();
        }
    }
    else
    {
        if (editType_ == RowEditType::Append)
            editItem_->parent()->removeRow(editItem_->row());
    }

    editItem_ = nullptr;
    editType_ = RowEditType::None;
    editor_->deleteLater();
    editor_   = nullptr;

    setFocus(Qt::OtherFocusReason);
}

void NbtTreeView::showContextMenu(const QPoint& position)
{
    auto* item = model_->itemFromIndex(indexAt(position));
    if (!item)
        return;

    auto* tag = itemTag(item);
    QMenu menu(this);

    if (isNamingTag(item))
        menu.addAction(EASYTR("Rename"), this, [this, item]() { invokeEditItemKey(item); });

    const bool valueEditable =
        isArrayElement(item) || (tag && nbt_utils::isTagValueEditable(tag->type(), isNamingTag(item)));
    if (valueEditable)
        menu.addAction(EASYTR("Edit Value"), this, [this, item]() { invokeEditItemValue(item); });

    const bool isRoot = !item->parent();
    if (!isRoot)
        menu.addAction(EASYTR("Delete"), this, [this, item]() { removeItem(item); });
    menu.addSeparator();

    if (tag && !nbt_utils::getAddableTagTypes(tag).empty())
    {
        auto* addMenu = menu.addMenu(tag->isCompound() ? EASYTR("Add Entry") : EASYTR("Add Element"));
        for (auto type : nbt_utils::getAddableTagTypes(tag))
        {
            addMenu->addAction(
                EASYTR(nbt::getTagTypeName(type)),
                this,
                [this, item, type]() { invokeAddItem(item, type); }
            );
        }
    }

    if (!menu.isEmpty())
        menu.exec(viewport()->mapToGlobal(position));
}

void NbtTreeView::recordStep(EditStep step)
{
    if (applyingHistory_)
        return;

    if (editGroupDepth_ > 0)
    {
        pendingAction_.steps.append(std::move(step));
        return;
    }

    EditAction action;
    action.steps.append(std::move(step));
    pushEditAction(std::move(action));
}

void NbtTreeView::beginEditGroup()
{
    ++editGroupDepth_;
}

void NbtTreeView::endEditGroup()
{
    if (editGroupDepth_ == 0 || --editGroupDepth_ > 0)
        return;
    if (pendingAction_.steps.isEmpty())
        return;

    pushEditAction(std::exchange(pendingAction_, EditAction{}));
}

void NbtTreeView::pushEditAction(EditAction action)
{
    action.id = nextActionId_++;
    undoStack_.push_back(std::move(action));
    redoStack_.clear();
    trimEditAction();
    emitHistoryChanged();
}

void NbtTreeView::trimEditAction()
{
    while (undoStack_.size() > MAX_UNDO_STEP)
        undoStack_.pop_front();
}

void NbtTreeView::clearHistory(bool clean)
{
    undoStack_.clear();
    redoStack_.clear();
    pendingAction_  = EditAction{};
    editGroupDepth_ = 0;
    savePointId_    = clean ? 0 : -1;
    emitHistoryChanged();
}

void NbtTreeView::emitHistoryChanged()
{
    emit canUndoChanged(canUndo());
    emit canRedoChanged(canRedo());

    const bool modified = isModified();
    if (modified != lastModified_)
    {
        lastModified_ = modified;
        emit modifiedChanged(modified);
    }
}

std::int64_t NbtTreeView::currentActionId() const
{
    return undoStack_.empty() ? 0 : undoStack_.back().id;
}

bool NbtTreeView::applyAction(const EditAction& action, bool reverse, QList<int>& focusPath)
{
    applyingHistory_ = true;

    bool ok = true;
    try
    {
        const int count = static_cast<int>(action.steps.size());
        for (int n = 0; ok && n < count; ++n)
            ok = applyStep(action.steps[reverse ? count - 1 - n : n], reverse, focusPath);
    }
    catch (...)
    {
        ok = false;
    }

    applyingHistory_ = false;
    return ok;
}

bool NbtTreeView::applyStep(const EditStep& step, bool reverse, QList<int>& focusPath)
{
    const bool insert =
        (step.type == EditActionType::Add && !reverse) || (step.type == EditActionType::Remove && reverse);

    switch (step.type)
    {
        case EditActionType::Add:
        case EditActionType::Remove:
        {
            if (step.path.isEmpty())
                return false;

            const auto parentPath = step.path.mid(0, step.path.size() - 1);
            focusPath = parentPath;

            if (insert)
            {
                auto* parentItem = itemFromPath(parentPath);
                if (!insertItem(parentItem, step.value, step.key, static_cast<qulonglong>(step.path.last())))
                    return false;
                focusPath = step.path;
                return true;
            }
            return removeItem(itemFromPath(step.path));
        }
        case EditActionType::Rename:
        {
            const auto& from = reverse ? step.newPath : step.path;
            const auto& to   = reverse ? step.path    : step.newPath;
            auto* item = itemFromPath(from);
            if (!item || !changeItemKey(item, reverse ? step.key : step.newKey))
                return false;
            focusPath = to;
            return true;
        }
        case EditActionType::Change:
        {
            auto* item = itemFromPath(step.path);
            if (!item || !changeItemValue(item, reverse ? step.value : step.newValue))
                return false;
            focusPath = step.path;
            return true;
        }
    }
    return false;
}
