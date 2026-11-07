#include "generator_overlay_widget.h"

#include <qdesktopservices.h>
#include <qdir.h>
#include <qevent.h>
#include <qfileinfo.h>
#include <qfont.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qstackedwidget.h>
#include <qstyle.h>
#include <qurl.h>

#include <components/image_grid_progress_bar.h>
#include "block_attribute_text.h"
#include "extended_block_usage_widget.h"

namespace
{

constexpr int PROGRESS_GRID_ROWS = 8;
constexpr int PROGRESS_GRID_COLS = 8;

} // namespace

GeneratorOverlayWidget::GeneratorOverlayWidget(QWidget* parent)
    : AdaptiveWidget(parent)
{
    // 构建 Progress View。
    progressBar_ = new ImageGridProgressBar(this);
    progressBar_->setImage(QPixmap(":/icons/app/minecraft_icon.png"));
    progressBar_->setGridSize(PROGRESS_GRID_ROWS, PROGRESS_GRID_COLS);
    progressBar_->setFixedSize(256, 256);

    generatingLabel_ = new QLabel(this);
    eportingLabel_   = new QLabel(this);
    for (auto& label : {generatingLabel_, eportingLabel_})
    {
        auto font = label->font();
        font.setPointSizeF(font.pointSizeF() * 0.85);
        label->setFont(font);
        label->setAlignment(Qt::AlignHCenter);
    }

    labelWidget_ = new QStackedWidget(this);
    labelWidget_->setContentsMargins(0, 0, 0, 0);
    labelWidget_->addWidget(generatingLabel_);
    labelWidget_->addWidget(eportingLabel_);

    cancelButton_ = new QPushButton(this);
    connect(cancelButton_, &QPushButton::clicked, this, [this]() { emit cancelRequested(); });

    progressWidget_      = new QWidget(this);
    auto* progressLayout = new QVBoxLayout(progressWidget_);
    progressLayout->setContentsMargins(0, 0, 0, 0);
    progressLayout->addWidget(progressBar_,  0, Qt::AlignCenter);
    progressLayout->addWidget(labelWidget_,  0, Qt::AlignCenter);
    progressLayout->addWidget(cancelButton_, 0, Qt::AlignCenter);

    // 构建 End View。
    endWidgetTitleLabel_    = new QLabel(this);
    errorMessageLabel_      = new QLabel(this);
    confirmButton_          = new QPushButton(this);
    openOutputFolderButton_ = new QPushButton(this);
    viewBlockUsageButton_   = new QPushButton(this);

    auto endWidgetTitleFont = endWidgetTitleLabel_->font();
    endWidgetTitleFont.setBold(true);
    endWidgetTitleFont.setPointSizeF(endWidgetTitleFont.pointSizeF() * 1.8);
    endWidgetTitleLabel_->setFont(endWidgetTitleFont);
    endWidgetTitleLabel_->setAlignment(Qt::AlignCenter);

    errorMessageLabel_->setWordWrap(true);
    errorMessageLabel_->setAlignment(Qt::AlignCenter);
    errorMessageLabel_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    confirmButton_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    confirmButton_->setMinimumSize(120, 60);
    auto confirmFont = confirmButton_->font();
    confirmFont.setBold(true);
    confirmFont.setPointSizeF(confirmFont.pointSizeF() * 1.15);
    confirmButton_->setFont(confirmFont);

    endExtraButtonWidget_      = new QWidget(this);
    auto* endExtraButtonLayout = new QHBoxLayout(endExtraButtonWidget_);
    endExtraButtonLayout->setContentsMargins(0, 0, 0, 0);
    endExtraButtonLayout->setSpacing(8);
    for (auto& button : {openOutputFolderButton_, viewBlockUsageButton_})
    {
        button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        auto font = button->font();
        font.setPointSizeF(font.pointSizeF() * 0.85);
        button->setFont(font);
        endExtraButtonLayout->addWidget(button);
    }

    auto* endButtonWidget = new QWidget(this);
    auto* endButtonLayout = new QVBoxLayout(endButtonWidget);
    endButtonLayout->setContentsMargins(0, 0, 0, 0);
    endButtonLayout->setSpacing(4);
    endButtonLayout->addWidget(confirmButton_);
    endButtonLayout->addWidget(endExtraButtonWidget_);

    connect(openOutputFolderButton_, &QPushButton::clicked, this, &GeneratorOverlayWidget::onOpenOutputFolderClicked);
    connect(viewBlockUsageButton_,   &QPushButton::clicked, this, &GeneratorOverlayWidget::onViewBlockUsageClicked);
    connect(confirmButton_,          &QPushButton::clicked, this, [this]() { emit confirmed(); });

    endWidget_ = new QWidget(this);
    endLayout_ = new QVBoxLayout(endWidget_);
    endLayout_->setContentsMargins(0, 0, 0, 0);
    endLayout_->setSpacing(2);
    endLayout_->addWidget(endWidgetTitleLabel_, 0, Qt::AlignCenter);
    endLayout_->addWidget(errorMessageLabel_,   0, Qt::AlignCenter);
    endLayout_->addWidget(endButtonWidget,      0, Qt::AlignCenter);

    // 构建 Stacked 容器。
    container_ = new QStackedWidget(this);
    container_->setContentsMargins(0, 0, 0, 0);
    container_->addWidget(progressWidget_);
    container_->addWidget(endWidget_);

    // 面板容器。
    panel_ = new QWidget(this);
    panel_->setObjectName(QStringLiteral("generatorOverlayPanel"));
    panel_->setAttribute(Qt::WA_StyledBackground, true);
    panel_->setStyleSheet(
        QStringLiteral("#generatorOverlayPanel { background-color: rgba(20, 20, 20, 220); border-radius: 12px; }"));
    auto* panelLayout = new QVBoxLayout(panel_);
    panelLayout->setContentsMargins(32, 32, 32, 32);
    panelLayout->addWidget(container_);

    // 窗口样式与主布局。
    setObjectName(QStringLiteral("generatorOverlayWidget"));
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral("#generatorOverlayWidget { background-color: rgba(0, 0, 0, 120); }"));

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(panel_, 0, Qt::AlignCenter);

    refreshText();
}

