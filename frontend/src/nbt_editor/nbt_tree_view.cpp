#include "nbt_tree_view.h"

#include <qevent.h>
#include <qmenu.h>
#include <qstandarditemmodel.h>

#include <easy_translate.hpp>

#include "nbt_row_editor.h"
#include "nbt_tree_item_delegate.h"

namespace
{

const QList<nbt::TagType>& addableTagTypes()
{
    static const QList<nbt::TagType> types
    {
        nbt::TT_BYTE, nbt::TT_SHORT, nbt::TT_INT, nbt::TT_LONG, nbt::TT_FLOAT, nbt::TT_DOUBLE,
        nbt::TT_STRING, nbt::TT_BYTE_ARRAY, nbt::TT_INT_ARRAY, nbt::TT_LONG_ARRAY,
        nbt::TT_LIST, nbt::TT_COMPOUND
    };
    return types;
}

} // namespace

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
    if (!parentItem)
        return false;

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
        if (value.type() != parentTag->listItemType() && parentTag->listItemType() != nbt::TT_END)
            return false;
        parentTag->pushBack(std::move(value));
        (void)buildItem(parentItem, false, QString(), parentTag->size() - 1, parentTag->back());
    }
    else if (parentTag->isCompound())
    {
        const auto& stdKey = key.toStdString();
        auto& target = (*parentTag)[stdKey];
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
    if (!item)
        return false;

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
        auto* parentItem = item->parent();
        for (qulonglong row = item->row() + 1; row < static_cast<qulonglong>(parentItem->rowCount()); ++row)
        {
            auto* child = parentItem->child(static_cast<int>(row));
            const qulonglong index = row - 1;
            QString valueText;
            switch (parentTag->type())
            {
                case nbt::TT_BYTE_ARRAY:
                case nbt::TT_INT_ARRAY:
                case nbt::TT_LONG_ARRAY:
                    valueText = valueTextForArrayElement(*parentTag, index);
                    break;
                case nbt::TT_LIST:
                    valueText = valueTextForTag(parentTag->at(static_cast<std::size_t>(index)));
                    break;
                default:
                    break;
            }
            const QString text = "[" + QString::number(index) + "]: " + valueText;
            child->setText(text);
            child->setData(index, TagIndexRole);
        }
        // 如果删除元素后，列表为空，则将其置为初始状态以使其能够接受不同的 Tag Type 进行重新初始化。
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
    if (!item || !isNamingTag(item))
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
    auto tag = std::move(*itemTag(item));
    const QString valueText = valueTextForTag(tag);
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
    if (!item)
        return false;

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
                const auto i = itemTagIndex(item).toULongLong();
                parentTag->getByteArray()[static_cast<std::size_t>(i)] = newValue.getByte();
                item->setText(QStringLiteral("[%1]: %2").arg(i).arg(newValue.getByte()));
                break;
            }
            case nbt::TT_INT_ARRAY:
            {
                if (!newValue.isInt()) return false;
                const auto i = itemTagIndex(item).toULongLong();
                parentTag->getIntArray()[static_cast<std::size_t>(i)] = newValue.getInt();
                item->setText(QStringLiteral("[%1]: %2").arg(i).arg(newValue.getInt()));
                break;
            }
            case nbt::TT_LONG_ARRAY:
            {
                if (!newValue.isLong()) return false;
                const auto i = itemTagIndex(item).toULongLong();
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

        auto* parentItem = item->parent();
        const int row    = item->row();
        parentItem->removeRow(row);
        parentItem->insertRow(row, createItem(false, QString(), row, target));
    }
    else if (parentTag->isCompound())
    {
        const QString& key = itemTagKey(item).toString();
        auto& target = parentTag->at(key.toStdString());
        target = std::move(newValue);

        auto* parentItem = item->parent();
        const int row    = item->row();
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
    if (!parentItem)
        return false;

    auto* parentTag = itemTag(parentItem);
    if (!parentTag)
        return false;

    if (parentTag->isArray() || parentTag->isContainer())
    {
        bool isNamingTag = true;
        if (parentTag->isArray())
        {
            if (type != arrayElementTagType(parentTag->type()))
                return false;
            isNamingTag = false;
        }
        else if (parentTag->isList())
        {
            if (type != parentTag->listItemType() && parentTag->listItemType() != nbt::TT_END)
                return false;

            if (!NbtRowEditor::editable(type, false))
                return addItem(parentItem, nbt::Tag(type));

            isNamingTag = false;
        }

        auto* item = new QStandardItem();
        item->setIcon(iconForTagType(type));
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
    if (!item || !isNamingTag(item))
        return false;

    auto* tag = itemTag(item);
    if (!tag)
        return false;

    const QString keyText   = itemTagKey(item).toString();
    const QString valueText = valueTextForTag(*tag);

    beginRowEdit(item, tag->type(), true, keyText, valueText, true, RowEditType::Modify);
    return true;
}

bool NbtTreeView::invokeEditItemValue(QStandardItem* item)
{
    if (!item)
        return false;

    nbt::TagType type;
    QString      valueText;

    if (isArrayElement(item))
    {
        auto* parentTag = itemParentTag(item);
        if (!parentTag)
            return false;
        type      = arrayElementTagType(parentTag->type());
        valueText = valueTextForArrayElement(*parentTag, itemTagIndex(item).toULongLong());
    }
    else
    {
        auto* tag = itemTag(item);
        if (!tag || !NbtRowEditor::valueEditable(tag->type(), isNamingTag(item)))
            return false;
        type      = tag->type();
        valueText = valueTextForTag(*tag);
    }

    const QString keyText = isNamingTag(item) ? itemTagKey(item).toString() : QString();
    beginRowEdit(item, type, isNamingTag(item), keyText, valueText, false, RowEditType::Modify);
    return true;
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

    QStandardItem* item = model_->itemFromIndex(currentIndex());
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
        QStandardItem* item = model_->itemFromIndex(index);
        if (item && invokeEditItemValue(item))
            return true;
    }
    return QTreeView::edit(index, trigger, e);
}

