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

    static constexpr const char* DEFAULT_CUSTOM_BLOCK_ID = "minecraft:glass_group:glass";

public:
    enum Mode
    {
        NoneBlockMode,
        AirBlockMode,
        CustomBlockMode
    };

    explicit FallbackBlockSelector(BlockDataMap blocks, QWidget* parent = nullptr);

    void setMode(Mode mode);
    Mode mode() const;

    void setModeButtonVisible(Mode mode, bool visible);
    bool modeButtonVisible(Mode mode) const;

    void setSelectedCustomBlock(const QString& blockId);

    BlockDataPair fallbackBlock() const;

protected:
    void updateText(LanguageManager::Language loaded = LanguageManager::instance().loadedLanguage()) override;

private:
    void updateButtonState();
    void updateCustomButtonTextIcon();

    Mode mode_ = NoneBlockMode;

    QButtonGroup* buttonGroup_               = nullptr;
    QToolButton*  noneBlockButton_           = nullptr;
    QToolButton*  airBlockButton_            = nullptr;
    QToolButton*  customBlockButton_         = nullptr;
    QToolButton*  customBlockSelectorButton_ = nullptr;

    BlockDataMap  blocks_;
    BlockDataPair customBlock_ = {std::string_view(), nullptr}; // 空方块
};
