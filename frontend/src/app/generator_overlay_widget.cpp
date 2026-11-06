#include "generator_overlay_widget.h"

#include <qdesktopservices.h>
#include <qdir.h>
#include <qevent.h>
#include <qfileinfo.h>
#include <qfont.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qprocess.h>
#include <qpushbutton.h>
#include <qstackedwidget.h>
#include <qstyle.h>
#include <qurl.h>

#include <components/image_grid_progress_bar.h>
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

    progressLabel_ = new QLabel(this);
    loadingLabel_  = new QLabel(this);
    for (auto& label : {progressLabel_, loadingLabel_})
    {
        auto font = label->font();
        font.setPointSizeF(font.pointSizeF() * 0.9);
        label->setFont(font);
    }

    labelWidget_ = new QStackedWidget(this);
    labelWidget_->setContentsMargins(0, 0, 0, 0);
    labelWidget_->addWidget(progressLabel_);
    labelWidget_->addWidget(loadingLabel_);

    cancelButton_ = new QPushButton(this);
    connect(cancelButton_, &QPushButton::clicked, this, [this]() { emit cancelRequested(); });

    progressWidget_      = new QWidget(this);
    auto* progressLayout = new QVBoxLayout(progressWidget_);
    progressLayout->setContentsMargins(0, 0, 0, 0);
    progressLayout->addWidget(progressBar_,  0, Qt::AlignCenter);
    progressLayout->addWidget(labelWidget_,  0, Qt::AlignCenter);
    progressLayout->addWidget(cancelButton_, 0, Qt::AlignCenter);

    // 构建 End View。
    errorMessageLabel_      = new QLabel(this);
    errorMessageLabel_->setWordWrap(true);
    errorMessageLabel_->setAlignment(Qt::AlignCenter);
    openOutputFolderButton_ = new QPushButton(this);
    viewBlockUsageButton_   = new QPushButton(this);
    confirmButton_          = new QPushButton(this);
    connect(openOutputFolderButton_, &QPushButton::clicked, this, &GeneratorOverlayWidget::onOpenOutputFolderClicked);
    connect(viewBlockUsageButton_,   &QPushButton::clicked, this, &GeneratorOverlayWidget::onViewBlockUsageClicked);
    connect(confirmButton_,          &QPushButton::clicked, this, [this]() { emit confirmed(); });

    confirmButton_->setMinimumSize(200, 50);
    auto confirmFont = confirmButton_->font();
    confirmFont.setBold(true);
    confirmFont.setPointSizeF(confirmFont.pointSizeF() * 1.15);
    confirmButton_->setFont(confirmFont);

    auto* endButtonWidget = new QWidget(this);
    auto* endButtonLayout = new QHBoxLayout(endButtonWidget);
    endButtonLayout->setContentsMargins(0, 0, 0, 0);
    endButtonLayout->addWidget(openOutputFolderButton_);
    endButtonLayout->addWidget(viewBlockUsageButton_);

    endWidget_           = new QWidget(this);
    auto* endLayout      = new QVBoxLayout(endWidget_);
    endLayout->setContentsMargins(0, 0, 0, 0);
    endLayout->addWidget(errorMessageLabel_, 0, Qt::AlignCenter);
    endLayout->addWidget(confirmButton_,     0, Qt::AlignCenter);
    endLayout->addWidget(endButtonWidget,    0, Qt::AlignCenter);

    container_ = new QStackedWidget(this);
    container_->setContentsMargins(0, 0, 0, 0);
    container_->addWidget(progressWidget_);
    container_->addWidget(endWidget_);

    panel_ = new QWidget(this);
    panel_->setObjectName(QStringLiteral("generatorOverlayPanel"));
    panel_->setAttribute(Qt::WA_StyledBackground, true);
    panel_->setStyleSheet(
        QStringLiteral("#generatorOverlayPanel { background-color: rgba(20, 20, 20, 220); border-radius: 12px; }"));
    auto* panelLayout = new QVBoxLayout(panel_);
    panelLayout->setContentsMargins(32, 32, 32, 32);
    panelLayout->addWidget(container_);

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

