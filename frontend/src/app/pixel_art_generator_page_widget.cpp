#include "pixel_art_generator_page_widget.h"

#include <assert.h>
#include <stdint.h>

#include <qdir.h>
#include <qlayout.h>
#include <qmessagebox.h>
#include <qmetaobject.h>
#include <qscrollarea.h>
#include <qstackedlayout.h>
#include <qstyle.h>
#include <qthread.h>
#include <qtoolbutton.h>

#include <app_defines.h>
#include "app_configuration.h"
#include "extended_block_list_widget.h"
#include "generator_common_option_widget.h"
#include "generator_overlay_widget.h"
#include "mcpack_generator_option_widget.h"
#include <utils/qstring_path.h>

namespace
{

constexpr int START_BUTTON_SIZE   = 64;
constexpr int START_BUTTON_MARGIN = 16;
constexpr int START_BUTTON_RADIUS = START_BUTTON_SIZE / 2;

constexpr int IMAGE_MODE_PROGRESS_UPDATE_INTERVAL = 10000;
constexpr int VIDEO_MODE_PROGRESS_UPDATE_INTERVAL = 1;

constexpr int     PIXEL_ART_MAX_FRAME_SIZE_WIDTH  = 540;
constexpr int     PIXEL_ART_MAX_FRAME_SIZE_HEIGHT = 380;
constexpr Version PIXEL_ART_DEFAULT_MIN_VERSION{1, 21, 50};

} // namespace

PixelArtGeneratorPageWidget::PixelArtGeneratorPageWidget(Mode mode, QWidget* parent)
    : AdaptiveWidget(parent), mode_(mode)
{
    // 构造 UI 元素。
    commonOptionWidget_   = new GeneratorCommonOptionWidget(this);
    mcpackOptionWidget_   = new MCPackGeneratorOptionWidget(this);
    availableBlockWidget_ = new ExtendedBlockListWidget(this);
    optionWidget_         = new QScrollArea(this);
    commonOptionWidget_->setMode(
        mode_ == ImageMode ? GeneratorCommonOptionWidget::ImageMode : GeneratorCommonOptionWidget::VideoMode);
    commonOptionWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    mcpackOptionWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    availableBlockWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    availableBlockWidget_->setMinimumHeight(540);
    optionWidget_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    optionWidget_->setWidgetResizable(true);

    // 选项配置界面（包括通用选项配置界面、MC Pack 选项配置界面和可用方块选择界面）。
    auto* optionContent = new QWidget(optionWidget_);
    auto* optionLayout  = new QVBoxLayout(optionContent);
    optionLayout->setContentsMargins(0, 0, 0, 0);
    optionLayout->setSpacing(12);
    optionLayout->addWidget(commonOptionWidget_);
    optionLayout->addWidget(mcpackOptionWidget_);
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

    // 工厂实例。
    structureFactory_ = new MCStructureFactory(BlockDataMap(), DEFAULT_DESIRED_SURFACE);
    functionFactory_  = new MCFunctionFactory(BlockDataMap(),  DEFAULT_DESIRED_SURFACE);

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
        this,           &PixelArtGeneratorPageWidget::startGenerate
    );
    connect(
        overlayWidget_, &GeneratorOverlayWidget::cancelRequested,
        this,           &PixelArtGeneratorPageWidget::stopGenerate
    );
    connect(
        overlayWidget_, &GeneratorOverlayWidget::confirmed,
        this,           &PixelArtGeneratorPageWidget::returnToOptionView
    );

    commonOptionWidget_->setOutputFileExtension(QStringLiteral("mcpack"));

    // 配置默认值（未来需要根据程序配置进行设置）。
    availableBlockWidget_->setBlocks(getAppConfiguration().baseBlocks);
    commonOptionWidget_->setSurfaceDirection(DEFAULT_DESIRED_SURFACE);
    commonOptionWidget_->fallbackBlockSelector_->setCustomBlocks(getAppConfiguration().baseBlocks);
    commonOptionWidget_->fallbackBlockSelector_->setSelectedCustomBlock(DEFAULT_FALLBACK_BLOCK_ID);

    commonOptionWidget_->setMaxFrameSizeRange(
        QSize(1, 1), QSize(PIXEL_ART_MAX_FRAME_SIZE_WIDTH, PIXEL_ART_MAX_FRAME_SIZE_HEIGHT));
    commonOptionWidget_->setMaxFrameSize(
        mode_ == ImageMode
        ? QSize(DEFAULT_PIXEL_ART_IMAGE_MAX_WIDTH, DEFAULT_PIXEL_ART_IMAGE_MAX_HEIGHT)
        : QSize(DEFAULT_PIXEL_ART_VIDEO_MAX_WIDTH, DEFAULT_PIXEL_ART_VIDEO_MAX_HEIGHT)
    );

    commonOptionWidget_->setMaxFrameCountRange(1, INT32_MAX);
    commonOptionWidget_->setMaxFrameCount(360);
    commonOptionWidget_->setMaxFrameCount(DEFAULT_PIXEL_ART_VIDEO_MAX_FRAME_COUNT);

    commonOptionWidget_->setNumThreadsRange(1, 16);
    commonOptionWidget_->setNumThreads(4);

    mcpackOptionWidget_->setPackNamespace(DEFAULT_MCPACK_NAMESPACE);
    mcpackOptionWidget_->setPackVersion(DEFAULT_MCPACK_VERSION);
    mcpackOptionWidget_->setAutoFillPackName(true);
    mcpackOptionWidget_->setAutoFillPackDescription(true);
    mcpackOptionWidget_->setAssistFunctionEnabled(true);

    // 初始化文本和刷新 Start 按钮位置。
    refreshText();
    updateStartButtonGeometry();
}

