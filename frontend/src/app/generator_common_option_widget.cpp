#include "generator_common_option_widget.h"

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qfiledialog.h>
#include <qgridlayout.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qspinbox.h>
#include <qtoolbutton.h>

#include <components/no_wheel_focus_filter.h>
#include "block_attribute_text.h"

GeneratorCommonOptionWidget::GeneratorCommonOptionWidget(QWidget* parent)
    : AdaptiveWidget(parent)
{
    // 构造 UI 元素。
    inputFileLabel_            = new QLabel(this);
    inputFileInputer_          = new QLineEdit(this);
    inputFileSelector_         = new QToolButton(this);
    auto* inputFileWidget      = new QWidget(this);
    auto* inputFileLayout      = new QHBoxLayout(inputFileWidget);
    inputFileLayout->setContentsMargins(0, 0, 0, 0);
    inputFileLayout->addWidget(inputFileInputer_,  1);
    inputFileLayout->addWidget(inputFileSelector_, 0);

    outputFileLabel_           = new QLabel(this);
    outputFileSelector_        = new OutputFileSelector(this);

    surfaceDirectionLabel_     = new QLabel(this);
    surfaceDirectionSelector_  = new QComboBox(this);
    QT_BAN_WHEEL_FOCUS(surfaceDirectionSelector_);

    fallbackBlockLabel_        = new QLabel(this);
    fallbackBlockSelector_     = new FallbackBlockSelector(this);

    maxFrameSizeLabel_         = new QLabel(this);
    maxFrameSizeWidthInputer_  = new QSpinBox(this);
    maxFrameSizeHeightInputer_ = new QSpinBox(this);
    auto* maxFrameSizeWidget   = new QWidget(this);
    auto* maxFrameSizeLayout   = new QHBoxLayout(maxFrameSizeWidget);
    maxFrameSizeLayout->setContentsMargins(0, 0, 0, 0);
    maxFrameSizeLayout->addWidget(maxFrameSizeWidthInputer_);
    maxFrameSizeLayout->addWidget(maxFrameSizeHeightInputer_);
    QT_BAN_WHEEL_FOCUS(maxFrameSizeWidthInputer_);
    QT_BAN_WHEEL_FOCUS(maxFrameSizeHeightInputer_);

    maxFrameCountLabel_        = new QLabel(this);
    maxFrameCountInputer_      = new QSpinBox(this);
    maxFrameCountInfCheck_     = new QCheckBox(this);
    maxFrameCountWidget_       = new QWidget(this);
    auto* maxFrameCountLayout  = new QHBoxLayout(maxFrameCountWidget_);
    maxFrameCountLayout->setContentsMargins(0, 0, 0, 0);
    maxFrameCountLayout->setSpacing(2);
    maxFrameCountLayout->addWidget(maxFrameCountInputer_,  1);
    maxFrameCountLayout->addWidget(maxFrameCountInfCheck_, 0);
    QT_BAN_WHEEL_FOCUS(maxFrameCountInputer_);

    numThreadsLabel_           = new QLabel(this);
    numThreadsInputer_         = new QSpinBox(this);
    QT_BAN_WHEEL_FOCUS(numThreadsInputer_);

    // 构造 Surface Direction ComboBox。
    surfaceDirectionSelector_->addItems({"", "", "", "", "", ""});
    surfaceDirectionSelector_->setItemData(0, static_cast<int>(SURFACE_DIRECTION_UP));
    surfaceDirectionSelector_->setItemData(1, static_cast<int>(SURFACE_DIRECTION_BOTTOM));
    surfaceDirectionSelector_->setItemData(2, static_cast<int>(SURFACE_DIRECTION_NORTH));
    surfaceDirectionSelector_->setItemData(3, static_cast<int>(SURFACE_DIRECTION_SOUTH));
    surfaceDirectionSelector_->setItemData(4, static_cast<int>(SURFACE_DIRECTION_EAST));
    surfaceDirectionSelector_->setItemData(5, static_cast<int>(SURFACE_DIRECTION_WEST));

    // 主布局
    auto* mainLayout = new QGridLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setHorizontalSpacing(12);
    mainLayout->setVerticalSpacing(12);
    mainLayout->setColumnMinimumWidth(0, 128);
    mainLayout->addWidget(inputFileLabel_,           0, 0);
    mainLayout->addWidget(inputFileWidget,           0, 1);
    mainLayout->addWidget(outputFileLabel_,          1, 0);
    mainLayout->addWidget(outputFileSelector_,       1, 1);
    mainLayout->addWidget(surfaceDirectionLabel_,    2, 0);
    mainLayout->addWidget(surfaceDirectionSelector_, 2, 1);
    mainLayout->addWidget(fallbackBlockLabel_,       3, 0);
    mainLayout->addWidget(fallbackBlockSelector_,    3, 1);
    mainLayout->addWidget(maxFrameSizeLabel_,        4, 0);
    mainLayout->addWidget(maxFrameSizeWidget,        4, 1);
    mainLayout->addWidget(maxFrameCountLabel_,       5, 0);
    mainLayout->addWidget(maxFrameCountWidget_,      5, 1);
    mainLayout->addWidget(numThreadsLabel_,          6, 0);
    mainLayout->addWidget(numThreadsInputer_,        6, 1);
    mainLayout->setColumnStretch(0, 0);
    mainLayout->setColumnStretch(1, 1);

    // Connections
    connect(
        inputFileInputer_,      &QLineEdit::textChanged,
        this,                   &GeneratorCommonOptionWidget::setInputFilePath
    );
    connect(
        inputFileSelector_,     &QToolButton::clicked,
        this,                   &GeneratorCommonOptionWidget::onInputFileSelectorClicked
    );
    connect(
        maxFrameCountInfCheck_, &QCheckBox::toggled,
        this,                   &GeneratorCommonOptionWidget::refreshUiState
    );

    connect(
        surfaceDirectionSelector_, &QComboBox::currentIndexChanged,
        this, [this]() { emit surfaceDirectionChanged(surfaceDirection()); }
    );
    connect(
        maxFrameSizeWidthInputer_, &QSpinBox::valueChanged,
        this, [this]() { emit maxFrameSizeChanged(maxFrameSize()); }
    );
    connect(
        maxFrameSizeHeightInputer_, &QSpinBox::valueChanged,
        this, [this]() { emit maxFrameSizeChanged(maxFrameSize()); }
    );
    connect(
        maxFrameCountInputer_, &QSpinBox::valueChanged,
        this, [this]() { emit maxFrameCountChanged(maxFrameCount()); }
    );
    connect(
        maxFrameCountInfCheck_, &QCheckBox::toggled,
        this, [this]() { emit maxFrameCountChanged(maxFrameCount()); }
    );
    connect(
        numThreadsInputer_, &QSpinBox::valueChanged,
        this, [this]() { emit numThreadsChanged(numThreads()); }
    );

    refreshText();
    refreshUiState();
}