void GeneratorOverlayWidget::setProgressUpdateInterval(int interval)
{
    progressUpdateInterval_ = qMax(1, interval);
}

int GeneratorOverlayWidget::progressUpdateInterval() const
{
    return progressUpdateInterval_;
}

void GeneratorOverlayWidget::setProgress(double progress)
{
    progressBar_->setProgress(progress);
    const bool exporting = progress >= 100.0;
    if (exporting)
    {
        labelWidget_->setCurrentWidget(eportingLabel_);
        cancelButton_->setVisible(false);
    }
    else
    {
        labelWidget_->setCurrentWidget(generatingLabel_);
        cancelButton_->setVisible(true);
        generatingLabel_->setText(
            QStringLiteral("%1 %2%")
                .arg(EASYTR("Generating"))
                .arg(progress, 5, 'f', 2, QLatin1Char('0'))
        );
    }
}

void GeneratorOverlayWidget::setEndViewInfo(
    bool                       success,
    QString                    errorMessage,
    QString                    inputFilePath,
    QString                    outputFilePath,
    BlockDataPair              fallbackBlock,
    QSize                      frameSize,
    int                        frameCount,
    SurfaceDirection           surfaceDirection,
    BlockDataMap               blocks,
    BaseFactory::BlockUsageMap blockUsage)
{
    endViewInfo_.success          = success;
    endViewInfo_.errorMessage     = std::move(errorMessage);
    endViewInfo_.inputFilePath    = std::move(inputFilePath);
    endViewInfo_.outputFilePath   = std::move(outputFilePath);
    endViewInfo_.fallbackBlock    = std::move(fallbackBlock);
    endViewInfo_.frameSize        = std::move(frameSize);
    endViewInfo_.frameCount       = frameCount;
    endViewInfo_.surfaceDirection = surfaceDirection;
    endViewInfo_.blocks           = std::move(blocks);
    endViewInfo_.blockUsage       = std::move(blockUsage);
}

void GeneratorOverlayWidget::showProgressView()
{
    setProgress(0.0);
    container_->setCurrentWidget(progressWidget_);
}

void GeneratorOverlayWidget::showEndView()
{
    refreshEndWidgetUi();
    container_->setCurrentWidget(endWidget_);
}