QIcon NbtTreeView::iconForTagType(nbt::TagType type)
{
    static const QIcon icons[] =
    {
        QIcon(),
        QIcon(QStringLiteral(":/icons/nbt_editor/byte.svg")),
        QIcon(QStringLiteral(":/icons/nbt_editor/short.svg")),
        QIcon(QStringLiteral(":/icons/nbt_editor/int.svg")),
        QIcon(QStringLiteral(":/icons/nbt_editor/long.svg")),
        QIcon(QStringLiteral(":/icons/nbt_editor/float.svg")),
        QIcon(QStringLiteral(":/icons/nbt_editor/double.svg")),
        QIcon(QStringLiteral(":/icons/nbt_editor/byte_array.svg")),
        QIcon(QStringLiteral(":/icons/nbt_editor/string.svg")),
        QIcon(QStringLiteral(":/icons/nbt_editor/list.svg")),
        QIcon(QStringLiteral(":/icons/nbt_editor/compound.svg")),
        QIcon(QStringLiteral(":/icons/nbt_editor/int_array.svg")),
        QIcon(QStringLiteral(":/icons/nbt_editor/long_array.svg")),
    };
    return icons[static_cast<int>(type)];
}

QString NbtTreeView::valueTextForTag(const nbt::Tag& tag)
{
    switch (tag.type())
    {
        case nbt::TT_BYTE:   return QString::number(tag.getByte());
        case nbt::TT_SHORT:  return QString::number(tag.getShort());
        case nbt::TT_INT:    return QString::number(tag.getInt());
        case nbt::TT_LONG:   return QString::number(tag.getLong());
        case nbt::TT_FLOAT:  return QString::number(tag.getFloat());
        case nbt::TT_DOUBLE: return QString::number(tag.getDouble());
        case nbt::TT_STRING: return QString::fromStdString(tag.getString());
        default:             return QString();
    }
}

QString NbtTreeView::valueTextForArrayElement(const nbt::Tag& arrayTag, qulonglong index)
{
    switch (arrayTag.type())
    {
        case nbt::TT_BYTE_ARRAY: return QString::number(arrayTag.getByteArray()[index]);
        case nbt::TT_INT_ARRAY:  return QString::number(arrayTag.getIntArray()[index]);
        case nbt::TT_LONG_ARRAY: return QString::number(arrayTag.getLongArray()[index]);
        default:                 return QString();
    }
}

nbt::TagType NbtTreeView::arrayElementTagType(nbt::TagType arrayType)
{
    switch (arrayType)
    {
        case nbt::TT_BYTE_ARRAY: return nbt::TT_BYTE;
        case nbt::TT_INT_ARRAY:  return nbt::TT_INT;
        case nbt::TT_LONG_ARRAY: return nbt::TT_LONG;
        default:                 return nbt::TT_END;
    }
}

QString NbtTreeView::defaultValueText(nbt::TagType type)
{
    if (nbt::details::isInteger(type))         return QStringLiteral("0");
    else if (nbt::details::isFloatPoint(type)) return QStringLiteral("0.0");
    else if (nbt::details::isString(type))     return QString();
    else                                       return QString();
}

