#pragma once

#include <qicon.h>
#include <qstring.h>

#include <base_factory.hpp>
#include <components/adaptive_widget.h>

class QTableWidget;

/** 以表格形式展示方块用量信息的控件。 */
class BlockUsageWidget : public AdaptiveWidget
{
    Q_OBJECT

public:
    using BlockUsageType = BaseFactory::BlockUsageMap;

    /** 表格排序方式。 */
    enum SortMode
    {
        IdAscendingSortMode,    ///< 按方块 ID（游戏内）字典序升序排序
        IdDescendingSortMode,   ///< 按方块 ID（游戏内）字典序降序排序
        UsageAscendingSortMode, ///< 按方块用量升序排序（由少到多）
        UsageDescendingSortMode ///< 按方块用量降序排序（由多到少）
    };

    explicit BlockUsageWidget(QWidget* parent = nullptr);

    void setBlocks(BlockDataMap blocks);
    const BlockDataMap& blocks() const;

    void setSurfaceDirection(SurfaceDirection direction);
    SurfaceDirection surfaceDirection() const;

    void setBlockUsage(BlockUsageType usage);
    const BlockUsageType& blockUsage() const;

    void setSortMode(SortMode mode);
    SortMode sortMode() const;
    void sort();

    QString toMarkdown() const;

signals:
    void sortModeChanged(SortMode mode);

protected:
    void refreshText() override;

private:
    void rebuildTableWidget();
    void updateHeaderLabels();
    QIcon colorPreviewIcon(Rgb color);

    BlockDataMap     blocks_;
    SurfaceDirection surfaceDirection_ = SURFACE_DIRECTION_UP;
    BlockUsageType   blockUsage_;

    SortMode         sortMode_    = IdAscendingSortMode;
    QTableWidget*    tableWidget_ = nullptr;
};
