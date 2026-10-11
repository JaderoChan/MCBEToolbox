#include "nbt_tree_view.h"

#include <qapplication.h>
#include <qmenu.h>
#include <qmessagebox.h>
#include <qstandarditemmodel.h>
#include <qstyleoption.h>
#include <qlineedit.h>

#include <easy_translate.hpp>

#include "nbt_row_editor.h"
#include "nbt_tree_item_delegate.h"
#include "nbt_value_utils.h"

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
    connect(qApp, &QApplication::focusChanged,            this, &NbtTreeView::handleFocusChanged);
}

void NbtTreeView::setTag(const nbt::Tag& tag, const QString& rootName)
{
    tag_      = tag;
    rootName_ = rootName;
    rebuild();
}

void NbtTreeView::setRootName(const QString& name)
{
    if (rootName_ != name)
    {
        rootName_ = name;
        rebuild();
    }
}

bool NbtTreeView::removeItem(QStandardItem* item)
{
    if (!item)
        return false;

    const QStringList path       = item->data(PathRole).toStringList();
    const int         arrayIndex = item->data(ArrayIndexRole).toInt();

    if (path.isEmpty() && arrayIndex < 0)
        return false;

    if (arrayIndex >= 0)
    {
        nbt::Tag* array = resolveTag(path);
        if (!array)
            return false;
        array->erase(static_cast<nbt::Tag::size_type>(arrayIndex));
    }
    else
    {
        nbt::Tag* container = resolveTag(QStringList(path.mid(0, path.size() - 1)));
        if (!container)
            return false;

        if (container->isCompound())
            container->erase(path.last().toStdString());
        else if (container->isList())
            container->erase(static_cast<nbt::Tag::size_type>(path.last().toInt()));
        else
            return false;
    }

    rebuild();
    emit tagChanged();
    return true;
}

bool NbtTreeView::renameKey(QStandardItem* item, const QString& newKey)
{
    if (!item)
        return false;

    const QStringList path = item->data(PathRole).toStringList();
    if (path.isEmpty() || item->data(ArrayIndexRole).toInt() >= 0)
        return false;

    if (newKey == path.last())
        return true;

    nbt::Tag* container = resolveTag(QStringList(path.mid(0, path.size() - 1)));
    if (!container || !container->isCompound())
        return false;

    if (!renameCompoundKey(*container, path.last(), newKey))
        return false;

    rebuild();
    emit tagChanged();
    return true;
}

void NbtTreeView::changeEvent(QEvent* event)
{
    QTreeView::changeEvent(event);
    if (event->type() == QEvent::LanguageChange)
        retranslateItem(rootItem());
}

void NbtTreeView::keyPressEvent(QKeyEvent* event)
{
    QStandardItem* item = model_->itemFromIndex(currentIndex());
    if (item)
    {
        const EditCapability cap = editCapabilityFor(item);
        switch (event->key())
        {
            case Qt::Key_F2:
                if (cap.keyEditable)
                {
                    beginRowEdit(item, EditKey);
                    return;
                }
                break;
            case Qt::Key_Return:
            case Qt::Key_Enter:
                if (cap.valueEditable || cap.keyEditable)
                {
                    beginRowEdit(item, cap.valueEditable ? EditValue : EditKey);
                    return;
                }
                break;
            case Qt::Key_Delete:
                removeItem(item);
                return;
            default:
                break;
        }
    }

    QTreeView::keyPressEvent(event);
}

bool NbtTreeView::edit(const QModelIndex& index, EditTrigger trigger, QEvent* event)
{
    if (!(trigger & editTriggers()))
        return QTreeView::edit(index, trigger, event);

    QStandardItem* item = model_->itemFromIndex(index);
    if (!item)
        return QTreeView::edit(index, trigger, event);

    if (
        trigger == QAbstractItemView::DoubleClicked &&
        event && event->type() == QEvent::MouseButtonDblClick &&
        delegate_)
    {
        QStyleOptionViewItem option;
        initViewItemOption(&option);
        option.rect = visualRect(index);

        const int clickX = static_cast<QMouseEvent*>(event)->pos().x();
        if (clickX < delegate_->textLeftX(option, index))
        {
            if (item->rowCount() > 0)
                setExpanded(index, !isExpanded(index));
            return true;
        }
    }

    const EditCapability cap = editCapabilityFor(item);
    if (!cap.keyEditable && !cap.valueEditable)
        return QTreeView::edit(index, trigger, event);

    beginRowEdit(item, editTargetForClick(item, index, trigger, event));
    return true;
}

