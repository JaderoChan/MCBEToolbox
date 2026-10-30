#pragma once

#include <qlist.h>
#include <qmap.h>
#include <qstring.h>

#include <block.hpp>
#include <components/adaptive_widget.h>

class QLayout;
class QScrollArea;
class BlockListItemWidget;

/** 以流式网格布局形式（列宽自适应列表）显示方块信息的控件，支持对方块进行单选/多选。 */
class BlockListWidget : public AdaptiveWidget
{
    Q_OBJECT

public:
    explicit BlockListWidget(const BlockEntryMap& entries, QWidget* parent = nullptr);

    void select(const QString& blockId);
    void select(const QList<QString>& blockIds);
    void unselect(const QString& blockId);
    void unselect(const QList<QString>& blockIds);
    /** 闭区间范围选择。 */
    void selectRange(int start, int end);
    /** 闭区间范围取消选择。 */
    void unselectRange(int start, int end);
    void selectAll();
    void unselectAll();

    /** \p index 越界时不做任何事。 */
    void scrollTo(int index);
    /** \p blockId 不存在时不做任何事。 */
    void scrollTo(const QString& blockId);

    /** 通过给定的版本解析出合适的 BlockDataMap，并以此重建视图。 */
    void    setMinimumVersion(const Version& version);
    Version minimumVersion() const;

    void setMultiSelectEnabled(bool enabled);
    bool multiSelectEnabled() const;

    void excludeBlocks(const BlockAttributes& attributes, BlockAttributeFilterMode filterMode);
    void appendBlocks(const BlockAttributes& attributes, BlockAttributeFilterMode filterMode);

    const BlockDataMap& blocks() const;
    const BlockDataMap& selecteds() const;

signals:
    void selectionChanged();

protected:
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;

private:
    void rebuildBlocks();
    void rebuildItems();
    void setItemSelected(const QString& blockId, bool selected);
    void onSelectedsChanged();

    const BlockEntryMap& entries_;
    BlockDataMap         blocks_;
    BlockDataMap         selecteds_;
    Version              minimumVersion_     = Version(1, 21, 50, 7);
    bool                 multiSelectEnabled_ = true;

    QScrollArea* scrollArea_    = nullptr;
    QWidget*     gridContainer_ = nullptr;
    QLayout*     gridLayout_    = nullptr;

    QMap<QString, BlockListItemWidget*> items_;

    int  rangeSelectStart_ = -1;
    int  rangeSelectEnd_   = -1;
    bool shiftPressed_     = false;
    bool startIsSelected_  = false;
};