void GeneratorOverlayWidget::refreshText()
{
    generatingLabel_->setText(
        QStringLiteral("%1 %2%")
            .arg(EASYTR("Generating"))
            .arg(progressBar_->progress(), 5, 'f', 2, QLatin1Char('0'))
    );
    eportingLabel_->setText(EASYTR("Exporting..."));
    cancelButton_->setText(EASYTR("Cancel"));

    endWidgetTitleLabel_->setText(
        endViewInfo_.success ? EASYTR("Generation succeeded!") : EASYTR("Generation failed!"));
    confirmButton_->setText(EASYTR("Confirm"));
    openOutputFolderButton_->setText(EASYTR("Open Output Folder"));
    viewBlockUsageButton_->setText(EASYTR("View Block Usage"));
}

void GeneratorOverlayWidget::onOpenOutputFolderClicked()
{
    if (endViewInfo_.outputFilePath.isEmpty())
        return;
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(endViewInfo_.outputFilePath).absolutePath()));
}

void GeneratorOverlayWidget::onViewBlockUsageClicked()
{
    if (!blockUsageDialog_)
    {
        blockUsageDialog_ = new AdaptiveDialog(this);
        blockUsageWidget_ = new ExtendedBlockUsageWidget(blockUsageDialog_);
        auto* layout      = new QVBoxLayout(blockUsageDialog_);
        blockUsageWidget_->blockUsageWidget()->setSortMode(BlockUsageWidget::UsageDescendingSortMode);
        blockUsageWidget_->setMarkdownTitle(QFileInfo(endViewInfo_.inputFilePath).fileName());
        blockUsageWidget_->setMarkdownSummary(makeMarkdownSummary());
        layout->addWidget(blockUsageWidget_);
        blockUsageDialog_->resize(640, 640);
        blockUsageDialog_->setWindowTitle(EASYTR("Block Usage"));
    }

    auto* usageWidget = blockUsageWidget_->blockUsageWidget();
    usageWidget->setBlocks(endViewInfo_.blocks);
    usageWidget->setSurfaceDirection(endViewInfo_.surfaceDirection);
    usageWidget->setBlockUsage(endViewInfo_.blockUsage);

    blockUsageDialog_->exec();
}

QString GeneratorOverlayWidget::makeMarkdownSummary() const
{
    const auto inputFilePathSec = QStringLiteral("- %1: %2")
        .arg(EASYTR("Input File Path"))
        .arg(endViewInfo_.inputFilePath);
    const auto surfaceSec       = QStringLiteral("- %1: %2")
        .arg(EASYTR("Desired Surface"))
        .arg(getSurfaceDirectionText(endViewInfo_.surfaceDirection));
    const auto fallbackSec      = QStringLiteral("- %1: %2")
        .arg(EASYTR("Fallback Block"))
        .arg(
            endViewInfo_.fallbackBlock.first == std::string_view()
            ? EASYTR("None Block")
            : QString::fromStdString(std::string(endViewInfo_.fallbackBlock.first))
        );
    const auto frameSizeSec     = QStringLiteral("- %1: %2*%3")
        .arg(EASYTR("Frame Size"))
        .arg(endViewInfo_.frameSize.width()).arg(endViewInfo_.frameSize.height());
    const auto frameCountSec    = QStringLiteral("- %1: %2")
        .arg(EASYTR("Frame Count"))
        .arg(endViewInfo_.frameCount);

    const auto summary =
        endViewInfo_.frameCount == -1
        ? QStringLiteral("%1\n%2\n%3\n%4")
            .arg(inputFilePathSec)
            .arg(surfaceSec)
            .arg(fallbackSec)
            .arg(frameSizeSec)
        : QStringLiteral("%1\n%2\n%3\n%4\n%5")
            .arg(inputFilePathSec)
            .arg(surfaceSec)
            .arg(fallbackSec)
            .arg(frameSizeSec)
            .arg(frameCountSec);
    return summary;
}

void GeneratorOverlayWidget::refreshEndWidgetUi()
{
    endWidgetTitleLabel_->setText(
        endViewInfo_.success ? EASYTR("Generation succeeded!") : EASYTR("Generation failed!"));

    if (endViewInfo_.success)
    {
        errorMessageLabel_->setVisible(false);
        endExtraButtonWidget_->setVisible(true);
    }
    else
    {
        errorMessageLabel_->setText(endViewInfo_.errorMessage);
        errorMessageLabel_->setVisible(true);
        endExtraButtonWidget_->setVisible(false);
    }
}