QStandardItem* NbtTreeView::rootItem() const
{
    return model_->invisibleRootItem()->child(0);
}

NbtTreeView::EditCapability NbtTreeView::editCapabilityFor(QStandardItem* item)
{
    const QStringList path       = item->data(PathRole).toStringList();
    const int         arrayIndex = item->data(ArrayIndexRole).toInt();

    if (arrayIndex >= 0)
        return {false, true};

    nbt::Tag* container = resolveTag(QStringList(path.mid(0, path.size() - 1)));
    const bool keyEditable = path.isEmpty() || (container && container->isCompound());

    nbt::Tag* self = resolveTag(path);
    const bool valueEditable = self && !self->isContainer() && !self->isArray();

    return {keyEditable, valueEditable};
}

NbtTreeView::PendingKind NbtTreeView::pendingKindFor(const QStringList& path, int arrayIndex) const
{
    if (!pendingNewEntryPath_.isEmpty() && path == pendingNewEntryPath_)
        return PendingKind::CompoundEntry;

    if (pendingNewElementIndex_ >= 0)
    {
        const bool isMatch = arrayIndex >= 0
            ? (path == pendingNewElementContainer_ && arrayIndex == pendingNewElementIndex_)
            : (path == (QStringList(pendingNewElementContainer_) << QString::number(pendingNewElementIndex_)));
        if (isMatch)
            return PendingKind::Element;
    }

    return PendingKind::None;
}

NbtTreeView::EditTarget NbtTreeView::editTargetForClick(
    QStandardItem* item, const QModelIndex& index, EditTrigger trigger, QEvent* event)
{
    const QStringList path       = item->data(PathRole).toStringList();
    const int         arrayIndex = item->data(ArrayIndexRole).toInt();

    switch (pendingKindFor(path, arrayIndex))
    {
        case PendingKind::CompoundEntry: return EditKey;
        case PendingKind::Element:       return EditValue;
        case PendingKind::None:          break;
    }

    const EditCapability cap = editCapabilityFor(item);
    if (!cap.keyEditable)
        return EditValue;
    if (!cap.valueEditable)
        return EditKey;

    if (
        trigger == QAbstractItemView::DoubleClicked &&
        event && event->type() == QEvent::MouseButtonDblClick &&
        delegate_)
    {
        QStyleOptionViewItem option;
        initViewItemOption(&option);
        option.rect = visualRect(index);

        const int clickX = static_cast<QMouseEvent*>(event)->pos().x();
        return clickX < delegate_->keyBoundaryX(option, index) ? EditKey : EditValue;
    }

    return EditKey;
}

