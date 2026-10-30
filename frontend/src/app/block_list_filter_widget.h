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
class BlockListFilterWidget : public AdaptiveWidget
{
public:
    explicit BlockListFilterWidget(const BlockEntryMap& entries, QWidget* parent = nullptr);

    BlockListWidget* blockListWidget() const;

protected:
    void updateText(LanguageManager::Language loaded = LanguageManager::instance().loadedLanguage()) override;

private:
    BlockAttributes checkedAttributes() const;
    void            applyFilter(bool supplement);
    void            updateSelectionCountLabel();
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
