#include "mcpack_generator_option_widget.h"

#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qdatetime.h>
#include <qfileinfo.h>
#include <qgridlayout.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qvalidator.h>

#include <app_defines.h>
#include "block_attribute_text.h"

MCPackGeneratorOptionWidget::MCPackGeneratorOptionWidget(QWidget* parent)
    : AdaptiveWidget(parent)
{
    // 构造 UI 元素。
    packTypeLabel_                = new QLabel(this);
    packTypeStructureButton_      = new QRadioButton(this);
    packTypeFunctionButton_       = new QRadioButton(this);
    auto* packTypeWidget          = new QWidget(this);
    auto* packTypeLayout          = new QHBoxLayout(packTypeWidget);
    packTypeLayout->setContentsMargins(0, 0, 0, 0);
    packTypeLayout->addWidget(packTypeStructureButton_);
    packTypeLayout->addWidget(packTypeFunctionButton_);

    auto* packTypeButtonGroup     = new QButtonGroup(this);
    packTypeButtonGroup->setExclusive(true);
    packTypeButtonGroup->addButton(packTypeStructureButton_);
    packTypeButtonGroup->addButton(packTypeFunctionButton_);
    packTypeStructureButton_->setChecked(true);

    packNameLabel_                = new QLabel(this);
    packNameInputer_              = new QLineEdit(this);
    packNameAutoFillCheck_        = new QCheckBox(this);
    auto* packNameWidget          = new QWidget(this);
    auto* packNameLayout          = new QHBoxLayout(packNameWidget);
    packNameLayout->setContentsMargins(0, 0, 0, 0);
    packNameLayout->setSpacing(2);
    packNameLayout->addWidget(packNameInputer_,       1);
    packNameLayout->addWidget(packNameAutoFillCheck_, 0);

    packDescriptionLabel_         = new QLabel(this);
    packDescriptionInputer_       = new QLineEdit(this);
    packDescriptionAutoFillCheck_ = new QCheckBox(this);
    auto* packDescriptionWidget   = new QWidget(this);
    auto* packDescriptionLayout   = new QHBoxLayout(packDescriptionWidget);
    packDescriptionLayout->setContentsMargins(0, 0, 0, 0);
    packDescriptionLayout->setSpacing(2);
    packDescriptionLayout->addWidget(packDescriptionInputer_,       1);
    packDescriptionLayout->addWidget(packDescriptionAutoFillCheck_, 0);

    packNamespaceLabel_           = new QLabel(this);
    packNamespaceInputer_         = new QLineEdit(this);

    packVersionLabel_             = new QLabel(this);
    packVersionInputer_           = new QLineEdit(this);

    assistFunctionEnabledLabel_   = new QLabel(this);
    assistFunctionEnabledCheck_   = new QCheckBox(this);

    // 配置输入验证器。
    auto* packNameValidator       = new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("^[^\\\\/:*?\"<>|]+$")), this);
    packNameInputer_->setValidator(packNameValidator);
    auto* packNamespaceValidator  = new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("^[A-Za-z_][A-Za-z0-9_]*$")), this);
    packNamespaceInputer_->setValidator(packNamespaceValidator);
    auto* packVersionValidator    = new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("^\\d{1,2}\\.\\d{1,2}\\.\\d{1,2}$")), this);
    packVersionInputer_->setValidator(packVersionValidator);

    // 主布局
    auto* mainLayout = new QGridLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setHorizontalSpacing(12);
    mainLayout->setVerticalSpacing(12);
    mainLayout->setColumnMinimumWidth(0, 128);
    mainLayout->addWidget(packTypeLabel_,              0, 0);
    mainLayout->addWidget(packTypeWidget,              0, 1);
    mainLayout->addWidget(packNameLabel_,              1, 0);
    mainLayout->addWidget(packNameWidget,              1, 1);
    mainLayout->addWidget(packDescriptionLabel_,       2, 0);
    mainLayout->addWidget(packDescriptionWidget,       2, 1);
    mainLayout->addWidget(packNamespaceLabel_,         3, 0);
    mainLayout->addWidget(packNamespaceInputer_,       3, 1);
    mainLayout->addWidget(packVersionLabel_,           4, 0);
    mainLayout->addWidget(packVersionInputer_,         4, 1);
    mainLayout->addWidget(assistFunctionEnabledLabel_, 5, 0);
    mainLayout->addWidget(assistFunctionEnabledCheck_, 5, 1);
    mainLayout->setColumnStretch(0, 0);
    mainLayout->setColumnStretch(1, 1);

    // Connections
    connect(
        packTypeButtonGroup, &QButtonGroup::buttonToggled,
        this, [this]() { refreshAssistFunctionUiState(); emit packTypeChanged(packType()); }
    );
    connect(
        packNameInputer_, &QLineEdit::textChanged,
        this, [this](const QString& text) { emit packNameChanged(text); }
    );
    connect(
        packDescriptionInputer_, &QLineEdit::textChanged,
        this, [this](const QString& text) { emit packDescriptionChanged(text); }
    );
    connect(
        packNamespaceInputer_, &QLineEdit::textChanged,
        this, [this](const QString& text) { emit packNamespaceChanged(text); }
    );
    connect(
        packVersionInputer_, &QLineEdit::textChanged,
        this, [this]() { emit packVersionChanged(packVersion()); }
    );
    connect(
        packNameAutoFillCheck_, &QCheckBox::toggled,
        this, &MCPackGeneratorOptionWidget::setAutoFillPackName
    );
    connect(
        packDescriptionAutoFillCheck_, &QCheckBox::toggled,
        this, &MCPackGeneratorOptionWidget::setAutoFillPackDescription
    );
    connect(
        assistFunctionEnabledCheck_, &QCheckBox::toggled,
        this, [this](bool enabled) { emit assistFunctionEnabledToggled(enabled); }
    );

    refreshText();
    refreshAssistFunctionUiState();
}