void NbtTreeView::beginRowEdit(QStandardItem* item, EditTarget focusTarget)
{
    if (!item)
        return;

    const QStringList targetPath       = item->data(PathRole).toStringList();
    const int         targetArrayIndex = item->data(ArrayIndexRole).toInt();

    closeRowEdit(true);

    item = findItemFor(targetPath, targetArrayIndex);
    if (!item)
        return;

    const QStringList path        = targetPath;
    const int         arrayIndex  = targetArrayIndex;
    const PendingKind pendingKind = pendingKindFor(path, arrayIndex);

    bool         nameEditable  = false;
    bool         valueEditable = false;
    QString      nameStr;
    QString      valueStr;
    nbt::TagType validatorType = nbt::TT_END;

    if (pendingKind == PendingKind::CompoundEntry)
    {
        nameEditable = true;
        if (nbt::Tag* self = resolveTag(path))
        {
            valueEditable = !self->isContainer() && !self->isArray();
            if (valueEditable)
            {
                valueStr      = valueText(*self);
                validatorType = self->type();
            }
        }
    }
    else if (pendingKind == PendingKind::Element)
    {
        valueEditable = true;
        if (arrayIndex >= 0)
        {
            if (nbt::Tag* array = resolveTag(path))
            {
                valueStr      = arrayElementText(*array, static_cast<nbt::Tag::size_type>(arrayIndex));
                validatorType = nbtArrayElementType(array->type());
            }
        }
        else if (nbt::Tag* self = resolveTag(path))
        {
            valueStr      = valueText(*self);
            validatorType = self->type();
        }
    }
    else
    {
        const EditCapability cap = editCapabilityFor(item);
        nameEditable  = cap.keyEditable;
        valueEditable = cap.valueEditable;
        nameStr       = path.isEmpty() ? rootName_ : path.last();

        if (arrayIndex >= 0)
        {
            if (nbt::Tag* array = resolveTag(path))
            {
                valueStr      = arrayElementText(*array, static_cast<nbt::Tag::size_type>(arrayIndex));
                validatorType = nbtArrayElementType(array->type());
            }
        }
        else if (nbt::Tag* self = resolveTag(path))
        {
            valueStr      = valueText(*self);
            validatorType = self->type();
        }
    }

    if (!nameEditable && !valueEditable)
        return;

    auto* editor = new NbtRowEditor(viewport());
    editor->setNameEditable(nameEditable);
    editor->setValueEditable(valueEditable);

    if (nameEditable)
        editor->nameEdit()->setText(nameStr);

    if (valueEditable)
    {
        editor->valueEdit()->setText(valueStr);
        if (QValidator* validator = nbtValidatorForType(validatorType, editor->valueEdit()))
            editor->valueEdit()->setValidator(validator);
    }

    connect(editor, &NbtRowEditor::confirmed, this, [this]() { closeRowEdit(true);  });
    connect(editor, &NbtRowEditor::cancelled, this, [this]() { closeRowEdit(false); });

    activeEditor_   = editor;
    activeEditItem_ = item;
    setIndexWidget(item->index(), editor);

    QLineEdit* focusEdit = nullptr;
    if (focusTarget == EditKey && nameEditable)
        focusEdit = editor->nameEdit();
    else if (valueEditable)
        focusEdit = editor->valueEdit();
    else if (nameEditable)
        focusEdit = editor->nameEdit();

    if (focusEdit)
    {
        focusEdit->setFocus(Qt::MouseFocusReason);
        focusEdit->selectAll();
    }
}

void NbtTreeView::closeRowEdit(bool commit)
{
    if (!activeEditor_ || !activeEditItem_)
        return;

    NbtRowEditor*  editor = activeEditor_;
    QStandardItem* item   = activeEditItem_;
    activeEditor_   = nullptr;
    activeEditItem_ = nullptr;

    const QStringList path        = item->data(PathRole).toStringList();
    const int         arrayIndex  = item->data(ArrayIndexRole).toInt();
    const PendingKind pendingKind = pendingKindFor(path, arrayIndex);

    const bool    nameWasEditable  = editor->nameEdit()->isVisible();
    const bool    valueWasEditable = editor->valueEdit()->isVisible();
    const QString nameValue        = editor->nameEdit()->text();
    const QString valueValue       = editor->valueEdit()->text();

    const QModelIndex itemIndex = item->index();
    setIndexWidget(itemIndex, nullptr);
    update(itemIndex);

    if (!commit)
    {
        if (pendingKind != PendingKind::None)
            cancelPendingEntry();
    }
    else
    {
        switch (pendingKind)
        {
            case PendingKind::CompoundEntry:
                finalizePendingNewEntry(nameValue, valueWasEditable ? valueValue : QString());
                break;
            case PendingKind::Element:
                finalizePendingNewElement(valueValue);
                break;
            case PendingKind::None:
                if (valueWasEditable)
                    commitValueEdit(item, valueValue);
                if (nameWasEditable)
                    commitKeyEdit(item, nameValue);
                break;
        }
    }

    setFocus(Qt::OtherFocusReason);
}

void NbtTreeView::handleFocusChanged(QWidget* /*oldWidget*/, QWidget* nowWidget)
{
    if (!activeEditor_)
        return;
    if (nowWidget && (nowWidget == activeEditor_ || activeEditor_->isAncestorOf(nowWidget)))
        return;

    NbtRowEditor* editorAtRequestTime = activeEditor_;
    QMetaObject::invokeMethod(this, [this, editorAtRequestTime]()
    {
        if (activeEditor_ == editorAtRequestTime)
            closeRowEdit(true);
    }, Qt::QueuedConnection);
}

