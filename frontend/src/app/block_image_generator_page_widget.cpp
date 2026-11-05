#include "block_image_generator_page_widget.h"

#include <assert.h>
#include <stdint.h>

#include <qlayout.h>
#include <qscrollarea.h>
#include <qstackedlayout.h>
#include <qstyle.h>
#include <qthread.h>
#include <qtoolbutton.h>

#include <app_defines.h>
#include <single_image_ostream.hpp>
#include <video_image_ostream.hpp>
#include "app_configuration.h"
#include "extended_block_list_widget.h"
#include "generator_common_option_widget.h"
#include "generator_overlay_widget.h"

namespace
{

constexpr int START_BUTTON_SIZE   = 64;
constexpr int START_BUTTON_MARGIN = 16;
constexpr int START_BUTTON_RADIUS = START_BUTTON_SIZE / 2;

} // namespace

BlockImageGeneratorPageWidget::BlockImageGeneratorPageWidget(Mode mode, QWidget* parent)
    : AdaptiveWidget(parent), mode_(mode)
{
    // 构造 UI 元素。
    commonOptionWidget_   = new GeneratorCommonOptionWidget(this);
    availableBlockWidget_ = new ExtendedBlockListWidget(this);
    optionWidget_         = new QScrollArea(this);
    commonOptionWidget_->setMode(
        mode_ == ImageMode ? GeneratorCommonOptionWidget::ImageMode : GeneratorCommonOptionWidget::VideoMode);
    commonOptionWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    availableBlockWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    optionWidget_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    optionWidget_->setWidgetResizable(true);

    // 选项配置界面（包括通用选项配置界面和可用方块选择界面）。
    auto* optionContent = new QWidget(optionWidget_);
    auto* optionLayout  = new QVBoxLayout(optionContent);
    optionLayout->setContentsMargins(0, 0, 0, 0);
    optionLayout->setSpacing(12);
    optionLayout->addWidget(commonOptionWidget_);
    optionLayout->addWidget(availableBlockWidget_);
    optionWidget_->setWidget(optionContent);

    // 悬浮的圆形 Start 按钮。
    startButton_ = new QToolButton(this);
    startButton_->setToolButtonStyle(Qt::ToolButtonIconOnly);
    startButton_->setCursor(Qt::PointingHandCursor);
    startButton_->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    startButton_->setFixedSize(START_BUTTON_SIZE, START_BUTTON_SIZE);
    startButton_->setIconSize(QSize(START_BUTTON_SIZE * 2 / 3, START_BUTTON_SIZE * 2 / 3));
    startButton_->setStyleSheet(QStringLiteral(
        "QToolButton { border: none; border-radius: %1px; background-color: palette(highlight); }"
        "QToolButton:hover { background-color: palette(highlight); }"
        "QToolButton:pressed { background-color: palette(dark); }"
    ).arg(START_BUTTON_RADIUS));

    // 覆盖层，用于生成时的进度条显示以及生成结束的信息展示。
    overlayWidget_ = new GeneratorOverlayWidget(this);
    overlayWidget_->setVisible(false);

    // 主布局。
    mainLayout_ = new QStackedLayout(this);
    mainLayout_->setContentsMargins(12, 12, 12, 12);
    mainLayout_->setStackingMode(QStackedLayout::StackAll);
    mainLayout_->addWidget(optionWidget_);
    mainLayout_->addWidget(overlayWidget_);
    mainLayout_->setCurrentWidget(optionWidget_);

    // Connections
    connect(
        startButton_,   &QToolButton::clicked,
        this,           &BlockImageGeneratorPageWidget::startGenerate
    );
    connect(
        overlayWidget_, &GeneratorOverlayWidget::cancelRequested,
        this,           &BlockImageGeneratorPageWidget::stopGenerate
    );

    // 配置默认值（未来需要根据程序配置进行设置）。
    availableBlockWidget_->setBlocks(getAppConfiguration().baseBlocks);
    commonOptionWidget_->setSurfaceDirection(DEFAULT_DESIRED_SURFACE);
    commonOptionWidget_->fallbackBlockSelector_->setCustomBlocks(getAppConfiguration().baseBlocks);
    commonOptionWidget_->fallbackBlockSelector_->setSelectedCustomBlock(DEFAULT_FALLBACK_BLOCK_ID);

    commonOptionWidget_->setMaxFrameSizeRange(
        QSize(1, 1),
        QSize(mode_ == ImageMode ? 4096 : 128, mode_ == ImageMode ? 4096 : 128)
    );
    commonOptionWidget_->setMaxFrameSize(
        mode_ == ImageMode
        ? QSize(DEFAULT_BLOCK_IMAGE_IMAGE_MAX_WIDTH, DEFAULT_BLOCK_IMAGE_IMAGE_MAX_HEIGHT)
        : QSize(DEFAULT_BLOCK_IMAGE_VIDEO_MAX_WIDTH, DEFAULT_BLOCK_IMAGE_VIDEO_MAX_HEIGHT)
    );

    commonOptionWidget_->setMaxFrameCountRange(1, INT32_MAX);
    commonOptionWidget_->setMaxFrameCount(360);
    commonOptionWidget_->setMaxFrameCount(DEFAULT_BLOCK_IMAGE_VIDEO_MAX_FRAME_COUNT);

    commonOptionWidget_->setNumThreadsRange(1, 16);
    commonOptionWidget_->setNumThreads(4);

    // 初始化文本和刷新 Start 按钮位置。
    refreshText();
    updateStartButtonGeometry();
}