void GeneratorOverlayWidget::showProgressView()
{
    container_->setCurrentWidget(progressWidget_);
    setProgress(0.0);
}

void GeneratorOverlayWidget::setProgress(double progress)
{
    progressBar_->setProgress(progress);
    const bool exporting = progress >= 100.0;
    if (exporting)
    {
        labelWidget_->setCurrentWidget(loadingLabel_);
        cancelButton_->setVisible(false);
    }
    else
    {
        labelWidget_->setCurrentWidget(progressLabel_);
        cancelButton_->setVisible(true);
        progressLabel_->setText(
            QStringLiteral("%1 %2%")
                .arg(EASYTR("Generating"))
                .arg(progress, 5, 'f', 2, QLatin1Char('0'))
        );
    }
}

void GeneratorOverlayWidget::showEndView(
    bool success, const QString& errorMessage, const QString& outputFilePath,
    const BlockDataMap& blocks, SurfaceDirection surfaceDirection,
    const BaseFactory::BlockUsageMap& blockUsage)
{
    outputFilePath_   = outputFilePath;
    blocks_           = blocks;
    surfaceDirection_ = surfaceDirection;
    blockUsage_       = blockUsage;

    errorMessageLabel_->setText(
        success
        ? EASYTR("Generation succeeded!")
        : QStringLiteral("%1\n%2")
            .arg(EASYTR("Generation failed!"))
            .arg(errorMessage)
    );
    openOutputFolderButton_->setVisible(success);
    viewBlockUsageButton_->setVisible(success);

    container_->setCurrentWidget(endWidget_);
}

void GeneratorOverlayWidget::onOpenOutputFolderClicked()
{
    if (outputFilePath_.isEmpty())
        return;

#ifdef _WIN32
    QProcess::startDetached(
        QStringLiteral("explorer.exe"),
        {QStringLiteral("/select,"), QDir::toNativeSeparators(outputFilePath_)}
    );
#else
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(outputFilePath_).absolutePath()));
#endif // _WIN32
}

void GeneratorOverlayWidget::onViewBlockUsageClicked()
{
    if (!blockUsageDialog_)
    {
        blockUsageDialog_ = new AdaptiveDialog(this);
        blockUsageWidget_ = new ExtendedBlockUsageWidget(blockUsageDialog_);
        auto* layout      = new QVBoxLayout(blockUsageDialog_);
        blockUsageWidget_->blockUsageWidget()->setSortMode(BlockUsageWidget::UsageDescendingSortMode);
        blockUsageWidget_->setMarkdownTitle(QFileInfo(outputFilePath_).completeBaseName());
        // TODO: Markdown Summary 必须包含输入文件路径，期望方块面，fallback block、帧的输出尺寸、帧的实际输出数量等信息。
        // blockUsageWidget_->setMarkdownSummary();
        layout->addWidget(blockUsageWidget_);
        blockUsageDialog_->resize(640, 640);
        blockUsageDialog_->setWindowTitle(EASYTR("Block Usage"));
    }

    auto* usageWidget = blockUsageWidget_->blockUsageWidget();
    usageWidget->setBlocks(blocks_);
    usageWidget->setSurfaceDirection(surfaceDirection_);
    usageWidget->setBlockUsage(blockUsage_);

    blockUsageDialog_->exec();
}

void GeneratorOverlayWidget::refreshText()
{
    progressLabel_->setText(
        QStringLiteral("%1 %2%")
            .arg(EASYTR("Generating"))
            .arg(progressBar_->progress(), 5, 'f', 2, QLatin1Char('0'))
    );
    loadingLabel_->setText(EASYTR("Exporting..."));
    cancelButton_->setText(EASYTR("Cancel"));

    openOutputFolderButton_->setText(EASYTR("Open Output Folder"));
    viewBlockUsageButton_->setText(EASYTR("View Block Usage"));
    confirmButton_->setText(EASYTR("Confirm"));
}