void NbtTreeView::showContextMenu(const QPoint& position)
{
    QStandardItem* item = model_->itemFromIndex(indexAt(position));
    if (!item)
        return;

    const QStringList path       = item->data(PathRole).toStringList();
    const int         arrayIndex = item->data(ArrayIndexRole).toInt();
    nbt::Tag*         self       = resolveTag(path);
    if (!self)
        return;

    QMenu menu(this);

    if (arrayIndex < 0 && self->isCompound())
    {
        appendAddTypeActions(menu.addMenu(EASYTR("Add Entry")), item);
    }
    else if (arrayIndex < 0 && self->isList())
    {
        if (self->listItemType() == nbt::TT_END)
        {
            appendAddTypeActions(menu.addMenu(EASYTR("Add Element")), item);
        }
        else
        {
            const nbt::TagType itemType = self->listItemType();
            menu.addAction(EASYTR("Add Element"), this, [this, item, itemType]()
            { beginAddListElement(item, nbt::Tag(itemType)); });
        }
    }
    else if (arrayIndex < 0 && self->isArray())
    {
        menu.addAction(EASYTR("Add Element"), this, [this, item]()
        { beginAddArrayElement(item); });
    }

    if (arrayIndex < 0)
    {
        if (path.isEmpty())
        {
            menu.addAction(EASYTR("Rename"), this, [this, item]() { beginRowEdit(item, EditKey); });
        }
        else if (nbt::Tag* container = resolveTag(QStringList(path.mid(0, path.size() - 1))))
        {
            if (container->isCompound())
                menu.addAction(EASYTR("Rename"), this, [this, item]() { beginRowEdit(item, EditKey); });
        }
    }

    const bool isRoot = path.isEmpty() && arrayIndex < 0;
    if (!isRoot)
    {
        menu.addSeparator();
        menu.addAction(EASYTR("Delete"), this, [this, item]() { removeItem(item); });
    }

    if (!menu.isEmpty())
        menu.exec(viewport()->mapToGlobal(position));
}

void NbtTreeView::rebuild()
{
    QSet<QString> expandedPaths;
    collectExpandedPaths(rootItem(), expandedPaths);

    model_->clear();
    model_->setColumnCount(1);

    if (tag_.type() != nbt::TT_END)
    {
        QStandardItem* root = buildItem(nullptr, rootName_, tag_, QStringList());
        model_->appendRow(root);
        restoreExpandedPaths(root, expandedPaths);
        setExpanded(root->index(), true);
    }
}

QStandardItem* NbtTreeView::buildItem(
    QStandardItem* parent, const QString& key, nbt::Tag& value, const QStringList& path)
{
    auto* item = new QStandardItem();
    item->setData(path, PathRole);
    item->setData(-1, ArrayIndexRole);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    item->setIcon(iconForType(value.type()));
    updateItemText(item, key, value);

    if (parent)
        parent->appendRow(item);

    if (value.isCompound())
    {
        for (auto it = value.begin(); it != value.end(); ++it)
        {
            const QString key2 = QString::fromStdString(it.key());
            buildItem(item, key2, *it, QStringList(path) << key2);
        }
    }
    else if (value.isList())
    {
        int idx = 0;
        for (auto it = value.begin(); it != value.end(); ++it, ++idx)
            buildItem(item, QString("[%1]").arg(idx), *it, QStringList(path) << QString::number(idx));
    }
    else if (value.isArray())
    {
        const auto count = value.size();
        for (nbt::Tag::size_type i = 0; i < count; ++i)
        {
            auto* child = new QStandardItem();
            child->setData(path, PathRole);
            child->setData(static_cast<int>(i), ArrayIndexRole);
            child->setFlags(child->flags() | Qt::ItemIsEditable);
            child->setIcon(iconForType(nbtArrayElementType(value.type())));
            updateArrayElementText(child, value, i);
            item->appendRow(child);
        }
    }

    return item;
}

