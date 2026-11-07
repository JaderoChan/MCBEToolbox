#include "output_file_selector.h"

#include <qbuttongroup.h>
#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qradiobutton.h>
#include <qtoolbutton.h>

OutputFileSelector::OutputFileSelector(QWidget* parent)
    : AdaptiveWidget(parent)
{
    toSourceDirectoryButton_ = new QRadioButton(this);
    toCustomDirectoryButton_ = new QRadioButton(this);
    toCustomFileButton_      = new QRadioButton(this);
    customFilePathInputer_   = new QLineEdit(this);
    customFilePathSelector_  = new QToolButton(this);

    auto* buttonGroup = new QButtonGroup(this);
    buttonGroup->setExclusive(true);
    buttonGroup->addButton(toSourceDirectoryButton_);
    buttonGroup->addButton(toCustomDirectoryButton_);
    buttonGroup->addButton(toCustomFileButton_);

    auto* buttonWidget = new QWidget(this);
    auto* buttonLayout = new QHBoxLayout(buttonWidget);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->addWidget(toSourceDirectoryButton_);
    buttonLayout->addWidget(toCustomDirectoryButton_);
    buttonLayout->addWidget(toCustomFileButton_);

    customFilePathWidget_ = new QWidget(this);
    auto* customFilePathLayout = new QHBoxLayout(customFilePathWidget_);
    customFilePathLayout->setContentsMargins(0, 0, 0, 0);
    customFilePathLayout->addWidget(customFilePathInputer_);
    customFilePathLayout->addWidget(customFilePathSelector_);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(buttonWidget);
    mainLayout->addWidget(customFilePathWidget_);

    connect(toSourceDirectoryButton_, &QRadioButton::clicked, this, [this]() { setMode(ToSourceDirectoryMode); });
    connect(toCustomDirectoryButton_, &QRadioButton::clicked, this, [this]() { setMode(ToCustomDirectoryMode); });
    connect(toCustomFileButton_,      &QRadioButton::clicked, this, [this]() { setMode(ToCustomFileMode);       });

    connect(
        customFilePathInputer_,  &QLineEdit::textChanged,
        this, &OutputFileSelector::onCustomFilePathTextChanged
    );
    connect(
        customFilePathSelector_, &QToolButton::clicked,
        this, &OutputFileSelector::onCustomFilePathSelectorClicked
    );

    refreshText();
    refreshUiState();
}

void OutputFileSelector::setMode(Mode mode)
{
    if (mode_ != mode)
    {
        mode_ = mode;
        refreshUiState();
    }
}

OutputFileSelector::Mode OutputFileSelector::mode() const
{
    return mode_;
}

void OutputFileSelector::setInputFilePath(QString filePath)
{
    inputFilePath_ = std::move(filePath);
}

void OutputFileSelector::setCustomOutputDirectoryPath(QString dirPath)
{
    if (customOutputDirectoryPath_ != dirPath)
    {
        customOutputDirectoryPath_ = std::move(dirPath);
        if (mode_ == ToCustomDirectoryMode)
        {
            const bool old = customFilePathInputer_->blockSignals(true);
            customFilePathInputer_->setText(customOutputDirectoryPath_);
            customFilePathInputer_->blockSignals(old);
        }
    }
}

void OutputFileSelector::setCustomOutputFilePath(QString filePath)
{
    if (customOutputFilePath_ != filePath)
    {
        customOutputFilePath_ = std::move(filePath);
        if (mode_ == ToCustomFileMode)
        {
            const bool old = customFilePathInputer_->blockSignals(true);
            customFilePathInputer_->setText(customOutputFilePath_);
            customFilePathInputer_->blockSignals(old);
        }
    }
}

const QString& OutputFileSelector::inputFilePath() const
{
    return inputFilePath_;
}

const QString& OutputFileSelector::customOutputDirectoryPath() const
{
    return customOutputDirectoryPath_;
}

const QString& OutputFileSelector::customOutputFilePath() const
{
    return customOutputFilePath_;
}

void OutputFileSelector::setOutputFileExtension(QString extension)
{
    outputFileExtension_ = std::move(extension);
}

