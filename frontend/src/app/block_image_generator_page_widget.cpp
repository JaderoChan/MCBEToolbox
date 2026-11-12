#include "block_image_generator_page_widget.h"

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
#include <utils/qstring_path.h>
#include "extended_block_list_widget.h"
#include "generator_common_option_widget.h"
#include "generator_overlay_widget.h"

namespace
{

constexpr int START_BUTTON_SIZE   = 64;
constexpr int START_BUTTON_MARGIN = 16;
constexpr int START_BUTTON_RADIUS = START_BUTTON_SIZE / 2;

constexpr int IMAGE_MODE_PROGRESS_UPDATE_INTERVAL = 10000;
constexpr int VIDEO_MODE_PROGRESS_UPDATE_INTERVAL = 1;

constexpr int   MAX_FRAME_COUNT_RANGE_MIN      = 1;
constexpr int   MAX_FRAME_COUNT_RANGE_MAX      = INT32_MAX;
constexpr int   NUM_THREADS_RANGE_MIN          = 1;
constexpr int   NUM_THREADS_RANGE_MAX          = 16;
constexpr QSize MAX_FRAME_SIZE_RANGE_MIN       = QSize(1,    1   );
constexpr QSize IMAGE_MAX_FRAME_SIZE_RANGE_MAX = QSize(4096, 4096);
constexpr QSize VIDEO_MAX_FRAME_SIZE_RANGE_MAX = QSize(256,  128 );

template <typename T>
inline T clamp(const T& value, const T& min, const T& max)
{
    assert(min <= max);
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

inline QSize clampSize(const QSize& value, const QSize& min, const QSize& max)
{
    return QSize(
        clamp(value.width(),  min.width(),  max.width()),
        clamp(value.height(), min.height(), max.height())
    );
}

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
    availableBlockWidget_->setMinimumHeight(540);
    optionWidget_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    optionWidget_->setWidgetResizable(true);

    // 选项配置界面（包括通用选项配置界面和可用方块选择界面）。
    auto* optionContent = new QWidget(optionWidget_);
    auto* optionLayout  = new QVBoxLayout(optionContent);
    optionLayout->setContentsMargins(6, 6, 6, 6);
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
    mainLayout_->setContentsMargins(0, 0, 0, 0);
    mainLayout_->setStackingMode(QStackedLayout::StackAll);
    mainLayout_->addWidget(optionWidget_);
    mainLayout_->addWidget(overlayWidget_);
    mainLayout_->setCurrentWidget(optionWidget_);

    // 配置硬编码值。
    commonOptionWidget_->setOutputFileExtension(mode_ == ImageMode ? QStringLiteral("png") : QStringLiteral("mp4"));
    commonOptionWidget_->setMaxFrameSizeRange(
        MAX_FRAME_SIZE_RANGE_MIN,
        mode_ == ImageMode ? IMAGE_MAX_FRAME_SIZE_RANGE_MAX : VIDEO_MAX_FRAME_SIZE_RANGE_MAX
    );
    commonOptionWidget_->setMaxFrameCountRange(MAX_FRAME_COUNT_RANGE_MIN, MAX_FRAME_COUNT_RANGE_MAX);
    commonOptionWidget_->setNumThreadsRange(NUM_THREADS_RANGE_MIN, NUM_THREADS_RANGE_MAX);
    commonOptionWidget_->fallbackBlockSelector_->setModeButtonVisible(FallbackBlockSelector::AirBlockMode, false);

    // 复原配置。
    const auto& dataCache = appConfig.dataCache;
    const auto& config    = mode_ == ImageMode ? appConfig.blockImageImageGroup : appConfig.blockImageVideoGroup;

    // 工厂实例。
    factory_ = new BlockImageFactory(BlockDataMap(), static_cast<SurfaceDirection>(config.desiredSurface));

    commonOptionWidget_->outputFileSelector_->setMode(static_cast<OutputFileSelector::Mode>(config.outputFileMode));
    commonOptionWidget_->outputFileSelector_->setCustomOutputDirectoryPath(config.customOutputDirectory);

    commonOptionWidget_->setSurfaceDirection(static_cast<SurfaceDirection>(config.desiredSurface));

    commonOptionWidget_->fallbackBlockSelector_->setCustomBlocks(dataCache.baseBlocks);
    commonOptionWidget_->fallbackBlockSelector_->setMode(static_cast<FallbackBlockSelector::Mode>(config.fallbackSelectorMode));
    commonOptionWidget_->fallbackBlockSelector_->setSelectedCustomBlock(config.selectedCustomBlock);

    commonOptionWidget_->setMaxFrameSize(clampSize(
        config.maxFrameSize,
        MAX_FRAME_SIZE_RANGE_MIN,
        mode_ == ImageMode ? IMAGE_MAX_FRAME_SIZE_RANGE_MAX : VIDEO_MAX_FRAME_SIZE_RANGE_MAX
    ));

    commonOptionWidget_->setMaxFrameCount(config.maxFrameCount);
    commonOptionWidget_->setNumThreads(config.numThreads);

    availableBlockWidget_->setBlocks(dataCache.baseBlocks);
    availableBlockWidget_->setCheckedBlockAttributes(config.checkedBlockAttributes);
    availableBlockWidget_->setBlockFilterMode(static_cast<BlockAttributeFilterMode>(config.blockFilterMode));
    availableBlockWidget_->setFilterPanelExpanded(config.filterPanelExpaned);
    availableBlockWidget_->unselectAll();
    availableBlockWidget_->select(config.selectedBlocks);

    // Connections
    connect(
        commonOptionWidget_->outputFileSelector_, &OutputFileSelector::modeChanged,
        this, [this](OutputFileSelector::Mode mode)
        {
            configRef().outputFileMode = mode;
            syncConfig();
        }
    );
    connect(
        commonOptionWidget_->outputFileSelector_, &OutputFileSelector::customOutputDirectoryChanged,
        this, [this](QString dirPath)
        {
            configRef().customOutputDirectory = dirPath;
            syncConfig();
        }
    );
    connect(
        commonOptionWidget_->fallbackBlockSelector_, &FallbackBlockSelector::modeChanged,
        this, [this](FallbackBlockSelector::Mode mode)
        {
            configRef().fallbackSelectorMode = mode;
            syncConfig();
        }
    );
    connect(
        commonOptionWidget_->fallbackBlockSelector_, &FallbackBlockSelector::selectedCustomBlockChanged,
        this, [this](QString blockId)
        {
            configRef().selectedCustomBlock = blockId;
            syncConfig();
        }
    );
    connect(
        commonOptionWidget_, &GeneratorCommonOptionWidget::maxFrameSizeChanged,
        this, [this](QSize size)
        {
            configRef().maxFrameSize = size;
            syncConfig();
        }
    );
    connect(
        commonOptionWidget_, &GeneratorCommonOptionWidget::maxFrameCountChanged,
        this, [this](int count)
        {
            configRef().maxFrameCount = count;
            syncConfig();
        }
    );
    connect(
        commonOptionWidget_, &GeneratorCommonOptionWidget::numThreadsChanged,
        this, [this](int num)
        {
            configRef().numThreads = num;
            syncConfig();
        }
    );

    connect(
        availableBlockWidget_, &ExtendedBlockListWidget::checkedBlockAttributesChanged,
        this, [this](int attributes)
        {
            configRef().checkedBlockAttributes = attributes;
            syncConfig();
        }
    );
    connect(
        availableBlockWidget_, &ExtendedBlockListWidget::blockFilterModeChanged,
        this, [this](BlockAttributeFilterMode mode)
        {
            configRef().blockFilterMode = mode;
            syncConfig();
        }
    );
    connect(
        availableBlockWidget_, &ExtendedBlockListWidget::selectionChanged,
        this, [this]()
        {
            configRef().selectedBlocks.clear();
            for (const auto& [k, v] : availableBlockWidget_->selecteds())
                configRef().selectedBlocks.append(QString::fromStdString(std::string(k)));
            syncConfig();
        }
    );
    connect(
        availableBlockWidget_, &ExtendedBlockListWidget::filterPanelExpandedToggled,
        this, [this](bool expanded)
        {
            configRef().filterPanelExpaned = expanded;
            syncConfig();
        }
    );

    connect(
        startButton_,   &QToolButton::clicked,
        this,           &BlockImageGeneratorPageWidget::startGenerate
    );
    connect(
        overlayWidget_, &GeneratorOverlayWidget::cancelRequested,
        this,           &BlockImageGeneratorPageWidget::stopGenerate
    );
    connect(
        overlayWidget_, &GeneratorOverlayWidget::confirmed,
        this,           &BlockImageGeneratorPageWidget::returnToOptionView
    );

    // 初始化文本和刷新 Start 按钮位置。
    refreshText();
    updateStartButtonGeometry();
}

BlockImageGeneratorPageWidget::~BlockImageGeneratorPageWidget()
{
    delete stream_;
    delete factory_;
}

BlockImageGeneratorPageWidget::Mode BlockImageGeneratorPageWidget::mode() const
{
    return mode_;
}

void BlockImageGeneratorPageWidget::startGenerate()
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