void NbtTreeView::retranslateItem(QStandardItem* item)
{
    if (!item)
        return;

    const QStringList path       = item->data(PathRole).toStringList();
    const int         arrayIndex = item->data(ArrayIndexRole).toInt();

    if (arrayIndex < 0)
    {
        if (nbt::Tag* self = resolveTag(path))
        {
            const QString key = path.isEmpty() ? rootName_ : path.last();
            updateItemText(item, key, *self);
        }
    }

    for (int i = 0; i < item->rowCount(); ++i)
        retranslateItem(item->child(i));
}

void NbtTreeView::updateItemText(QStandardItem* item, const QString& key, const nbt::Tag& value) const
{
    const bool    isSummary = value.isContainer() || value.isArray();
    const QString valuePart = isSummary ? summaryText(value) : valueText(value);
    const QString text      = key + QStringLiteral(": ") + valuePart;

    item->setText(text);
    item->setData(key.size() + 2, ValueStartRole);
    item->setData(isSummary, SummaryStyleRole);
}

void NbtTreeView::updateArrayElementText(QStandardItem* item, const nbt::Tag& array, nbt::Tag::size_type index) const
{
    const QString key  = QString("[%1]").arg(static_cast<qulonglong>(index));
    const QString text = key + QStringLiteral(": ") + arrayElementText(array, index);

    item->setText(text);
    item->setData(key.size() + 2, ValueStartRole);
    item->setData(false, SummaryStyleRole);
}

nbt::Tag* NbtTreeView::resolveTag(const QStringList& path)
{
    nbt::Tag* current = &tag_;
    for (const QString& segment : path)
    {
        if (!current)
            return nullptr;

        if (current->isCompound())
        {
            const std::string key = segment.toStdString();
            if (!current->contains(key))
                return nullptr;
            current = &current->at(key);
        }
        else if (current->isList())
        {
            bool ok = false;
            const int idx = segment.toInt(&ok);
            if (!ok || idx < 0 || static_cast<nbt::Tag::size_type>(idx) >= current->size())
                return nullptr;
            current = &current->at(static_cast<nbt::Tag::size_type>(idx));
        }
        else
        {
            return nullptr;
        }
    }
    return current;
}

QStandardItem* NbtTreeView::findItemByPath(QStandardItem* item, const QStringList& path) const
{
    if (!item)
        return nullptr;
    if (item->data(PathRole).toStringList() == path)
        return item;

    for (int i = 0; i < item->rowCount(); ++i)
    {
        if (auto* found = findItemByPath(item->child(i), path))
            return found;
    }
    return nullptr;
}

QStandardItem* NbtTreeView::findItemFor(const QStringList& path, int arrayIndex) const
{
    QStandardItem* found = findItemByPath(rootItem(), path);
    if (arrayIndex < 0 || !found)
        return found;

    for (int i = 0; i < found->rowCount(); ++i)
    {
        QStandardItem* child = found->child(i);
        if (child->data(ArrayIndexRole).toInt() == arrayIndex)
            return child;
    }
    return nullptr;
}

void NbtTreeView::collectExpandedPaths(QStandardItem* item, QSet<QString>& out) const
{
    if (!item)
        return;
    if (isExpanded(item->index()))
        out.insert(pathKey(item->data(PathRole).toStringList()));

    for (int i = 0; i < item->rowCount(); ++i)
        collectExpandedPaths(item->child(i), out);
}

void NbtTreeView::restoreExpandedPaths(QStandardItem* item, const QSet<QString>& paths)
{
    if (!item)
        return;
    if (paths.contains(pathKey(item->data(PathRole).toStringList())))
        setExpanded(item->index(), true);

    for (int i = 0; i < item->rowCount(); ++i)
        restoreExpandedPaths(item->child(i), paths);
}

QString NbtTreeView::pathKey(const QStringList& path) const
{
    return path.join(QChar(0x1F));
}