void GeneratorCommonOptionWidget::setMode(Mode mode)
{
    if (mode_ != mode)
    {
        mode_ = mode;
        refreshUiState();
    }
}

GeneratorCommonOptionWidget::Mode GeneratorCommonOptionWidget::mode() const
{
    return mode_;
}

void GeneratorCommonOptionWidget::setInputFileFilter(QString filter)
{
    inputFileFilter_ = std::move(filter);
}

const QString& GeneratorCommonOptionWidget::inputFileFilter() const
{
    return inputFileFilter_;
}

void GeneratorCommonOptionWidget::setOutputFileFilter(QString filter)
{
    outputFileSelector_->setOutputFileFilter(filter);
}

const QString& GeneratorCommonOptionWidget::outputFileFilter() const
{
    return outputFileSelector_->outputFileFilter();
}

void GeneratorCommonOptionWidget::setOutputFileExtension(QString extension)
{
    outputFileSelector_->setOutputFileExtension(std::move(extension));
}

const QString& GeneratorCommonOptionWidget::outputFileExtension() const
{
    return outputFileSelector_->outputFileExtension();
}

void GeneratorCommonOptionWidget::setMaxFrameSizeRange(const QSize& min, const QSize& max)
{
    maxFrameSizeWidthInputer_->setRange(min.width(),   max.width());
    maxFrameSizeHeightInputer_->setRange(min.height(), max.height());
}

void GeneratorCommonOptionWidget::setMaxFrameCountRange(int min, int max)
{
    maxFrameCountInputer_->setRange(min, max);
}

void GeneratorCommonOptionWidget::setNumThreadsRange(int min, int max)
{
    numThreadsInputer_->setRange(min, max);
}

void GeneratorCommonOptionWidget::setModeButtonVisible(FallbackBlockSelector::Mode mode, bool visible)
{
    fallbackBlockSelector_->setModeButtonVisible(mode, visible);
}

void GeneratorCommonOptionWidget::setSurfaceDirection(SurfaceDirection surfaceDirection)
{
    for (int i = 0; i < surfaceDirectionSelector_->count(); ++i)
    {
        if (static_cast<SurfaceDirection>(surfaceDirectionSelector_->itemData(i).toInt()) == surfaceDirection)
        {
            surfaceDirectionSelector_->setCurrentIndex(i);
            break;
        }
    }
}

void GeneratorCommonOptionWidget::setMaxFrameSize(const QSize& size)
{
    if (maxFrameSize() != size)
    {
        const bool oldW = maxFrameSizeWidthInputer_->blockSignals(true);
        const bool oldH = maxFrameSizeHeightInputer_->blockSignals(true);
        maxFrameSizeWidthInputer_->setValue(size.width());
        maxFrameSizeHeightInputer_->setValue(size.height());
        maxFrameSizeWidthInputer_->blockSignals(oldW);
        maxFrameSizeHeightInputer_->blockSignals(oldH);
        emit maxFrameSizeChanged(size);
    }
}