PixelArtGeneratorPageWidget::~PixelArtGeneratorPageWidget()
{
    delete stream_;
    delete structureFactory_;
    delete functionFactory_;
}

PixelArtGeneratorPageWidget::Mode PixelArtGeneratorPageWidget::mode() const
{
    return mode_;
}

void PixelArtGeneratorPageWidget::startGenerate()
{
    if (workThread_)
        return;
    if (!validateOptionAndOpenStream())
        return;

    cancelRequested_.store(false, std::memory_order_relaxed);
    progressCounter_.store(0,     std::memory_order_relaxed);
    progressInterval_ =
        (mode_ == VideoMode) ? VIDEO_MODE_PROGRESS_UPDATE_INTERVAL : IMAGE_MODE_PROGRESS_UPDATE_INTERVAL;

    fallbackBlock_ = commonOptionWidget_->fallbackBlock();

    mcpackOptionWidget_->setProcessInfoForAutoFill(
        commonOptionWidget_->inputFilePath(),
        commonOptionWidget_->mode(),
        commonOptionWidget_->fallbackBlock(),
        commonOptionWidget_->surfaceDirection(),
        streamFrameSize_,
        streamFrameCount_
    );

    const auto    packType       = mcpackOptionWidget_->packType();
    const bool    usingStructure = (packType == MCPackGeneratorOptionWidget::StructurePackType);
    BlockDataMap& appliedBlocks  = usingStructure ? appliedBlocksStructure_ : appliedBlocksFunction_;
    bool&         blocksApplied  = usingStructure ? blocksAppliedStructure_ : blocksAppliedFunction_;

    const BlockDataMap& selectedBlocks = availableBlockWidget_->selecteds();
    const bool          blocksChanged  = !blocksApplied || selectedBlocks != appliedBlocks;

    MCPackGeneratorOption option{};
    option.needUpdateBlocks          = blocksChanged;
    option.blocks                    = selectedBlocks;
    option.desiredSurface            = commonOptionWidget_->surfaceDirection();
    option.fallbackBlock             = fallbackBlock_.second ? &fallbackBlock_ : nullptr;
    option.callback                  = &PixelArtGeneratorPageWidget::progressCallback;
    option.userdata                  = this;
    option.outFilePath               = toFsPath(commonOptionWidget_->outputFilePath());
    option.numThreads                = commonOptionWidget_->numThreads();
    option.assistFunctionEnabled     = mcpackOptionWidget_->assistFunctionEnabled();
    option.detachFileEnabled         = (mode_ == VideoMode);
    option.packOption.formatVersion  = 2;
    option.packOption.name           = mcpackOptionWidget_->packName().toStdString();
    option.packOption.desciption     = mcpackOptionWidget_->packDescription().toStdString();
    option.packOption.nameSpace      = mcpackOptionWidget_->packNamespace().toStdString();
    option.packOption.version        = mcpackOptionWidget_->packVersion();
    option.packOption.minVersion     = PIXEL_ART_DEFAULT_MIN_VERSION;

    if (blocksChanged)
    {
        appliedBlocks = selectedBlocks;
        blocksApplied = true;
    }

    overlayWidget_->setProgressUpdateInterval(progressInterval_);
    overlayWidget_->showProgressView();
    overlayWidget_->setVisible(true);
    overlayWidget_->raise();
    mainLayout_->setCurrentWidget(overlayWidget_);

    ImageOStream*       stream           = stream_;
    MCStructureFactory* structureFactory = structureFactory_;
    MCFunctionFactory*  functionFactory  = functionFactory_;

    workThread_ = QThread::create([this, option, stream, structureFactory, functionFactory, packType]()
    {
        FactoryResult result;
        try
        {
            if (packType == MCPackGeneratorOptionWidget::StructurePackType)
                result = generateStructurePack(*stream, *structureFactory, option);
            else
                result = generateFucntionPack(*stream, *functionFactory, option);
        }
        catch (const std::exception& e)
        {
            result = {BaseFactory::FAIL, {}, e.what()};
        }
        catch (...)
        {
            result = {BaseFactory::FAIL, {}, "Unknown exception"};
        }

        QMetaObject::invokeMethod(this, [this, result]() { onGenerationFinished(result); }, Qt::QueuedConnection);
    });
    connect(workThread_, &QThread::finished, workThread_, &QObject::deleteLater);
    workThread_->start();
}