QIcon NbtTreeView::iconForType(nbt::TagType type) const
{
    const char* fileName = nullptr;
    switch (type)
    {
        case nbt::TT_BYTE:       fileName = "byte";       break;
        case nbt::TT_SHORT:      fileName = "short";      break;
        case nbt::TT_INT:        fileName = "int";        break;
        case nbt::TT_LONG:       fileName = "long";       break;
        case nbt::TT_FLOAT:      fileName = "float";      break;
        case nbt::TT_DOUBLE:     fileName = "double";     break;
        case nbt::TT_STRING:     fileName = "string";     break;
        case nbt::TT_BYTE_ARRAY: fileName = "byte_array"; break;
        case nbt::TT_INT_ARRAY:  fileName = "int_array";  break;
        case nbt::TT_LONG_ARRAY: fileName = "long_array"; break;
        case nbt::TT_LIST:       fileName = "list";       break;
        case nbt::TT_COMPOUND:   fileName = "compound";   break;
        default:                 return QIcon();
    }
    return QIcon(QString(":/icons/nbt_editor/%1.svg").arg(fileName));
}

QString NbtTreeView::summaryText(const nbt::Tag& value) const
{
    const auto count = value.size();
    return count == 1 ? QString(EASYTR("1 entry")) : QString(EASYTR("%1 entries")).arg(count);
}

QString NbtTreeView::valueText(const nbt::Tag& value) const
{
    switch (value.type())
    {
        case nbt::TT_BYTE:   return QString::number(value.getByte());
        case nbt::TT_SHORT:  return QString::number(value.getShort());
        case nbt::TT_INT:    return QString::number(value.getInt());
        case nbt::TT_LONG:   return QString::number(static_cast<qlonglong>(value.getLong()));
        case nbt::TT_FLOAT:  return QString::number(value.getFloat());
        case nbt::TT_DOUBLE: return QString::number(value.getDouble());
        case nbt::TT_STRING: return QString::fromStdString(value.getString());
        default:             return QString();
    }
}

QString NbtTreeView::arrayElementText(const nbt::Tag& array, nbt::Tag::size_type index) const
{
    switch (array.type())
    {
        case nbt::TT_BYTE_ARRAY: return QString::number(array.getByteArray()[index]);
        case nbt::TT_INT_ARRAY:  return QString::number(array.getIntArray()[index]);
        case nbt::TT_LONG_ARRAY: return QString::number(static_cast<qlonglong>(array.getLongArray()[index]));
        default:                 return QString();
    }
}

void NbtTreeView::commitValueEdit(QStandardItem* item, const QString& text)
{
    if (!item)
        return;

    const QStringList path       = item->data(PathRole).toStringList();
    const int         arrayIndex = item->data(ArrayIndexRole).toInt();

    if (arrayIndex >= 0)
    {
        nbt::Tag* array = resolveTag(path);
        if (array && setArrayElementValue(*array, static_cast<nbt::Tag::size_type>(arrayIndex), text))
        {
            updateArrayElementText(item, *array, static_cast<nbt::Tag::size_type>(arrayIndex));
            emit tagChanged();
        }
        return;
    }

    nbt::Tag* self = resolveTag(path);
    if (self && !self->isContainer() && !self->isArray() && setScalarValue(*self, text))
    {
        const QString key = path.isEmpty() ? (rootName_.isEmpty() ? QString(EASYTR("Root")) : rootName_) : path.last();
        updateItemText(item, key, *self);
        emit tagChanged();
    }
}

void NbtTreeView::commitKeyEdit(QStandardItem* item, const QString& text)
{
    if (!item)
        return;

    const QStringList path    = item->data(PathRole).toStringList();
    const QString     newName = text.trimmed();
    if (newName.isEmpty())
        return;

    const QString currentName = path.isEmpty() ? rootName_ : path.last();
    if (newName == currentName)
        return;

    if (path.isEmpty())
        setRootName(newName);
    else
        renameKey(item, newName);
}

