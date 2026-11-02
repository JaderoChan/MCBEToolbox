#pragma once

#include <string>
#include <vector>

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

    void sort(SortMode mode = IdAscendingSortMode);
    SortMode sortMode() const;

    QString toMarkdown() const;

protected:
    void refreshText() override;

private:
    /** 单行缓存数据，避免重排序/刷新文本时重复查找方块与用量信息。 */
    struct RowData
    {
        std::string      programId; ///< 程序用方块 ID
        const BlockData* block = nullptr;
        std::size_t      usage = 0;
    };

    void rebuildTableWidget();
    void rebuildRows();
    void sortRows();
    void populateTable();
    void updateHeaderLabels();

    const std::string& rawBlockId(const RowData& row) const;
    QString displayBlockId(const RowData& row)        const;
    QString localizedBlockName(const RowData& row)    const;
    QString colorHexText(const RowData& row)          const;
    QIcon   blockIcon(const RowData& row)             const;
    QIcon   colorPreviewIcon(const RowData& row)      const;

    BlockDataMap     blocks_;
    SurfaceDirection surfaceDirection_ = SURFACE_DIRECTION_UP;
    BlockUsageType   blockUsage_;

    SortMode             sortMode_    = IdAscendingSortMode;
    QTableWidget*        tableWidget_ = nullptr;
    std::vector<RowData> rows_;
};