    const BlockDataMap& selectedBlocks = availableBlockWidget_->selecteds();
    const bool          blocksChanged  = !blocksApplied_ || selectedBlocks != appliedBlocks_;

    BlockImageGeneratorOption option{};
    option.needUpdateBlocks = blocksChanged;
    option.blocks           = selectedBlocks;
    option.desiredSurface   = commonOptionWidget_->surfaceDirection();
    option.fallbackBlock    = fallbackBlock_.second ? &fallbackBlock_ : nullptr;
    option.callback         = &BlockImageGeneratorPageWidget::progressCallback;
    option.userdata         = this;
    option.outFilePath      = toFsPath(commonOptionWidget_->outputFilePath());
    option.numThreads       = commonOptionWidget_->numThreads();
    option.generateVideo    = (mode_ == VideoMode);
    option.textureDirPath   =
        toFsPath(QDir(APP_RESOURCES_DIRPATH).absoluteFilePath(QStringLiteral(APP_BLOCK_TEXTURES_DIRPATH)));

    if (blocksChanged)
    {
        appliedBlocks_ = selectedBlocks;
        blocksApplied_ = true;
    }

    overlayWidget_->setProgressUpdateInterval(progressInterval_);
    overlayWidget_->showProgressView();
    overlayWidget_->setVisible(true);
    overlayWidget_->raise();
    mainLayout_->setCurrentWidget(overlayWidget_);

