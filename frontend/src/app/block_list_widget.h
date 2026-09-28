#pragma once

#include <qmap.h>
#include <qstring.h>
#include <qlist.h>

#include <block.hpp>
#include "trwidget.h"

class QScrollArea;
class QLayout;
class BlockListItemWidget;

class BlockListWidget : public TrWidget
{
public:
    explicit BlockListWidget(const BlockEntryMap& entries, QWidget* parent = nullptr);

    void select(const QString& blockId);
    void select(const QList<QString>& blockIds);
    void unselect(const QString& blockId);
    void unselect(const QList<QString>& blockIds);
    void selectAll();
    void unselectAll();

    Version minimumVersion() const;
    void    setMinimumVersion(const Version& version);

    bool isMultiSelectEnabled() const;
    void setMultiSelectEnabled(bool enabled);

    void selectByAttribute(BlockAttributeFilterMode mode, const BlockAttributes& attributes);

private:
    void rebuildBlocks();
    void rebuildItems();
    void setItemSelected(const QString& blockId, bool selected);

    const BlockEntryMap& entries_;
    BlockDataMap         blocks_;
    BlockDataMap         selecteds_;
    Version              minimumVersion_     = Version(1, 21, 50, 7);
    bool                 multiSelectEnabled_ = true;

    QScrollArea* scrollArea_    = nullptr;
    QWidget*     gridContainer_ = nullptr;
    QLayout*     gridLayout_    = nullptr;

    QMap<QString, BlockListItemWidget*> items_;
};