nbt::Tag NbtTreeView::parseValueText(nbt::TagType type, const QString& text, bool* ok)
{
    const QString trimmed = text.trimmed();
    switch (type)
    {
        case nbt::TT_BYTE:   return nbt::Tag(static_cast<int8_t>(trimmed.toLongLong(ok)));
        case nbt::TT_SHORT:  return nbt::Tag(static_cast<int16_t>(trimmed.toLongLong(ok)));
        case nbt::TT_INT:    return nbt::Tag(static_cast<int32_t>(trimmed.toLongLong(ok)));
        case nbt::TT_LONG:   return nbt::Tag(static_cast<int64_t>(trimmed.toLongLong(ok)));
        case nbt::TT_FLOAT:  return nbt::Tag(static_cast<float>(trimmed.toDouble(ok)));
        case nbt::TT_DOUBLE: return nbt::Tag(trimmed.toDouble(ok));
        case nbt::TT_STRING: *ok = true;  return nbt::Tag(text.toStdString());
        default:             *ok = false; return nbt::Tag(type);
    }
}

nbt::Tag* NbtTreeView::itemParentTag(const QStandardItem* item)
{
    QStandardItem* parentItem = item->parent();
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
    QStandardItem* item = new QStandardItem();
    item->setIcon(iconForTagType(arrayElementTagType(parentArrayTag->type())));
    item->setText(QStringLiteral("[%1]: %2").arg(index).arg(valueTextForArrayElement(*parentArrayTag, index)));
    item->setData(false, IsNamingTagRole);
    item->setData(index, TagIndexRole);
    return item;
}

QStandardItem* NbtTreeView::createItem(bool isNamingTag, const QString& key, qulonglong index, const nbt::Tag& value)
{
    QStandardItem* item = new QStandardItem();
    item->setIcon(iconForTagType(value.type()));
    item->setText(
        isNamingTag
        ? QStringLiteral("%1: %2").arg(key).arg(valueTextForTag(value))
        : QStringLiteral("[%1]: %2").arg(index).arg(valueTextForTag(value))
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
        const QString keyText   = editor_->keyText();
        const QString valueText = editor_->valueText();

        bool ok = true;
        nbt::Tag value = editor_->valueEditable()
            ? parseValueText(editor_->type(), valueText, &ok)
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
                auto* parentItem = editItem_->parent();
                const int row    = editItem_->row();
                changeItemValue(editItem_, std::move(value));

                if (parentItem)
                {
                    if (QStandardItem* resultItem = parentItem->child(row))
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
        {
            editItem_->parent()->removeRow(editItem_->row());
        }
    }

    editItem_ = nullptr;
    editType_ = RowEditType::None;
    editor_->deleteLater();
    editor_   = nullptr;

    setFocus(Qt::OtherFocusReason);
}

void NbtTreeView::showContextMenu(const QPoint& position)
{
    QStandardItem* item = model_->itemFromIndex(indexAt(position));
    if (!item)
        return;

    auto* tag = itemTag(item);
    QMenu menu(this);

    if (tag && tag->isCompound())
    {
        QMenu* addMenu = menu.addMenu(EASYTR("Add Entry"));
        for (nbt::TagType type : addableTagTypes())
            addMenu->addAction(
                EASYTR(nbt::getTagTypeName(type)),
                this,
                [this, item, type]() { invokeAddItem(item, type); }
            );
    }
    else if (tag && tag->isList())
    {
        if (tag->listItemType() == nbt::TT_END)
        {
            QMenu* addMenu = menu.addMenu(EASYTR("Add Element"));
            for (nbt::TagType type : addableTagTypes())
                addMenu->addAction(
                    EASYTR(nbt::getTagTypeName(type)),
                    this,
                    [this, item, type]() { invokeAddItem(item, type); }
                );
        }
        else
        {
            const nbt::TagType type = tag->listItemType();
            menu.addAction(
                EASYTR("Add Element"),
                this,
                [this, item, type]() { invokeAddItem(item, type); }
            );
        }
    }
    else if (tag && tag->isArray())
    {
        const nbt::TagType type = arrayElementTagType(tag->type());
        menu.addAction(
            EASYTR("Add Element"),
            this,
            [this, item, type]() { invokeAddItem(item, type); }
        );
    }

    if (isNamingTag(item))
        menu.addAction(EASYTR("Rename"), this, [this, item]() { invokeEditItemKey(item); });

    const bool valueEditable =
        isArrayElement(item) || (tag && NbtRowEditor::valueEditable(tag->type(), isNamingTag(item)));
    if (valueEditable)
        menu.addAction(EASYTR("Edit Value"), this, [this, item]() { invokeEditItemValue(item); });

    const bool isRoot = !item->parent() && !isArrayElement(item);
    if (!isRoot)
    {
        menu.addSeparator();
        menu.addAction(EASYTR("Delete"), this, [this, item]() { removeItem(item); });
    }

    if (!menu.isEmpty())
        menu.exec(viewport()->mapToGlobal(position));
}