    ImageOStream*      stream  = stream_;
    BlockImageFactory* factory = factory_;

    workThread_ = QThread::create([this, option, stream, factory]()
    {
        FactoryResult result;
        try
        {
            result = generateBlockImage(*stream, *factory, option);
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

void BlockImageGeneratorPageWidget::stopGenerate()
{
    cancelRequested_.store(true, std::memory_order_release);
}

void BlockImageGeneratorPageWidget::refreshText()
{
    startButton_->setToolTip(EASYTR("Start Generate"));

    if (mode_ == ImageMode)
    {
        commonOptionWidget_->setInputFileFilter(
            QStringLiteral("%1 (*.png *.jpg *.jpeg *.bmp *.webp *.tiff *.tif)").arg(EASYTR("Images")));
        commonOptionWidget_->setOutputFileFilter(QStringLiteral("%1 (*.png)").arg(EASYTR("PNG Image")));
    }
    else
    {
        commonOptionWidget_->setInputFileFilter(
            QStringLiteral("%1 (*.mp4 *.avi *.mov *.mkv *.webm *.flv *.wmv *.m4v *.mpg *.mpeg)").arg(EASYTR("Videos")));
        commonOptionWidget_->setOutputFileFilter(QStringLiteral("%1 (*.mp4)").arg(EASYTR("MP4 Video")));
    }
}

void BlockImageGeneratorPageWidget::resizeEvent(QResizeEvent* e)
{
    AdaptiveWidget::resizeEvent(e);
    updateStartButtonGeometry();
}

bool BlockImageGeneratorPageWidget::validateOptionAndOpenStream()
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

void BlockImageGeneratorPageWidget::onGenerationFinished(FactoryResult result)
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

void BlockImageGeneratorPageWidget::returnToOptionView()
{
    overlayWidget_->setVisible(false);
    mainLayout_->setCurrentWidget(optionWidget_);
    startButton_->raise();
}

void BlockImageGeneratorPageWidget::progressCallback(std::size_t current, std::size_t total, bool& stop, void* userdata)
{
    auto* self = static_cast<BlockImageGeneratorPageWidget*>(userdata);
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

void BlockImageGeneratorPageWidget::updateStartButtonGeometry()
{
    startButton_->move(
        width()  - START_BUTTON_MARGIN - START_BUTTON_SIZE,
        height() - START_BUTTON_MARGIN - START_BUTTON_SIZE
    );
    startButton_->raise();
}

AppConfiguration::CommonOptionConfig& BlockImageGeneratorPageWidget::configRef() const
{
    return mode_ == ImageMode ? appConfig.blockImageImageGroup : appConfig.blockImageVideoGroup;
}

void BlockImageGeneratorPageWidget::syncConfig() const
{
    appConfig.saveLater(
        mode_ == ImageMode
        ? AppConfiguration::BlockImageImageGroup
        : AppConfiguration::BlockImageVideoGroup
    );
}