void PixelArtGeneratorPageWidget::stopGenerate()
{
    cancelRequested_.store(true, std::memory_order_release);
}

void PixelArtGeneratorPageWidget::refreshText()
{
    startButton_->setToolTip(EASYTR("Start Generate"));

    if (mode_ == ImageMode)
    {
        commonOptionWidget_->setInputFileFilter(
            QStringLiteral("%1 (*.png *.jpg *.jpeg *.bmp *.webp *.tiff *.tif)").arg(EASYTR("Images")));
    }
    else
    {
        commonOptionWidget_->setInputFileFilter(
            QStringLiteral("%1 (*.mp4 *.avi *.mov *.mkv *.webm *.flv *.wmv *.m4v *.mpg *.mpeg)").arg(EASYTR("Videos")));
    }
    commonOptionWidget_->setOutputFileFilter(QStringLiteral("%1 (*.mcpack)").arg(EASYTR("MC Pack")));
}

void PixelArtGeneratorPageWidget::resizeEvent(QResizeEvent* e)
{
    AdaptiveWidget::resizeEvent(e);
    updateStartButtonGeometry();
}

bool PixelArtGeneratorPageWidget::validateOptionAndOpenStream()
{
    if (commonOptionWidget_->inputFilePath().isEmpty())
    {
        QMessageBox(
            QMessageBox::Warning, EASYTR("Notice"), EASYTR("Please select an input file first"),
            QMessageBox::Close, this
        ).exec();
        return false;
    }

    if (commonOptionWidget_->outputFilePath().isEmpty())
    {
        QMessageBox(
            QMessageBox::Warning, EASYTR("Notice"), EASYTR("Please specify an output file path first"),
            QMessageBox::Close, this
        ).exec();
        return false;
    }

    if (availableBlockWidget_->selecteds().empty())
    {
        QMessageBox(
            QMessageBox::Warning, EASYTR("Notice"), EASYTR("Please select at least one available block first"),
            QMessageBox::Close, this
        ).exec();
        return false;
    }

    if (!mcpackOptionWidget_->autoFillPackName() && mcpackOptionWidget_->packName().isEmpty())
    {
        QMessageBox(
            QMessageBox::Warning, EASYTR("Notice"), EASYTR("Please specify a pack name first"),
            QMessageBox::Close, this
        ).exec();
        return false;
    }

    if (mcpackOptionWidget_->packNamespace().isEmpty())
    {
        QMessageBox(
            QMessageBox::Warning, EASYTR("Notice"), EASYTR("Please specify a pack namespace first"),
            QMessageBox::Close, this
        ).exec();
        return false;
    }

    if (mcpackOptionWidget_->packVersion() == Version(0, 0, 0))
    {
        QMessageBox(
            QMessageBox::Warning, EASYTR("Notice"), EASYTR("Please input the correct pack version first"),
            QMessageBox::Close, this
        ).exec();
        return false;
    }

    // 尝试打开流。
    assert(stream_ == nullptr); // 应该每次任务结束后置空 stream_。
    const auto inputFilePath = commonOptionWidget_->inputFilePath();
    const auto maxFrameSize  = commonOptionWidget_->maxFrameSize();
    if (mode_ == ImageMode)
        stream_ = new SingleImageOStream(toFsPath(inputFilePath));
    else
        stream_ = new VideoImageOStream(toFsPath(inputFilePath), commonOptionWidget_->maxFrameCount());

    stream_->setMaxWidth(maxFrameSize.width());
    stream_->setMaxHeight(maxFrameSize.height());
    if (!stream_->isOpened())
    {
        delete stream_;
        stream_ = nullptr;
        QMessageBox(
            QMessageBox::Critical, EASYTR("Error"),
            EASYTR("Failed to open the input file, please check whether the file path and format are valid"),
            QMessageBox::Close, this
        ).exec();
        return false;
    }

    streamFrameSize_  = QSize(stream_->frameSize().width, stream_->frameSize().height);
    streamFrameCount_ = mode_ == ImageMode ? -1 : stream_->frameCount();

    return true;
}

