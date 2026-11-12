#include "generator_page_widget_base.h"

#include <assert.h>

#include <qlayout.h>
#include <qmessagebox.h>
#include <qmetaobject.h>
#include <qscrollarea.h>
#include <qstackedlayout.h>
#include <qstyle.h>
#include <qthread.h>
#include <qtoolbutton.h>

#include "extended_block_list_widget.h"
#include "generator_common_option_widget.h"
#include "generator_overlay_widget.h"

namespace
{

constexpr int START_BUTTON_SIZE   = 64;
constexpr int START_BUTTON_MARGIN = 16;
constexpr int START_BUTTON_RADIUS = START_BUTTON_SIZE / 2;

constexpr int IMAGE_MODE_PROGRESS_UPDATE_INTERVAL = 10000;

} // namespace

GeneratorPageWidgetBase::GeneratorPageWidgetBase(Mode mode, QWidget* parent)
    : AdaptiveWidget(parent), mode_(mode)
{
    // 构造 UI 元素。
    commonOptionWidget_   = new GeneratorCommonOptionWidget(this);
    availableBlockWidget_ = new ExtendedBlockListWidget(this);
    optionWidget_         = new QScrollArea(this);
    commonOptionWidget_->setMode(mode_ == ImageMode ? GeneratorCommonOptionWidget::ImageMode : GeneratorCommonOptionWidget::VideoMode);
    commonOptionWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    availableBlockWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    availableBlockWidget_->setMinimumHeight(540);
    optionWidget_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    optionWidget_->setWidgetResizable(true);

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

    connect(startButton_,   &QToolButton::clicked,                    this, &GeneratorPageWidgetBase::startGenerate);
    connect(overlayWidget_, &GeneratorOverlayWidget::cancelRequested, this, &GeneratorPageWidgetBase::stopGenerate);
    connect(overlayWidget_, &GeneratorOverlayWidget::confirmed,       this, &GeneratorPageWidgetBase::returnToOptionView);
}

GeneratorPageWidgetBase::~GeneratorPageWidgetBase()
{
    delete stream_;
}

GeneratorPageWidgetBase::Mode GeneratorPageWidgetBase::mode() const
{
    return mode_;
}

void GeneratorPageWidgetBase::setupGeneratorUi()
{
    // 选项配置界面（通用选项配置界面、派生类的额外选项界面、可用方块选择界面）。
    auto* optionContent = new QWidget(optionWidget_);
    auto* optionLayout  = new QVBoxLayout(optionContent);
    optionLayout->setContentsMargins(6, 6, 6, 6);
    optionLayout->setSpacing(12);
    optionLayout->addWidget(commonOptionWidget_);
    if (QWidget* extraWidget = createExtraOptionWidget())
        optionLayout->addWidget(extraWidget);
    optionLayout->addWidget(availableBlockWidget_);
    optionWidget_->setWidget(optionContent);

    // 回填通用配置。
    const auto& dataCache = appConfig.dataCache;
    const auto& config    = commonConfigRef();

    commonOptionWidget_->outputFileSelector_->setMode(static_cast<OutputFileSelector::Mode>(config.outputFileMode));
    commonOptionWidget_->outputFileSelector_->setCustomOutputDirectoryPath(config.customOutputDirectory);

    commonOptionWidget_->setSurfaceDirection(static_cast<SurfaceDirection>(config.desiredSurface));

    commonOptionWidget_->fallbackBlockSelector_->setCustomBlocks(dataCache.baseBlocks);
    commonOptionWidget_->fallbackBlockSelector_->setMode(static_cast<FallbackBlockSelector::Mode>(config.fallbackSelectorMode));
    commonOptionWidget_->fallbackBlockSelector_->setSelectedCustomBlock(config.selectedCustomBlock);

    // setMaxFrameSizeRange() 应该由派生类在调用本函数前设置好，setMaxFrameSize() 内部会按该范围自动钳位。
    commonOptionWidget_->setMaxFrameSize(config.maxFrameSize);
    commonOptionWidget_->setMaxFrameCount(config.maxFrameCount);
    commonOptionWidget_->setNumThreads(config.numThreads);

    availableBlockWidget_->setBlocks(dataCache.baseBlocks);
    availableBlockWidget_->setCheckedBlockAttributes(config.checkedBlockAttributes);
    availableBlockWidget_->setBlockFilterMode(static_cast<BlockAttributeFilterMode>(config.blockFilterMode));
    availableBlockWidget_->setFilterPanelExpanded(config.filterPanelExpanded);
    availableBlockWidget_->unselectAll();
    availableBlockWidget_->select(config.selectedBlocks);

    // Connections。
    GENERATOR_CONFIG_CONNECT(
        commonOptionWidget_->outputFileSelector_, &OutputFileSelector::modeChanged,
        (OutputFileSelector::Mode mode), commonConfigRef().outputFileMode = mode
    );
    GENERATOR_CONFIG_CONNECT(
        commonOptionWidget_->outputFileSelector_, &OutputFileSelector::customOutputDirectoryChanged,
        (QString dirPath), commonConfigRef().customOutputDirectory = dirPath
    );
    GENERATOR_CONFIG_CONNECT(
        commonOptionWidget_->fallbackBlockSelector_, &FallbackBlockSelector::modeChanged,
        (FallbackBlockSelector::Mode mode), commonConfigRef().fallbackSelectorMode = mode
    );
    GENERATOR_CONFIG_CONNECT(
        commonOptionWidget_->fallbackBlockSelector_, &FallbackBlockSelector::selectedCustomBlockChanged,
        (QString blockId), commonConfigRef().selectedCustomBlock = blockId
    );
    GENERATOR_CONFIG_CONNECT(
        commonOptionWidget_, &GeneratorCommonOptionWidget::maxFrameSizeChanged,
        (QSize size), commonConfigRef().maxFrameSize = size
    );
    GENERATOR_CONFIG_CONNECT(
        commonOptionWidget_, &GeneratorCommonOptionWidget::maxFrameCountChanged,
        (int count), commonConfigRef().maxFrameCount = count
    );
    GENERATOR_CONFIG_CONNECT(
        commonOptionWidget_, &GeneratorCommonOptionWidget::numThreadsChanged,
        (int num), commonConfigRef().numThreads = num
    );
    GENERATOR_CONFIG_CONNECT(
        availableBlockWidget_, &ExtendedBlockListWidget::checkedBlockAttributesChanged,
        (int attributes), commonConfigRef().checkedBlockAttributes = attributes
    );
    GENERATOR_CONFIG_CONNECT(
        availableBlockWidget_, &ExtendedBlockListWidget::blockFilterModeChanged,
        (BlockAttributeFilterMode mode), commonConfigRef().blockFilterMode = mode
    );
    GENERATOR_CONFIG_CONNECT(
        availableBlockWidget_, &ExtendedBlockListWidget::filterPanelExpandedToggled,
        (bool expanded), commonConfigRef().filterPanelExpanded = expanded
    );
    connect(
        availableBlockWidget_, &ExtendedBlockListWidget::selectionChanged,
        this, [this]()
        {
            auto& selectedBlocks = commonConfigRef().selectedBlocks;
            selectedBlocks.clear();
            for (const auto& [id, blcok] : availableBlockWidget_->selecteds())
                selectedBlocks.append(QString::fromStdString(std::string(id)));
            syncConfig();
        }
    );

    refreshText();
    updateStartButtonGeometry();
}