bool NbtTreeView::setScalarValue(nbt::Tag& target, const QString& text)
{
    bool ok = false;

    switch (target.type())
    {
        case nbt::TT_BYTE:
        {
            const qlonglong v = parseIntegerLiteral(text, &ok);
            if (ok) target.getByte() = static_cast<int8_t>(v);
            break;
        }
        case nbt::TT_SHORT:
        {
            const qlonglong v = parseIntegerLiteral(text, &ok);
            if (ok) target.getShort() = static_cast<int16_t>(v);
            break;
        }
        case nbt::TT_INT:
        {
            const qlonglong v = parseIntegerLiteral(text, &ok);
            if (ok) target.getInt() = static_cast<int32_t>(v);
            break;
        }
        case nbt::TT_LONG:
        {
            const qlonglong v = parseIntegerLiteral(text, &ok);
            if (ok) target.getLong() = static_cast<int64_t>(v);
            break;
        }
        case nbt::TT_FLOAT:
        {
            const double v = text.toDouble(&ok);
            if (ok) target.getFloat() = static_cast<float>(v);
            break;
        }
        case nbt::TT_DOUBLE:
        {
            const double v = text.toDouble(&ok);
            if (ok) target.getDouble() = v;
            break;
        }
        case nbt::TT_STRING:
        {
            target.getString() = text.toStdString();
            ok = true;
            break;
        }
        default:
            break;
    }

    if (!ok)
        QMessageBox::warning(
            this, EASYTR("Invalid Value"), EASYTR("The entered value is not valid for this tag type.")
        );

    return ok;
}

bool NbtTreeView::setArrayElementValue(nbt::Tag& array, nbt::Tag::size_type index, const QString& text)
{
    bool ok = false;
    const qlonglong v = parseIntegerLiteral(text, &ok);

    if (ok)
    {
        switch (array.type())
        {
            case nbt::TT_BYTE_ARRAY: array.getByteArray()[index] = static_cast<int8_t>(v);  break;
            case nbt::TT_INT_ARRAY:  array.getIntArray()[index]  = static_cast<int32_t>(v); break;
            case nbt::TT_LONG_ARRAY: array.getLongArray()[index] = static_cast<int64_t>(v); break;
            default:                 ok = false;                                            break;
        }
    }

    if (!ok)
        QMessageBox::warning(
            this, EASYTR("Invalid Value"), EASYTR("The entered value is not valid for this tag type.")
        );

    return ok;
}

bool NbtTreeView::renameCompoundKey(nbt::Tag& container, const QString& oldKey, const QString& newKey)
{
    const std::string oldK = oldKey.toStdString();
    const std::string newK = newKey.toStdString();

    if (!container.contains(oldK))
        return false;

    if (container.contains(newK))
    {
        QMessageBox::warning(
            this, EASYTR("Rename Failed"), EASYTR("A sibling entry with the same name already exists.")
        );
        return false;
    }

    nbt::Tag moved = std::move(container.at(oldK));
    container.erase(oldK);
    container.insert(newK, std::move(moved));
    return true;
}

qlonglong NbtTreeView::parseIntegerLiteral(const QString& text, bool* ok) const
{
    if (text.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0)
    {
        *ok = true;
        return 1;
    }
    if (text.compare(QStringLiteral("false"), Qt::CaseInsensitive) == 0)
    {
        *ok = true;
        return 0;
    }
    return text.toLongLong(ok, 0);
}

void NbtTreeView::appendAddTypeActions(QMenu* menu, QStandardItem* containerItem)
{
    nbt::Tag*  container           = resolveTag(containerItem->data(PathRole).toStringList());
    const bool isCompoundContainer = container && container->isCompound();

    for (nbt::TagType type : nbtAddableTagTypes())
    {
        menu->addAction(EASYTR(nbt::getTagTypeName(type)), this, [this, containerItem, type, isCompoundContainer]()
        {
            if (isCompoundContainer)
                beginAddCompoundEntry(containerItem, type);
            else
                beginAddListElement(containerItem, nbt::Tag(type));
        });
    }
}

void NbtTreeView::beginAddCompoundEntry(QStandardItem* containerItem, nbt::TagType type)
{
    nbt::Tag* container = resolveTag(containerItem->data(PathRole).toStringList());
    if (!container || !container->isCompound() || container->contains(std::string()))
        return;

    container->insert(std::string(), nbt::Tag(type));
    pendingNewEntryPath_ = QStringList(containerItem->data(PathRole).toStringList()) << QString();
    rebuild();

    if (auto* newItem = findItemByPath(rootItem(), pendingNewEntryPath_))
        beginRowEdit(newItem, EditKey);
}