void MCPackGeneratorOptionWidget::setProcessInfoForAutoFill(
    QString inputFilePath, GeneratorCommonOptionWidget::Mode mode, BlockDataPair fallbackBlock,
    SurfaceDirection surfaceDirection, QSize frameSize, int frameCount)
{
    processInfo_.inputFilePath    = std::move(inputFilePath);
    processInfo_.mode             = mode;
    processInfo_.fallbackBlock    = std::move(fallbackBlock);
    processInfo_.surfaceDirection = surfaceDirection;
    processInfo_.frameSize        = std::move(frameSize);
    processInfo_.frameCount       = std::move(frameCount);
}

void MCPackGeneratorOptionWidget::setPackType(PackType type)
{
    if (packType() == type)
        return;

    auto* button = (type == StructurePackType) ? packTypeStructureButton_ : packTypeFunctionButton_;
    const bool old = button->blockSignals(true);
    button->setChecked(true);
    button->blockSignals(old);
    refreshAssistFunctionUiState();
    emit packTypeChanged(type);
}

void MCPackGeneratorOptionWidget::setPackName(const QString& name)
{
    if (!isValidName(name) || packName() == name)
        return;
    const bool old = packNameInputer_->blockSignals(true);
    packNameInputer_->setText(name);
    packNameInputer_->blockSignals(old);
    packNameInputer_->setToolTip(name);
    emit packNameChanged(name);
}

void MCPackGeneratorOptionWidget::setPackDescription(const QString& description)
{
    if (packDescription() == description)
        return;
    const bool old = packDescriptionInputer_->blockSignals(true);
    packDescriptionInputer_->setText(description);
    packDescriptionInputer_->blockSignals(old);
    packDescriptionInputer_->setToolTip(description);
    emit packDescriptionChanged(description);
}

void MCPackGeneratorOptionWidget::setPackNamespace(const QString& nameSpace)
{
    if (!isValidNamespace(nameSpace) || packNamespace() == nameSpace)
        return;
    const bool old = packNamespaceInputer_->blockSignals(true);
    packNamespaceInputer_->setText(nameSpace);
    packNamespaceInputer_->blockSignals(old);
    packNamespaceInputer_->setToolTip(nameSpace);
    emit packNamespaceChanged(nameSpace);
}

void MCPackGeneratorOptionWidget::setPackVersion(const Version& version)
{
    if (packVersion() == version)
        return;
    const bool old = packVersionInputer_->blockSignals(true);
    packVersionInputer_->setText(QString::fromStdString(version.toString(false)));
    packVersionInputer_->blockSignals(old);
    emit packVersionChanged(version);
}

