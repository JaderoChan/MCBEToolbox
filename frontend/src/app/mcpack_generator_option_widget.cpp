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

MCPackGeneratorOptionWidget::MCPackGeneratorOptionWidget(QWidget* parent)
    : AdaptiveWidget(parent)
{
    // 构造 UI 元素。
    packTypeLabel_              = new QLabel(this);
    packTypeStructureButton_    = new QRadioButton(this);
    packTypeFunctionButton_     = new QRadioButton(this);
    auto* packTypeWidget        = new QWidget(this);
    auto* packTypeLayout        = new QHBoxLayout(packTypeWidget);
    packTypeLayout->setContentsMargins(0, 0, 0, 0);
    packTypeLayout->addWidget(packTypeStructureButton_);
    packTypeLayout->addWidget(packTypeFunctionButton_);

    auto* packTypeButtonGroup   = new QButtonGroup(this);
    packTypeButtonGroup->setExclusive(true);
    packTypeButtonGroup->addButton(packTypeStructureButton_);
    packTypeButtonGroup->addButton(packTypeFunctionButton_);
    packTypeStructureButton_->setChecked(true);

    packNameLabel_              = new QLabel(this);
    packNameInputer_            = new QLineEdit(this);
    packNameAutoFillBox_        = new QCheckBox(this);
    auto* packNameWidget        = new QWidget(this);
    auto* packNameLayout        = new QHBoxLayout(packNameWidget);
    packNameLayout->setContentsMargins(0, 0, 0, 0);
    packNameLayout->setSpacing(1);
    packNameLayout->addWidget(packNameInputer_);
    packNameLayout->addWidget(packNameAutoFillBox_);

    packDescriptionLabel_       = new QLabel(this);
    packDescriptionInputer_     = new QLineEdit(this);
    packDescriptionAutoFillBox_ = new QCheckBox(this);
    auto* packDescriptionWidget = new QWidget(this);
    auto* packDescriptionLayout = new QHBoxLayout(packDescriptionWidget);
    packDescriptionLayout->setContentsMargins(0, 0, 0, 0);
    packDescriptionLayout->setSpacing(1);
    packDescriptionLayout->addWidget(packDescriptionInputer_);
    packDescriptionLayout->addWidget(packDescriptionAutoFillBox_);

    packNamespaceLabel_         = new QLabel(this);
    packNamespaceInputer_       = new QLineEdit(this);

    packVersionLabel_           = new QLabel(this);
    packVersionInputer_         = new QLineEdit(this);

    assistFunctionEnabledLabel_ = new QLabel(this);
    assistFunctionEnabledBox_   = new QCheckBox(this);

    // 配置输入验证器。
    auto* packNameValidator      = new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("^[^\\\\/:*?\"<>|]+$")), this);
    packNameInputer_->setValidator(packNameValidator);
    auto* packNamespaceValidator = new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("^[A-Za-z_][A-Za-z0-9_]*$")), this);
    packNamespaceInputer_->setValidator(packNamespaceValidator);
    auto* packVersionValidator   = new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("^\\d{1,2}\\.\\d{1,2}\\.\\d{1,2}$")), this);
    packVersionInputer_->setValidator(packVersionValidator);

    // 主布局
    auto* mainLayout = new QGridLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setHorizontalSpacing(8);
    mainLayout->setVerticalSpacing(8);
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
    mainLayout->addWidget(assistFunctionEnabledBox_,   5, 1);
    mainLayout->setColumnStretch(0, 0);
    mainLayout->setColumnStretch(1, 1);

    // Connections
    connect(
        packNameAutoFillBox_,        &QCheckBox::toggled,
        this, &MCPackGeneratorOptionWidget::setAutoFillPackName
    );
    connect(
        packDescriptionAutoFillBox_, &QCheckBox::toggled,
        this, &MCPackGeneratorOptionWidget::setAutoFillPackDescription
    );

    refreshText();
}

void MCPackGeneratorOptionWidget::setProcessInfoAutoFill(
    QString inputFilePath, GeneratorCommonOptionWidget::Mode mode,
    SurfaceDirection surfaceDirection, QSize outputFrameSize, int outputFrameCount)
{
    processInfo_.inputFilePath    = std::move(inputFilePath);
    processInfo_.mode             = mode;
    processInfo_.surfaceDirection = surfaceDirection;
    processInfo_.outputFrameSize  = std::move(outputFrameSize);
    processInfo_.outputFrameCount = std::move(outputFrameCount);
}

void MCPackGeneratorOptionWidget::setPackType(PackType type)
{
    if (type == StructurePackType)     packTypeStructureButton_->setChecked(true);
    else if (type == FunctionPackType) packTypeFunctionButton_->setChecked(true);
}

void MCPackGeneratorOptionWidget::setPackName(const QString& name)
{
    if (!isValidName(name))
        return;
    packNameInputer_->setText(name);
    packNameInputer_->setToolTip(name);
}

void MCPackGeneratorOptionWidget::setPackDescription(const QString& description)
{
    packDescriptionInputer_->setText(description);
    packDescriptionInputer_->setToolTip(description);
}

void MCPackGeneratorOptionWidget::setPackNamespace(const QString& nameSpace)
{
    if (!isValidNamespace(nameSpace))
        return;
    packNamespaceInputer_->setText(nameSpace);
    packNamespaceInputer_->setToolTip(nameSpace);
}

void MCPackGeneratorOptionWidget::setPackVersion(const Version& version)
{
    packVersionInputer_->setText(QString::fromStdString(version.toString()));
}