const QString& OutputFileSelector::outputFileExtension() const
{
    return outputFileExtension_;
}

void OutputFileSelector::setOutputFileFilter(QString filter)
{
    outputFileFilter_ = std::move(filter);
}

const QString& OutputFileSelector::outputFileFilter() const
{
    return outputFileFilter_;
}

QString OutputFileSelector::outputFilePath(QString fileName, bool attachTimestamp) const
{
    if (mode_ == ToCustomFileMode)
    {
        return customOutputFilePath_;
    }
    else
    {
        if (inputFilePath_.isEmpty() || (mode_ == ToCustomDirectoryMode && customOutputDirectoryPath_.isEmpty()))
            return QString();

        const auto info       = QFileInfo(inputFilePath_);
        const auto parentPath = mode_ == ToSourceDirectoryMode ? info.path() : customOutputDirectoryPath_;
        const auto extra      =
            attachTimestamp
            ? QStringLiteral("-") + QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss"))
            : QString();

        fileName = fileName.isEmpty() ? info.fileName() : fileName;
        return parentPath + QStringLiteral("/") + fileName + extra + QStringLiteral(".") + outputFileExtension_;
    }
}

void OutputFileSelector::refreshText()
{
    toSourceDirectoryButton_->setText(EASYTR("Output to Input File Directory"));
    toCustomDirectoryButton_->setText(EASYTR("Output to Custom Directory"));
    toCustomFileButton_->setText(EASYTR("Output to Custom File"));
    if (mode_ == ToCustomDirectoryMode)
        customFilePathInputer_->setPlaceholderText(EASYTR("Please input the directory path"));
    else if (mode_ == ToCustomFileMode)
        customFilePathInputer_->setPlaceholderText(EASYTR("Please input the file path"));
    customFilePathSelector_->setText(QStringLiteral("..."));
}

void OutputFileSelector::refreshUiState()
{
    switch (mode_)
    {
        case ToSourceDirectoryMode:
        {
            toSourceDirectoryButton_->setChecked(true);
            customFilePathWidget_->setVisible(false);
            layout()->removeWidget(customFilePathWidget_);
            break;
        }
        case ToCustomDirectoryMode:
        {
            toCustomDirectoryButton_->setChecked(true);
            customFilePathWidget_->setVisible(true);
            layout()->addWidget(customFilePathWidget_);
            const bool old = customFilePathInputer_->blockSignals(true);
            customFilePathInputer_->setText(customOutputDirectoryPath_);
            customFilePathInputer_->setPlaceholderText(EASYTR("Please input the directory path"));
            customFilePathInputer_->blockSignals(old);
            break;
        }
        case ToCustomFileMode:
        {
            toCustomFileButton_->setChecked(true);
            customFilePathWidget_->setVisible(true);
            layout()->addWidget(customFilePathWidget_);
            const bool old = customFilePathInputer_->blockSignals(true);
            customFilePathInputer_->setText(customOutputFilePath_);
            customFilePathInputer_->setPlaceholderText(EASYTR("Please input the file path"));
            customFilePathInputer_->blockSignals(old);
            break;
        }
        default:
            break;
    }
}

void OutputFileSelector::onCustomFilePathTextChanged(const QString& text)
{
    switch (mode_)
    {
        case ToCustomDirectoryMode: customOutputDirectoryPath_ = text; break;
        case ToCustomFileMode:      customOutputFilePath_      = text; break;
        default: break;
    }
}

void OutputFileSelector::onCustomFilePathSelectorClicked()
{
    if (mode_ == ToCustomDirectoryMode)
    {
        const auto path = QFileDialog::getExistingDirectory(
            this,
            EASYTR("Select Output Directory"),
            customOutputDirectoryPath_
        );
        if (!path.isEmpty())
            setCustomOutputDirectoryPath(path);
    }
    else if (mode_ == ToCustomFileMode)
    {
        const auto path = QFileDialog::getSaveFileName(
            this,
            EASYTR("Select Output File"),
            customOutputFilePath_,
            QString::fromStdString(EASYTR(outputFileFilter_.toStdString()))
        );
        if (!path.isEmpty())
            setCustomOutputFilePath(path);
    }
}
