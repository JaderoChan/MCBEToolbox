#pragma once

#include <components/adaptive_widget.h>

class QScrollArea;
class QStackedLayout;
class QThread;
class QToolButton;

class ImageOStream;

class ExtendedBlockListWidget;
class GeneratorCommonOptionWidget;
class GeneratorOverlayWidget;

class BlockImageGeneratorPageWidget : public AdaptiveWidget
{
    Q_OBJECT

public:
    enum Mode
    {
        ImageMode,
        VideoMode
    };

    explicit BlockImageGeneratorPageWidget(Mode mode, QWidget* parent = nullptr);

    Mode mode() const;

    void startGenerate();
    void stopGenerate();

protected:
    void refreshText() override;
    void resizeEvent(QResizeEvent* e) override;

private:
    friend class BlockImageGeneratorWidget;

    bool validateOptionAndOpenStream();
    void updateStartButtonGeometry();

    Mode mode_;

    GeneratorCommonOptionWidget* commonOptionWidget_   = nullptr;
    ExtendedBlockListWidget*     availableBlockWidget_ = nullptr;
    QScrollArea*                 optionWidget_         = nullptr;
    QToolButton*                 startButton_          = nullptr;
    GeneratorOverlayWidget*      overlayWidget_        = nullptr;
    QStackedLayout*              mainLayout_           = nullptr;

    QThread*      workThread_ = nullptr;
    ImageOStream* stream_     = nullptr;
};