void PixelArtGeneratorPageWidget::onGenerationFinished(FactoryResult result)
{
    delete stream_;
    stream_     = nullptr;
    workThread_ = nullptr;

    if (result.result == BaseFactory::CANCEL)
    {
        // 用户主动取消，无需确认，直接返回选项界面。
        returnToOptionView();
        return;
    }

    overlayWidget_->setEndViewInfo(
        result.result == BaseFactory::SUCCESS,
        QString::fromStdString(result.errorMessage),
        commonOptionWidget_->inputFilePath(),
        commonOptionWidget_->outputFilePath(),
        commonOptionWidget_->fallbackBlock(),
        streamFrameSize_,
        streamFrameCount_,
        commonOptionWidget_->surfaceDirection(),
        availableBlockWidget_->selecteds(),
        result.usage
    );
    overlayWidget_->showEndView();
}

void PixelArtGeneratorPageWidget::returnToOptionView()
{
    overlayWidget_->setVisible(false);
    mainLayout_->setCurrentWidget(optionWidget_);
    startButton_->raise();
}

void PixelArtGeneratorPageWidget::progressCallback(std::size_t current, std::size_t total, bool& stop, void* userdata)
{
    auto* self = static_cast<PixelArtGeneratorPageWidget*>(userdata);
    stop = self->cancelRequested_.load(std::memory_order_acquire);

    const bool isFinal = (total == 0) || (current + 1 >= total);
    const auto count   = self->progressCounter_.fetch_add(1, std::memory_order_relaxed) + 1;
    if (!isFinal && self->progressInterval_ > 1 && (count % static_cast<std::size_t>(self->progressInterval_)) != 0)
        return;

    const double percent =
        total > 0 ? (static_cast<double>(current + 1) / static_cast<double>(total)) * 100.0 : 100.0;
    QMetaObject::invokeMethod(
        self, [self, percent]() { self->overlayWidget_->setProgress(percent); }, Qt::QueuedConnection);
}

void PixelArtGeneratorPageWidget::updateStartButtonGeometry()
{
    startButton_->move(
        width()  - START_BUTTON_MARGIN - START_BUTTON_SIZE,
        height() - START_BUTTON_MARGIN - START_BUTTON_SIZE
    );
    startButton_->raise();
}