void GeneratorPageWidgetBase::startGenerate()
{
    if (workThread_)
        return;
    if (!validateOptionAndOpenStream())
        return;

    cancelRequested_.store(false, std::memory_order_relaxed);
    progressCounter_.store(0,     std::memory_order_relaxed);
    progressInterval_ = (mode_ == VideoMode) ? videoProgressUpdateInterval() : IMAGE_MODE_PROGRESS_UPDATE_INTERVAL;

    fallbackBlock_ = commonOptionWidget_->fallbackBlock();

    overlayWidget_->setProgressUpdateInterval(progressInterval_);
    overlayWidget_->showProgressView();
    overlayWidget_->setVisible(true);
    overlayWidget_->raise();
    mainLayout_->setCurrentWidget(overlayWidget_);

    launchGenerateThread();
}

void GeneratorPageWidgetBase::stopGenerate()
{
    cancelRequested_.store(true, std::memory_order_release);
}

void GeneratorPageWidgetBase::refreshText()
{
    startButton_->setToolTip(EASYTR("Start Generate"));
    refreshFileFilters();
}

void GeneratorPageWidgetBase::resizeEvent(QResizeEvent* e)
{
    AdaptiveWidget::resizeEvent(e);
    updateStartButtonGeometry();
}

bool GeneratorPageWidgetBase::validateOptionAndOpenStream()
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

    if (!validateExtraOptions())
        return false;

    // 尝试打开流。
    assert(stream_ == nullptr); // 应该每次任务结束后置空 stream_。
    const auto inputFilePath = commonOptionWidget_->inputFilePath();
    const auto maxFrameSize  = commonOptionWidget_->maxFrameSize();
    stream_ = createInputStream(inputFilePath);

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

void GeneratorPageWidgetBase::onGenerationFinished(FactoryResult result)
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

void GeneratorPageWidgetBase::returnToOptionView()
{
    overlayWidget_->setVisible(false);
    mainLayout_->setCurrentWidget(optionWidget_);
    startButton_->raise();
}

void GeneratorPageWidgetBase::progressCallback(std::size_t current, std::size_t total, bool& stop, void* userdata)
{
    auto* self = static_cast<GeneratorPageWidgetBase*>(userdata);
    stop = self->cancelRequested_.load(std::memory_order_acquire);

    const bool isFinal = (total == 0) || (current + 1 >= total);
    const auto count   = self->progressCounter_.fetch_add(1, std::memory_order_relaxed) + 1;
    if (!isFinal && self->progressInterval_ > 1 && (count % static_cast<std::size_t>(self->progressInterval_)) != 0)
        return;

    const double percent =total > 0 ? (static_cast<double>(current + 1) / static_cast<double>(total)) * 100.0 : 100.0;
    QMetaObject::invokeMethod(self, [self, percent]() { self->overlayWidget_->setProgress(percent); }, Qt::QueuedConnection);
}

void GeneratorPageWidgetBase::updateStartButtonGeometry()
{
    startButton_->move(
        width()  - START_BUTTON_MARGIN - START_BUTTON_SIZE,
        height() - START_BUTTON_MARGIN - START_BUTTON_SIZE
    );
    startButton_->raise();
}