void MCPackGeneratorOptionWidget::setAssistFunctionEnabled(bool enabled)
{
    if (assistFunctionEnabledCheck_->isChecked() == enabled)
        return;
    const bool old = assistFunctionEnabledCheck_->blockSignals(true);
    assistFunctionEnabledCheck_->setChecked(enabled);
    assistFunctionEnabledCheck_->blockSignals(old);
    emit assistFunctionEnabledToggled(enabled);
}

void MCPackGeneratorOptionWidget::setAutoFillPackName(bool enabled)
{
    const bool old = packNameAutoFillCheck_->blockSignals(true);
    packNameAutoFillCheck_->setChecked(enabled);
    packNameAutoFillCheck_->blockSignals(old);
    packNameInputer_->setEnabled(!enabled);
    emit autoFillNameToggled(enabled);
}

void MCPackGeneratorOptionWidget::setAutoFillPackDescription(bool enabled)
{
    const bool old = packDescriptionAutoFillCheck_->blockSignals(true);
    packDescriptionAutoFillCheck_->setChecked(enabled);
    packDescriptionAutoFillCheck_->blockSignals(old);
    packDescriptionInputer_->setEnabled(!enabled);
    emit autoFillDescriptionToggled(enabled);
}

bool MCPackGeneratorOptionWidget::autoFillPackName() const
{
    return packNameAutoFillCheck_->isChecked();
}

bool MCPackGeneratorOptionWidget::autoFillPackDescription() const
{
    return packDescriptionAutoFillCheck_->isChecked();
}

MCPackGeneratorOptionWidget::PackType MCPackGeneratorOptionWidget::packType() const
{
    return packTypeStructureButton_->isChecked() ? StructurePackType : FunctionPackType;
}

QString MCPackGeneratorOptionWidget::packName() const
{
    if (autoFillPackName())
    {
        const auto name = QFileInfo(processInfo_.inputFilePath).fileName();
        if (!isValidName(name))
            return (packType() == StructurePackType ? EASYTR("A Structure Pack") : EASYTR("A Function Pack"));
        return name;
    }
    return packNameInputer_->text();
}

QString MCPackGeneratorOptionWidget::packDescription() const
{
    if (autoFillPackDescription())
    {
        const auto title               = QStringLiteral("%1 %2")
            .arg(EASYTR("By MCBE Toolbox"))
            .arg(APP_VERSION);
        const auto inputFilePathSec    = QStringLiteral("%1: '%2'")
            .arg(EASYTR("Input File Path"))
            .arg(processInfo_.inputFilePath);
        const auto modeSec             = QStringLiteral("%1: %2")
            .arg(EASYTR("Input File Type"))
            .arg(processInfo_.mode == GeneratorCommonOptionWidget::ImageMode
                ? EASYTR("Single Image Stream") : EASYTR("Multi Image Stream")
            );
        const auto surfaceDirectionSec = QStringLiteral("%1: %2")
            .arg(EASYTR("Desired Surface"))
            .arg(getSurfaceDirectionText(processInfo_.surfaceDirection));
        const auto fallbackSec         = QStringLiteral("%1: %2")
            .arg(EASYTR("Fallback Block"))
            .arg(
                processInfo_.fallbackBlock.first == std::string_view()
                ? EASYTR("None Block")
                : QString::fromStdString(std::string(processInfo_.fallbackBlock.first))
            );
        const auto frameSizeSec        = QStringLiteral("%1: %2*%3")
            .arg(EASYTR("Frame Size")).arg(processInfo_.frameSize.width()).arg(processInfo_.frameSize.height());

        auto description = QStringLiteral("%1! %2; %3; %4; %5; %6")
            .arg(title).arg(inputFilePathSec).arg(modeSec).arg(surfaceDirectionSec).arg(fallbackSec).arg(frameSizeSec);

        if (processInfo_.mode == GeneratorCommonOptionWidget::ImageMode)
        {
            const auto resultNameSec = QStringLiteral("%1: '%2'")
                .arg(packType() == StructurePackType ? EASYTR("Structure Name") : EASYTR("Function Name"))
                .arg(packNamespace() + QStringLiteral(":") + packName());
            description += QStringLiteral("; %1.").arg(resultNameSec);
        }
        else
        {
            const auto frameCountSec = QStringLiteral("%1: %2")
                .arg(EASYTR("Frame Count"))
                .arg(processInfo_.frameCount);
            const auto resultNameSec = QStringLiteral("%1: '%2'")
                .arg(packType() == StructurePackType ? EASYTR("Structure Name") : EASYTR("Function Name"))
                .arg(
                    packNamespace() + QStringLiteral(":") + packName() + QStringLiteral("-{frameIndex}") +
                    (packType() == FunctionPackType ? QStringLiteral("-{functionIndex}") : QString())
                );
            description += QStringLiteral("; %1; %2.").arg(frameCountSec).arg(resultNameSec);
        }

        return description;
    }
    return packDescriptionInputer_->text();;
}

