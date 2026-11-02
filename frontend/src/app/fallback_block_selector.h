#pragma once

#include <qstring.h>

#include <block.hpp>
#include <components/adaptive_widget.h>

class QToolButton;
class QButtonGroup;
class BlockListWidget;

class FallbackBlockSelector : public AdaptiveWidget
{
    Q_OBJECT

public:
    enum Mode
    {
        NoneBlockMode,
        AirBlockMode,
        CustomBlockMode
    };

    explicit FallbackBlockSelector(QWidget* parent = nullptr);

    void setMode(Mode mode);
    Mode mode() const;

    void setModeButtonVisible(Mode mode, bool visible);
    bool modeButtonVisible(Mode mode) const;

    void setCustomBlocks(BlockDataMap blocks);
    const BlockDataMap& customBlocks() const;

    void setSelectedCustomBlock(const QString& blockId);

    BlockDataPair fallbackBlock() const;

protected:
    void refreshText() override;

private:
    void refreshButtonState();
    void refreshCustomButtonTextIcon();

    Mode mode_ = NoneBlockMode;

    QButtonGroup* buttonGroup_               = nullptr;
    QToolButton*  noneBlockButton_           = nullptr;
    QToolButton*  airBlockButton_            = nullptr;
    QToolButton*  customBlockButton_         = nullptr;
    QToolButton*  customBlockSelectorButton_ = nullptr;

    BlockDataMap  customBlocks_;
    BlockDataPair selectedCustomBlock_ = {std::string_view(), nullptr}; ///< 空方块
};