void MCPackGeneratorOptionWidget::setAssistFunctionEnabled(bool enabled)
{
    assistFunctionEnabledBox_->setChecked(enabled);
}

void MCPackGeneratorOptionWidget::setAutoFillPackName(bool enabled)
{
    const bool old = packNameAutoFillBox_->blockSignals(true);
    packNameAutoFillBox_->setChecked(enabled);
    packNameAutoFillBox_->blockSignals(old);
    packNameInputer_->setEnabled(!enabled);
}

void MCPackGeneratorOptionWidget::setAutoFillPackDescription(bool enabled)
{
    const bool old = packDescriptionAutoFillBox_->blockSignals(true);
    packDescriptionAutoFillBox_->setChecked(enabled);
    packDescriptionAutoFillBox_->blockSignals(old);
    packDescriptionInputer_->setEnabled(!enabled);
}

bool MCPackGeneratorOptionWidget::autoFillPackName() const
{
    return packNameAutoFillBox_->isChecked();
}

bool MCPackGeneratorOptionWidget::autoFillPackDescription() const
{
    return packDescriptionAutoFillBox_->isChecked();
}

MCPackGeneratorOptionWidget::PackType MCPackGeneratorOptionWidget::packType() const
{
    return packTypeStructureButton_->isChecked() ? StructurePackType : FunctionPackType;
}

QString MCPackGeneratorOptionWidget::packName() const
{
    if (autoFillPackName())
    {
        auto name = QFileInfo(processInfo_.inputFilePath).baseName();
        if (!isValidName(name))
            name = (packType() == StructurePackType ? EASYTR("A Structure Pack") : EASYTR("A Function Pack"));
        return QStringLiteral("%1 %2")
            .arg(name)
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss")));
    }
    return packNameInputer_->text();
}

static QString surfaceDirectionText(SurfaceDirection surfaceDirection)
{
    switch (surfaceDirection)
    {
        case SURFACE_DIRECTION_UP:     return EASYTR("Up");
        case SURFACE_DIRECTION_BOTTOM: return EASYTR("Bottom");
        case SURFACE_DIRECTION_NORTH:  return EASYTR("North");
        case SURFACE_DIRECTION_SOUTH:  return EASYTR("South");
        case SURFACE_DIRECTION_EAST:   return EASYTR("East");
        case SURFACE_DIRECTION_WEST:   return EASYTR("West");
        default:                       return QString();
    }
}

QString MCPackGeneratorOptionWidget::packDescription() const
{
    if (autoFillPackDescription())
    {
        const auto title =
            QStringLiteral("%1 %2")
            .arg(EASYTR("By MCBE Toolbox "))
            .arg(APP_VERSION);

        const auto inputFilePathSec =
            QStringLiteral("%1: %2")
            .arg(EASYTR("Input File Path"))
            .arg(processInfo_.inputFilePath);

        const auto modeSec =
            QStringLiteral("%1: %2")
            .arg(EASYTR("Input File Type"))
            .arg(processInfo_.mode == GeneratorCommonOptionWidget::ImageMode
                ? EASYTR("Single Image Stream") : EASYTR("Multi Image Stream"));

        const auto surfaceDirectionSec =
            QStringLiteral("%1: %2")
            .arg(EASYTR("Desired Surface"))
            .arg(surfaceDirectionText(processInfo_.surfaceDirection));

        const auto outputFrameSizeSec =
            QStringLiteral("%1: %2*%3")
            .arg(EASYTR("Output Frame Size"))
            .arg(processInfo_.outputFrameSize.width())
            .arg(processInfo_.outputFrameSize.height());

        auto description =
            QStringLiteral("%1\n%2\n%3\n%4\n%5")
            .arg(title).arg(inputFilePathSec).arg(modeSec).arg(surfaceDirectionSec).arg(outputFrameSizeSec);

        if (processInfo_.mode == GeneratorCommonOptionWidget::VideoMode)
        {
            const auto outputFrameCountSec =
                QStringLiteral("%1: %2")
                .arg(EASYTR("Output Frame Count"))
                .arg(processInfo_.outputFrameCount);

            description += "\n" + outputFrameCountSec;
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
    return assistFunctionEnabledBox_->isChecked();
}

void MCPackGeneratorOptionWidget::refreshText()
{
    packTypeLabel_->setText(EASYTR("Pack Type"));
    packTypeStructureButton_->setText(EASYTR("Structure Pack"));
    packTypeFunctionButton_->setText(EASYTR("Function Pack"));

    packNameLabel_->setText(EASYTR("Pack Name"));
    packNameAutoFillBox_->setText(EASYTR("Auto fill"));
    packNameInputer_->setPlaceholderText(EASYTR("Please input the pack name"));

    packDescriptionLabel_->setText(EASYTR("Pack Description"));
    packDescriptionAutoFillBox_->setText(EASYTR("Auto fill"));
    packDescriptionInputer_->setPlaceholderText(EASYTR("Please input the pack description"));

    packNamespaceLabel_->setText(EASYTR("Pack Namespace"));
    packNamespaceInputer_->setPlaceholderText(EASYTR("Please input the pack namespace"));

    packVersionLabel_->setText(EASYTR("Pack Version"));
    packVersionInputer_->setPlaceholderText(EASYTR("Please input the pack version"));

    assistFunctionEnabledLabel_->setText(EASYTR("Enable Assist Function"));
    assistFunctionEnabledBox_->setText(EASYTR("Generate Assist Function For Load Structure/Function"));
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
