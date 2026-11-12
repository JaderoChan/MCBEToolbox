#pragma once

#include <qlist.h>

#include <block.hpp>
#include <components/adaptive_widget.h>

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QVariantAnimation;
class BlockListWidget;

/** 作为 #BlockListWidget 的父控件，扩展一些功能。 */
class ExtendedBlockListWidget : public AdaptiveWidget
{
    Q_OBJECT

public:
    explicit ExtendedBlockListWidget(QWidget* parent = nullptr);

    void select(const QString& blockId);
    void select(const QList<QString>& blockIds);
    void unselect(const QString& blockId);
    void unselect(const QList<QString>& blockIds);
    void selectAll();
    void unselectAll();

    void setCheckedBlockAttributes(int blockAttributes);
    void setBlockFilterMode(BlockAttributeFilterMode mode);
    int  checkedBlockAttributes() const;
    BlockAttributeFilterMode blockFilterMode() const;

    void setFilterPanelExpanded(bool expanded);
    bool filterPanelExpanded() const;

    void setBlocks(BlockDataMap blocks);
    const BlockDataMap& blocks()    const;
    const BlockDataMap& selecteds() const;

signals:
    void checkedBlockAttributesChanged(int blockAttributes);
    void blockFilterModeChanged(BlockAttributeFilterMode mode);
    void filterPanelExpandedToggled(bool expanded);
    void selectionChanged();

protected:
    void refreshText() override;

private:
    BlockAttributes checkedAttributes() const;
    void            applyFilter(bool supplement);
    void            refreshSelectionCountLabel();
    void            toggleFilterPanel(bool expand);

    QPushButton*       selectAllButton_       = nullptr;
    QPushButton*       unselectAllButton_     = nullptr;
    QLabel*            selectionCountLabel_   = nullptr;
    QPushButton*       filterToggleButton_    = nullptr;

    QWidget*           filterPanel_           = nullptr;
    QVariantAnimation* filterPanelAnimation_  = nullptr;
    QList<QCheckBox*>  attributeCheckBoxes_;
    QLabel*            filterModeLabel_       = nullptr;
    QComboBox*         filterModeCombo_       = nullptr;
    QPushButton*       excludeButton_         = nullptr;
    QPushButton*       supplementButton_      = nullptr;

    BlockListWidget*   blockListWidget_       = nullptr;
};