BlockImageGeneratorPageWidget::Mode BlockImageGeneratorPageWidget::mode() const
{
    return mode_;
}

void BlockImageGeneratorPageWidget::startGenerate()
{
    qDebug() << "Start Generate";
    // if (!validateOptionAndOpenStream())
    //     return;

    overlayWidget_->showProgressView(GeneratorOverlayWidget::GeneratingStage);
    overlayWidget_->setProgress(0.0);
    overlayWidget_->setVisible(true);
    mainLayout_->setCurrentWidget(overlayWidget_);
}

void BlockImageGeneratorPageWidget::stopGenerate()
{
    qDebug() << "Cancel Generate";
    // if (!workThread_ || !workThread_->isRunning())
    //     return;

    overlayWidget_->setVisible(false);
    mainLayout_->setCurrentWidget(optionWidget_);
    updateStartButtonGeometry();
}

void BlockImageGeneratorPageWidget::refreshText()
{
    startButton_->setToolTip(EASYTR("Start Generate"));
}

void BlockImageGeneratorPageWidget::resizeEvent(QResizeEvent* e)
{
    AdaptiveWidget::resizeEvent(e);
    updateStartButtonGeometry();
}

bool BlockImageGeneratorPageWidget::validateOptionAndOpenStream()
{
    // 验证配置是否有效。
    if (workThread_ && workThread_->isRunning()) // TODO：需要弹出提示框。
        return false;

    if (commonOptionWidget_->inputFilePath().isEmpty()) // TODO：需要弹出提示框。
        return false;

    if (commonOptionWidget_->outputFilePath().isEmpty()) // TODO：需要弹出提示框。
        return false;

    if (availableBlockWidget_->selecteds().empty()) // TODO：需要弹出提示框。
        return false;

    // 尝试打开流。
    assert(stream_ == nullptr); // 应该每次任务结束后置空 stream_。
    const auto inputFilePath = commonOptionWidget_->inputFilePath();
    const auto maxFrameSize  = commonOptionWidget_->maxFrameSize();
    if (mode_ == ImageMode)
    {
    #ifdef _WIN32
        stream_ = new SingleImageOStream(inputFilePath.toStdWString());
    #else
        stream_ = new SingleImageOStream(inputFilePath.toStdString());
    #endif // _WIN32
    }
    else
    {
        const auto maxFrameCount = commonOptionWidget_->maxFrameCount();
    #ifdef _WIN32
        stream_ = new VideoImageOStream(inputFilePath.toStdWString(), maxFrameCount);
    #else
        stream_ = new VideoImageOStream(inputFilePath.toStdString(),  maxFrameCount);
    #endif

    }

    stream_->setMaxWidth(maxFrameSize.width());
    stream_->setMaxHeight(maxFrameSize.height());
    if (!stream_->isOpened())
    {
        delete stream_;
        stream_ = nullptr;
        // TODO：需要弹出提示框。
        return false;
    }

    return true;
}

void BlockImageGeneratorPageWidget::updateStartButtonGeometry()
{
    startButton_->move(
        width()  - START_BUTTON_MARGIN - START_BUTTON_SIZE,
        height() - START_BUTTON_MARGIN - START_BUTTON_SIZE
    );
    startButton_->raise();
}
