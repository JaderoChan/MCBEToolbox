#include "generator_overlay_widget.h"

#include <qevent.h>
#include <qfont.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qstackedwidget.h>
#include <qstyle.h>

#include <components/image_grid_progress_bar.h>

namespace
{

constexpr QColor PROGRESS_UNLIT_COLOR{0, 0, 0, 128};
constexpr int    PROGRESS_GRID_ROWS  = 8;
constexpr int    PROGRESS_GRID_COLS  = 8;

} // namespace

GeneratorOverlayWidget::GeneratorOverlayWidget(QWidget* parent)
    : AdaptiveWidget(parent)
{
    // 构建 Progress View。
    progressBar_ = new ImageGridProgressBar(this);
    progressBar_->setImage(QPixmap(":/icons/app/minecraft_icon.png"));
    progressBar_->setUnlitColor(PROGRESS_UNLIT_COLOR);
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

    cancelButton_        = new QPushButton(this);

    progressWidget_      = new QWidget(this);
    auto* progressLayout = new QVBoxLayout(progressWidget_);
    progressLayout->setContentsMargins(0, 0, 0, 0);
    progressLayout->addStretch();
    progressLayout->addWidget(progressBar_,  0, Qt::AlignCenter);
    progressLayout->addWidget(labelWidget_,  0, Qt::AlignCenter);
    progressLayout->addWidget(cancelButton_, 0, Qt::AlignCenter);
    progressLayout->addStretch();

    progressWidget_->setObjectName(QStringLiteral("progressWidget"));
    progressWidget_->setAttribute(Qt::WA_StyledBackground, true);
    progressWidget_->setStyleSheet(QStringLiteral("#progressWidget { background-color: rgba(0, 0, 0, 120); }"));

    // 构建 End View。
    endWidget_ = new QStackedWidget(this);
    endWidget_->setObjectName(QStringLiteral("endWidget"));
    endWidget_->setAttribute(Qt::WA_StyledBackground, true);
    endWidget_->setStyleSheet(QStringLiteral("#endWidget { background-color: rgba(0, 0, 0, 120); }"));

    container_ = new QStackedWidget(this);
    container_->setContentsMargins(0, 0, 0, 0);
    container_->addWidget(progressWidget_);
    container_->addWidget(endWidget_);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(container_);

    // Connections
    connect(cancelButton_, &QPushButton::clicked, this, [this]() { emit cancelRequested(); });

    refreshText();
}

void GeneratorOverlayWidget::setProgress(double progress)
{
    progressBar_->setProgress(progress);
    progressLabel_->setText(
        QStringLiteral("%1 %2%")
            .arg(EASYTR("Generating"))
            .arg(progress, 5, 'f', 2, QLatin1Char('0'))
    );
}

void GeneratorOverlayWidget::setEndViewInfo(
    bool success, const QString& failMessage,
    const QString& outputFilePath, const BaseFactory::BlockUsageMap& blockUsage)
{
    // TODO
}

void GeneratorOverlayWidget::showProgressView(ProgressStage stage)
{
    container_->setCurrentWidget(progressWidget_);
    if (stage == GeneratingStage)
    {
        labelWidget_->setCurrentWidget(progressLabel_);
        cancelButton_->setVisible(true);
    }
    else
    {
        labelWidget_->setCurrentWidget(loadingLabel_);
        cancelButton_->setVisible(false);
    }
}

void GeneratorOverlayWidget::showEndView()
{
    container_->setCurrentWidget(endWidget_);
}

void GeneratorOverlayWidget::refreshText()
{
    progressLabel_->setText(
        QStringLiteral("%1 %2%")
            .arg(EASYTR("Generating"))
            .arg(progressBar_->progress(), 5, 'f', 2, QLatin1Char('0'))
    );
    loadingLabel_->setText(EASYTR("Exporting"));
    cancelButton_->setText(EASYTR("Cancel"));
}