void NbtTreeView::finalizePendingNewEntry(const QString& key, const QString& value)
{
    const QStringList path = pendingNewEntryPath_;
    pendingNewEntryPath_.clear();

    nbt::Tag* container = resolveTag(QStringList(path.mid(0, path.size() - 1)));
    if (!container)
        return;

    const QString newKey = key.trimmed();

    if (newKey.isEmpty())
    {
        container->erase(std::string());
        rebuild();
        return;
    }

    if (container->contains(newKey.toStdString()))
    {
        QMessageBox::warning(this, EASYTR("Add Failed"), EASYTR("A sibling entry with the same name already exists."));
        container->erase(std::string());
        rebuild();
        return;
    }

    renameCompoundKey(*container, QString(), newKey);

    if (nbt::Tag* newTag = resolveTag(QStringList(path.mid(0, path.size() - 1)) << newKey))
    {
        if (!newTag->isContainer() && !newTag->isArray())
            setScalarValue(*newTag, value);
    }

    rebuild();
    emit tagChanged();
}

void NbtTreeView::beginAddListElement(QStandardItem* containerItem, nbt::Tag value)
{
    const QStringList containerPath = containerItem->data(PathRole).toStringList();
    nbt::Tag*         container     = resolveTag(containerPath);
    if (!container || !container->isList())
        return;

    const bool isScalar      = !value.isContainer() && !value.isArray();
    const int  insertedIndex = static_cast<int>(container->size());

    try
    {
        container->pushBack(std::move(value));
    }
    catch (const std::exception& e)
    {
        QMessageBox::warning(this, EASYTR("Add Failed"), QString::fromUtf8(e.what()));
        return;
    }

    if (!isScalar)
    {
        rebuild();
        emit tagChanged();
        return;
    }

    pendingNewElementContainer_ = containerPath;
    pendingNewElementIndex_     = insertedIndex;
    rebuild();

    const QStringList itemPath = QStringList(containerPath) << QString::number(insertedIndex);
    if (auto* newItem = findItemByPath(rootItem(), itemPath))
        beginRowEdit(newItem, EditValue);
}

void NbtTreeView::beginAddArrayElement(QStandardItem* containerItem)
{
    const QStringList containerPath = containerItem->data(PathRole).toStringList();
    nbt::Tag*         array         = resolveTag(containerPath);
    if (!array || !array->isArray())
        return;

    switch (array->type())
    {
        case nbt::TT_BYTE_ARRAY: array->getByteArray().push_back(0); break;
        case nbt::TT_INT_ARRAY:  array->getIntArray().push_back(0);  break;
        case nbt::TT_LONG_ARRAY: array->getLongArray().push_back(0); break;
        default: return;
    }

    pendingNewElementContainer_ = containerPath;
    pendingNewElementIndex_     = static_cast<int>(array->size()) - 1;
    rebuild();

    if (auto* newItem = findItemFor(containerPath, pendingNewElementIndex_))
        beginRowEdit(newItem, EditValue);
}

void NbtTreeView::finalizePendingNewElement(const QString& value)
{
    const QStringList containerPath = pendingNewElementContainer_;
    const int         index         = pendingNewElementIndex_;
    pendingNewElementContainer_.clear();
    pendingNewElementIndex_ = -1;

    nbt::Tag* container = resolveTag(containerPath);
    if (!container)
        return;

    if (container->isList())
        setScalarValue(container->at(static_cast<nbt::Tag::size_type>(index)), value);
    else if (container->isArray())
        setArrayElementValue(*container, static_cast<nbt::Tag::size_type>(index), value);

    rebuild();
    emit tagChanged();
}

void NbtTreeView::cancelPendingEntry()
{
    if (!pendingNewEntryPath_.isEmpty())
    {
        const QStringList path = pendingNewEntryPath_;
        pendingNewEntryPath_.clear();

        if (nbt::Tag* container = resolveTag(QStringList(path.mid(0, path.size() - 1))))
            container->erase(std::string());
    }
    else if (pendingNewElementIndex_ >= 0)
    {
        const QStringList containerPath = pendingNewElementContainer_;
        const int         index         = pendingNewElementIndex_;
        pendingNewElementContainer_.clear();
        pendingNewElementIndex_ = -1;

        if (nbt::Tag* container = resolveTag(containerPath))
            container->erase(static_cast<nbt::Tag::size_type>(index));
    }

    rebuild();
}
