#pragma once

#include <cstddef>
#include <atomic>

#include <mcbe_toolbox_api.hpp>
#include <components/adaptive_widget.h>

class QScrollArea;
class QStackedLayout;
class QThread;
class QToolButton;

class ExtendedBlockListWidget;
class GeneratorCommonOptionWidget;
class GeneratorOverlayWidget;
class MCPackGeneratorOptionWidget;

class PixelArtGeneratorPageWidget : public AdaptiveWidget
{
    Q_OBJECT

public:
    enum Mode
    {
        ImageMode,
        VideoMode
    };

    explicit PixelArtGeneratorPageWidget(Mode mode, QWidget* parent = nullptr);
    ~PixelArtGeneratorPageWidget() override;

    Mode mode() const;

    void startGenerate();
    void stopGenerate();

protected:
    void refreshText() override;
    void resizeEvent(QResizeEvent* e) override;

private:
    friend class PixelArtGeneratorWidget;

    static void progressCallback(std::size_t current, std::size_t total, bool& stop, void* userdata);

    bool validateOptionAndOpenStream();
    void onGenerationFinished(FactoryResult result);
    void returnToOptionView();
    void updateStartButtonGeometry();

    Mode mode_;

    GeneratorCommonOptionWidget* commonOptionWidget_   = nullptr;
    MCPackGeneratorOptionWidget* mcpackOptionWidget_   = nullptr;
    ExtendedBlockListWidget*     availableBlockWidget_ = nullptr;
    QScrollArea*                 optionWidget_         = nullptr;
    QToolButton*                 startButton_          = nullptr;
    GeneratorOverlayWidget*      overlayWidget_        = nullptr;
    QStackedLayout*              mainLayout_           = nullptr;

    MCStructureFactory* structureFactory_ = nullptr;
    MCFunctionFactory*  functionFactory_  = nullptr;
    QThread*            workThread_       = nullptr;
    ImageOStream*       stream_           = nullptr;
    BlockDataPair       fallbackBlock_{std::string_view(), nullptr};
    QSize               streamFrameSize_;
    int                 streamFrameCount_;

    BlockDataMap             lastAvailableBlocks_;
    std::atomic<bool>        cancelRequested_{false};
    std::atomic<std::size_t> progressCounter_{0};
    int                      progressInterval_ = 1;
};