void GeneratorCommonOptionWidget::setMaxFrameCount(int count)
{
    if (maxFrameCount() == count)
        return;

    if (count == -1)
    {
        const bool old = maxFrameCountInfCheck_->blockSignals(true);
        maxFrameCountInfCheck_->setChecked(true);
        maxFrameCountInfCheck_->blockSignals(old);
        refreshUiState();
    }
    else
    {
        const bool old = maxFrameCountInputer_->blockSignals(true);
        maxFrameCountInputer_->setValue(count);
        maxFrameCountInputer_->blockSignals(old);
    }

    emit maxFrameCountChanged(count);
}

void GeneratorCommonOptionWidget::setNumThreads(int num)
{
    if (numThreads() == num)
        return;

    const bool old = numThreadsInputer_->blockSignals(true);
    numThreadsInputer_->setValue(num);
    numThreadsInputer_->blockSignals(old);
    emit numThreadsChanged(num);
}

QString GeneratorCommonOptionWidget::inputFilePath() const
{
    return inputFileInputer_->text();
}

QString GeneratorCommonOptionWidget::outputFilePath(QString fileName, bool attachTimestamp) const
{
    return outputFileSelector_->outputFilePath(fileName, attachTimestamp);
}

SurfaceDirection GeneratorCommonOptionWidget::surfaceDirection() const
{
    return static_cast<SurfaceDirection>(surfaceDirectionSelector_->currentData().toInt());
}

BlockDataPair GeneratorCommonOptionWidget::fallbackBlock() const
{
    return fallbackBlockSelector_->fallbackBlock();
}

QSize GeneratorCommonOptionWidget::maxFrameSize() const
{
    return QSize(maxFrameSizeWidthInputer_->value(), maxFrameSizeHeightInputer_->value());
}

int GeneratorCommonOptionWidget::maxFrameCount() const
{
    if (maxFrameCountInfCheck_->isChecked())
        return -1;
    return maxFrameCountInputer_->value();
}

int GeneratorCommonOptionWidget::numThreads() const
{
    return numThreadsInputer_->value();
}

void GeneratorCommonOptionWidget::refreshText()
{
    inputFileLabel_->setText(EASYTR("Input File"));
    inputFileInputer_->setPlaceholderText(EASYTR("Please input the file path"));
    inputFileSelector_->setText(QStringLiteral("..."));
    outputFileLabel_->setText(EASYTR("Output File"));
    surfaceDirectionLabel_->setText(EASYTR("Desired Surface"));
    for (int i = 0; i < surfaceDirectionSelector_->count(); ++i)
    {
        auto surfaceDirection = static_cast<SurfaceDirection>(surfaceDirectionSelector_->itemData(i).toInt());
        surfaceDirectionSelector_->setItemText(i, getSurfaceDirectionText(surfaceDirection));
    }
    fallbackBlockLabel_->setText(EASYTR("Fallback Block"));
    fallbackBlockLabel_->setToolTip(EASYTR("FallbackBlock.Tooltip"));
    maxFrameSizeLabel_->setText(EASYTR("Max Frame Size"));
    maxFrameCountLabel_->setText(EASYTR("Max Frame Count"));
    maxFrameCountInfCheck_->setText(EASYTR("No Limit"));
    numThreadsLabel_->setText(EASYTR("Number of Threads"));
}

void GeneratorCommonOptionWidget::onInputFileSelectorClicked()
{
    const auto filePath = QFileDialog::getOpenFileName(
        this,
        EASYTR("Select Input File"),
        QString(),
        QString::fromStdString(EASYTR(inputFileFilter_.toStdString()))
    );
    if (!filePath.isEmpty())
        setInputFilePath(filePath);
}

void GeneratorCommonOptionWidget::setInputFilePath(const QString& filePath)
{
    const auto old = inputFileInputer_->blockSignals(true);
    inputFileInputer_->setText(filePath);
    inputFileInputer_->blockSignals(old);
    outputFileSelector_->setInputFilePath(filePath);
}

void GeneratorCommonOptionWidget::refreshUiState()
{
    auto* mainLayout = static_cast<QGridLayout*>(layout());
    if (mode_ == ImageMode)
    {
        maxFrameCountLabel_->setVisible(false);
        maxFrameCountWidget_->setVisible(false);
        numThreadsLabel_->setVisible(false);
        numThreadsInputer_->setVisible(false);

        mainLayout->removeWidget(maxFrameCountLabel_);
        mainLayout->removeWidget(maxFrameCountWidget_);
        mainLayout->removeWidget(numThreadsLabel_);
        mainLayout->removeWidget(numThreadsInputer_);
    }
    else if (mode_ == VideoMode)
    {
        maxFrameCountLabel_->setVisible(true);
        maxFrameCountWidget_->setVisible(true);
        numThreadsLabel_->setVisible(true);
        numThreadsInputer_->setVisible(true);

        mainLayout->addWidget(maxFrameCountLabel_,   5, 0);
        mainLayout->addWidget(maxFrameCountWidget_,  5, 1);
        mainLayout->addWidget(numThreadsLabel_,      6, 0);
        mainLayout->addWidget(numThreadsInputer_,    6, 1);
    }

    maxFrameCountInputer_->setDisabled(maxFrameCountInfCheck_->isChecked());
}
