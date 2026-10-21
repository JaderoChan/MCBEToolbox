#include "nbt_tree_view.h"

#include <qevent.h>
#include <qmenu.h>
#include <qstandarditemmodel.h>

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
    delegate_ = new NbtTreeItemDelegate(this);
    setItemDelegate(delegate_);

    setEditTriggers(QAbstractItemView::DoubleClicked);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QTreeView::customContextMenuRequested, this, &NbtTreeView::showContextMenu);

    connect(
        selectionModel(), &QItemSelectionModel::selectionChanged, this,
        [this]() { emit selectedItemChanged(selectedItem()); }
    );
}

void NbtTreeView::setRootName(QString rootName)
{
    if (rootName_ == rootName)
        return;

    rootName_ = std::move(rootName);
    if (model_->rowCount() > 0)
    {
        auto* rootItem = model_->item(0);
        rootItem->setText(rootName_ + QStringLiteral(": "));
        rootItem->setData(rootName_, TagKeyRole);
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
    auto* parentTag = itemTag(parentItem);
    if (parentTag->isArray())
    {
        switch (parentTag->type())
        {
            case nbt::TT_BYTE_ARRAY:
                if (!value.isByte()) return false;
                parentTag->getByteArray().push_back(value.getByte());
                break;
            case nbt::TT_INT_ARRAY:
                if (!value.isInt()) return false;
                parentTag->getIntArray().push_back(value.getInt());
                break;
            case nbt::TT_LONG_ARRAY:
                if (!value.isLong()) return false;
                parentTag->getLongArray().push_back(value.getLong());
                break;
            default:
                return false;
        }

        auto* child = createArrayItem(parentTag, static_cast<qulonglong>(parentTag->size() - 1));
        parentItem->appendRow(child);
    }
    else if (parentTag->isList())
    {
        if (parentTag->listItemType() != nbt::TT_END && value.type() != parentTag->listItemType())
            return false;

        parentTag->pushBack(std::move(value));
        (void)buildItem(parentItem, false, QString(), parentTag->size() - 1, parentTag->back());
    }
    else if (parentTag->isCompound())
    {
        const auto& stdKey = key.toStdString();
        auto&       target = (*parentTag)[stdKey];
        if (!target.isEnd())
            return false;
        target = std::move(value);

        // 保证 QStandardItem 顺序与 Tag 顺序一致。
        qulonglong i = 0;
        for (const auto& [k, v] : parentTag->items())
        {
            if (k == stdKey) break;
            ++i;
        }

        auto* item = createItem(true, key, 0, target);
        if (target.isCompound())
        {
            for (auto& [k, v] : target.items())
                (void)buildItem(item, true, QString::fromStdString(k), 0, v);
        }
        parentItem->insertRow(i, item);
    }
    else
    {
        return false;
    }

    updateSummaryValue(parentItem, *parentTag);
    emit tagChanged();
    return true;
}

bool NbtTreeView::removeItem(QStandardItem* item)
{
    auto* parentTag = itemParentTag(item);
    if (!parentTag)
    {
        // 不允许直接删除根项。
        return false;
    }

    if (isNamingTag(item))
    {
        auto* parentItem = item->parent();
        parentTag->erase(itemTagKey(item).toString().toStdString());
        model_->removeRow(item->row(), item->index().parent());
        updateSummaryValue(parentItem, *parentTag);
    }
    else
    {
        parentTag->erase(static_cast<std::size_t>(itemTagIndex(item).toULongLong()));

        const bool blocked = model_->blockSignals(true);
        auto*       parentItem       = item->parent();
        const auto& parentModelIndex = parentItem->index();
        // 更新删除项之后项的索引值与文本。
        for (qulonglong row = item->row() + 1; row < static_cast<qulonglong>(parentItem->rowCount()); ++row)
        {
            const auto& index      = row - 1;
            const auto& modelIndex = model_->index(row, 0, parentModelIndex);

            QString valueText;
            if (parentTag->isArray())     valueText = nbt_utils::arrayElementText(*parentTag, index);
            else if (parentTag->isList()) valueText = nbt_utils::tagText((*parentTag)[static_cast<std::size_t>(index)]);
            const auto& text = QStringLiteral("[%1]: %2").arg(index).arg(valueText);

            model_->setData(modelIndex, text,  Qt::DisplayRole);
            model_->setData(modelIndex, index, TagIndexRole);
        }
        model_->blockSignals(blocked);

        // 如果删除元素后，列表为空，则将其置为初始状态以使其能够接受不同的 Tag 类型进行重新初始化。
        if (parentTag->isList() && parentTag->empty())
            *parentTag = nbt::Tag::list();

        model_->removeRow(item->row(), item->index().parent());
        updateSummaryValue(parentItem, *parentTag);
    }

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
    const auto& stdNewKey = newKey.toStdString();
    if (parentTag->contains(stdNewKey))
        return false;

    // 更改 Tag 中的名称。
    auto        tag       = std::move(*itemTag(item));
    const auto& valueText = nbt_utils::tagText(tag);
    parentTag->erase(itemTagKey(item).toString().toStdString());
    parentTag->insert(stdNewKey, std::move(tag));

    // 更改 Item 名称，并将其按照 Tag 顺序重新排列。
    item->setText(QStringLiteral("%1: %2").arg(newKey).arg(valueText));
    item->setData(newKey, TagKeyRole);

    qulonglong i = 0;
    for (const auto& [k, v] : parentTag->items())
    {
        if (k == stdNewKey) break;
        ++i;
    }
    item->parent()->insertRow(i, item->parent()->takeRow(item->row())[0]);

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

    if (parentTag->isArray())
    {
        switch (parentTag->type())
        {
            case nbt::TT_BYTE_ARRAY:
            {
                if (!newValue.isByte()) return false;
                const auto& i = itemTagIndex(item).toULongLong();
                parentTag->getByteArray()[static_cast<std::size_t>(i)] = newValue.getByte();
                item->setText(QStringLiteral("[%1]: %2").arg(i).arg(newValue.getByte()));
                break;
            }
            case nbt::TT_INT_ARRAY:
            {
                if (!newValue.isInt()) return false;
                const auto& i = itemTagIndex(item).toULongLong();
                parentTag->getIntArray()[static_cast<std::size_t>(i)] = newValue.getInt();
                item->setText(QStringLiteral("[%1]: %2").arg(i).arg(newValue.getInt()));
                break;
            }
            case nbt::TT_LONG_ARRAY:
            {
                if (!newValue.isLong()) return false;
                const auto& i = itemTagIndex(item).toULongLong();
                parentTag->getLongArray()[static_cast<std::size_t>(i)] = newValue.getLong();
                item->setText(QStringLiteral("[%1]: %2").arg(i).arg(newValue.getLong()));
                break;
            }
            default:
                return false;
        }
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
        const auto& key    = itemTagKey(item).toString();
        auto&       target = parentTag->at(key.toStdString());
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
        bool isNamingTag = true;
        if (parentTag->isArray())
        {
            if (type != nbt_utils::arrayElementTagType(parentTag->type()))
                return false;
            isNamingTag = false;
        }
        else if (parentTag->isList())
        {
            if (parentTag->listItemType() != nbt::TT_END && type != parentTag->listItemType())
                return false;

            if (!nbt_utils::tagEditable(type, false))
                return addItem(parentItem, nbt::Tag(type));

            isNamingTag = false;
        }

        auto* item = new QStandardItem();
        item->setIcon(nbt_utils::tagTypeIcon(type));
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

    const auto& keyText   = itemTagKey(item).toString();
    const auto& valueText = nbt_utils::tagText(*tag);

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
        type      = nbt_utils::arrayElementTagType(parentTag->type());
        valueText = nbt_utils::arrayElementText(*parentTag, itemTagIndex(item).toULongLong());
    }
    else
    {
        auto* tag = itemTag(item);
        if (!tag || !nbt_utils::tagValueEditable(tag->type(), isNamingTag(item)))
            return false;
        type      = tag->type();
        valueText = nbt_utils::tagText(*tag);
    }

    const auto& keyText = isNamingTag(item) ? itemTagKey(item).toString() : QString();
    beginRowEdit(item, type, isNamingTag(item), keyText, valueText, false, RowEditType::Modify);
    return true;
}

QStandardItem* NbtTreeView::rootItem() const
{
    return model_->item(0);
}

QStandardItem* NbtTreeView::selectedItem() const
{
    const auto& indexes = selectionModel()->selectedIndexes();
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
        const auto& key = itemTagKey(item).toString();
        return &parentTag->at(key.toStdString());
    }
    else
    {
        const auto& index = itemTagIndex(item).toULongLong();
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
    item->setIcon(nbt_utils::tagTypeIcon(nbt_utils::arrayElementTagType(parentArrayTag->type())));
    item->setText(
        QStringLiteral("[%1]: %2")
        .arg(index)
        .arg(nbt_utils::arrayElementText(*parentArrayTag, index))
    );
    item->setData(false, IsNamingTagRole);
    item->setData(index, TagIndexRole);
    return item;
}

QStandardItem* NbtTreeView::createItem(bool isNamingTag, const QString& key, qulonglong index, const nbt::Tag& value)
{
    auto* item = new QStandardItem();
    item->setIcon(nbt_utils::tagTypeIcon(value.type()));
    item->setText(
        isNamingTag
        ? QStringLiteral("%1: %2").arg(key).arg(nbt_utils::tagText(value))
        : QStringLiteral("[%1]: %2").arg(index).arg(nbt_utils::tagText(value))
    );
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
        for (std::size_t i = 0; i < value.size(); ++i)
        {
            auto* child = createArrayItem(&value, static_cast<qulonglong>(i));
            item->appendRow(child);
        }
    }
    else if (value.isList())
    {
        for (std::size_t i = 0; i < value.size(); ++i)
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

void NbtTreeView::updateSummaryValue(QStandardItem* item, const nbt::Tag& tag)
{
    if (!item)
        return;
    item->setData(static_cast<qulonglong>(tag.size()), SummaryValueRole);
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
        const auto& keyText   = editor_->keyText();
        const auto& valueText = editor_->valueText();

        bool ok = true;
        const auto& value = editor_->valueEditable()
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
            changeItemKey(editItem_, keyText);

            if (editor_->valueEditable())
            {
                auto*     parentItem = editItem_->parent();
                const int row        = editItem_->row();
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
            else
            {
                setCurrentIndex(editItem_->index());
            }
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
        isArrayElement(item) || (tag && nbt_utils::tagValueEditable(tag->type(), isNamingTag(item)));
    if (valueEditable)
        menu.addAction(EASYTR("Edit Value"), this, [this, item]() { invokeEditItemValue(item); });

    const bool isRoot = !item->parent();
    if (!isRoot)
        menu.addAction(EASYTR("Delete"), this, [this, item]() { removeItem(item); });
    menu.addSeparator();

    if (tag && !nbt_utils::addableTagTypes(tag).empty())
    {
        auto* addMenu = menu.addMenu(tag->isCompound() ? EASYTR("Add Entry") : EASYTR("Add Element"));
        for (auto type : nbt_utils::addableTagTypes(tag))
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
