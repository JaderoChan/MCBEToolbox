#pragma once

#include <components/adaptive_widget.h>

class QThread;
class ExtendedBlockListWidget;
class GeneratorCommonOptionWidget;
class GeneratorOverlayWidget;
class MCPackGeneratorOptionWidget;

class PixelArtGeneratorPageWidget : public AdaptiveWidget
{
    Q_OBJECT

public:
    explicit PixelArtGeneratorPageWidget(QWidget* parent = nullptr);

    void startGenerate();
    void stopGenerate();

protected:
    void refreshText() override;

private:
    friend class PixelArtGeneratorWidget;

    GeneratorCommonOptionWidget* commonOptionWidget_   = nullptr;
    MCPackGeneratorOptionWidget* mcpackOptionWidget_   = nullptr;
    ExtendedBlockListWidget*     availableBlockWidget_ = nullptr;
    QWidget*                     optionWidget_         = nullptr;
    GeneratorOverlayWidget*      overlayWidget_        = nullptr;

    QThread* workThread_ = nullptr;
};