QString MCPackGeneratorOptionWidget::packNamespace() const
{
    return packNamespaceInputer_->text();
}

Version MCPackGeneratorOptionWidget::packVersion() const
{
    try { return Version::fromString(packVersionInputer_->text().toStdString()); }
    catch (...) { return Version(); }
}

bool MCPackGeneratorOptionWidget::assistFunctionEnabled() const
{
    return assistFunctionEnabledCheck_->isChecked();
}

void MCPackGeneratorOptionWidget::refreshText()
{
    packTypeLabel_->setText(EASYTR("Pack Type"));
    packTypeStructureButton_->setText(EASYTR("Structure Pack"));
    packTypeFunctionButton_->setText(EASYTR("Function Pack"));

    packNameLabel_->setText(EASYTR("Pack Name"));
    packNameAutoFillCheck_->setText(EASYTR("Auto fill"));
    packNameInputer_->setPlaceholderText(EASYTR("Please input the pack name"));

    packDescriptionLabel_->setText(EASYTR("Pack Description"));
    packDescriptionAutoFillCheck_->setText(EASYTR("Auto fill"));
    packDescriptionInputer_->setPlaceholderText(EASYTR("Please input the pack description"));

    packNamespaceLabel_->setText(EASYTR("Pack Namespace"));
    packNamespaceInputer_->setPlaceholderText(EASYTR("Please input the pack namespace"));

    packVersionLabel_->setText(EASYTR("Pack Version"));
    packVersionInputer_->setPlaceholderText(EASYTR("Please input the pack version"));

    assistFunctionEnabledLabel_->setText(EASYTR("Enable Assist Function"));
    assistFunctionEnabledCheck_->setText(EASYTR("Generate Assist Function For Load Structure/Function"));

    if (packType() == FunctionPackType)
    {
        const QString tooltip = EASYTR("Assist function is not available in the Function Pack for the time being.");
        assistFunctionEnabledLabel_->setToolTip(tooltip);
        assistFunctionEnabledCheck_->setToolTip(tooltip);
    }
}

bool MCPackGeneratorOptionWidget::isValidName(const QString& name)
{
    static const QRegularExpression regex(QStringLiteral("^[^\\\\/:*?\"<>|]+$"));
    return regex.match(name).hasMatch();
}

bool MCPackGeneratorOptionWidget::isValidNamespace(const QString& nameSpace)
{
    static const QRegularExpression regex(QStringLiteral("^[A-Za-z_][A-Za-z0-9_]*$"));
    return regex.match(nameSpace).hasMatch();
}

void MCPackGeneratorOptionWidget::refreshAssistFunctionUiState()
{
    if (packType() == FunctionPackType)
    {
        const QString tooltip = EASYTR("Assist function is not available in the Function Pack for the time being.");
        assistFunctionEnabledLabel_->setToolTip(tooltip);
        assistFunctionEnabledCheck_->setToolTip(tooltip);
        assistFunctionEnabledLabel_->setDisabled(true);
        assistFunctionEnabledCheck_->setDisabled(true);
    }
    else
    {
        assistFunctionEnabledLabel_->setToolTip(QString());
        assistFunctionEnabledCheck_->setToolTip(QString());
        assistFunctionEnabledLabel_->setDisabled(false);
        assistFunctionEnabledCheck_->setDisabled(false);
    }
}
