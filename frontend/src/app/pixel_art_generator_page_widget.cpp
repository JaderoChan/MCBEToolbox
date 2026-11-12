#include "pixel_art_generator_page_widget.h"

#include <stdint.h>

#include <qmessagebox.h>
#include <qmetaobject.h>
#include <qthread.h>

#include <utils/clamp.h>
#include <utils/qstring_path.h>
#include "extended_block_list_widget.h"
#include "generator_common_option_widget.h"
#include "mcpack_generator_option_widget.h"

namespace
{

constexpr int   MAX_FRAME_COUNT_RANGE_MIN           = 1;
constexpr int   MAX_FRAME_COUNT_RANGE_MAX           = INT32_MAX;
constexpr int   NUM_THREADS_RANGE_MIN               = 1;
constexpr int   NUM_THREADS_RANGE_MAX               = 16;
constexpr QSize MAX_FRAME_SIZE_RANGE_MIN            = QSize(1,    1   );
constexpr QSize MAX_FRAME_SIZE_RANGE_MAX_FLOOR      = QSize(1024, 1024);
constexpr QSize MAX_FRAME_SIZE_RANGE_MAX_WALL       = QSize(720,  380 );
constexpr int   VIDEO_MODE_PROGRESS_UPDATE_INTERVAL = 4;

constexpr Version MCPACK_MIN_VERSION = Version(1, 21, 50);

} // namespace

PixelArtGeneratorPageWidget::PixelArtGeneratorPageWidget(Mode mode, QWidget* parent)
    : GeneratorPageWidgetBase(mode, parent)
{
    // 配置硬编码值。
    commonOptionWidget_->setOutputFileExtension(QStringLiteral("mcpack"));
    commonOptionWidget_->setMaxFrameSizeRange(MAX_FRAME_SIZE_RANGE_MIN, MAX_FRAME_SIZE_RANGE_MAX_WALL);
    commonOptionWidget_->setMaxFrameCountRange(MAX_FRAME_COUNT_RANGE_MIN, MAX_FRAME_COUNT_RANGE_MAX);
    commonOptionWidget_->setNumThreadsRange(NUM_THREADS_RANGE_MIN, NUM_THREADS_RANGE_MAX);

    // 工厂实例。
    const auto desiredSurface = static_cast<SurfaceDirection>(commonConfigRef().desiredSurface);
    structureFactory_ = new MCStructureFactory(BlockDataMap(), desiredSurface);
    functionFactory_  = new MCFunctionFactory(BlockDataMap(),  desiredSurface);

    setupGeneratorUi();

    // 配置范围限制：UP/BOTTOM 和 SIDE 所允许的范围不同。
    connect(
        commonOptionWidget_, &GeneratorCommonOptionWidget::surfaceDirectionChanged,
        this, [this](SurfaceDirection surfaceDirection)
        {
            if (surfaceDirection == SURFACE_DIRECTION_UP || surfaceDirection == SURFACE_DIRECTION_BOTTOM)
            {
                commonOptionWidget_->setMaxFrameSizeRange(MAX_FRAME_SIZE_RANGE_MIN, MAX_FRAME_SIZE_RANGE_MAX_FLOOR);
            }
            else
            {
                auto maxFrameSize = commonOptionWidget_->maxFrameSize();
                maxFrameSize = clampSize(maxFrameSize, MAX_FRAME_SIZE_RANGE_MIN, MAX_FRAME_SIZE_RANGE_MAX_WALL);
                commonOptionWidget_->setMaxFrameSize(maxFrameSize);
                commonOptionWidget_->setMaxFrameSizeRange(MAX_FRAME_SIZE_RANGE_MIN, MAX_FRAME_SIZE_RANGE_MAX_WALL);
            }
        }
    );
}

PixelArtGeneratorPageWidget::~PixelArtGeneratorPageWidget()
{
    delete structureFactory_;
    delete functionFactory_;
}

AppConfiguration::PixelArtGeneratorConfig& PixelArtGeneratorPageWidget::configRef() const
{
    return mode_ == ImageMode ? appConfig.pixelArtImageGroup : appConfig.pixelArtVideoGroup;
}

AppConfiguration::CommonOptionConfig& PixelArtGeneratorPageWidget::commonConfigRef() const
{
    return configRef().commonConfig;
}

AppConfiguration::MCPackOptionConfig& PixelArtGeneratorPageWidget::mcpackConfigRef() const
{
    return configRef().mcpackConfig;
}

void PixelArtGeneratorPageWidget::syncConfig() const
{
    appConfig.saveLater(
        mode_ == ImageMode
        ? AppConfiguration::PixelArtImageGroup
        : AppConfiguration::PixelArtVideoGroup
    );
}

void PixelArtGeneratorPageWidget::refreshFileFilters()
{
    if (mode_ == ImageMode)
    {
        commonOptionWidget_->setInputFileFilter(
            QStringLiteral("%1 (*.png *.jpg *.jpeg *.bmp *.webp *.tiff *.tif)")
            .arg(EASYTR("Images"))
        );
    }
    else
    {
        commonOptionWidget_->setInputFileFilter(
            QStringLiteral("%1 (*.mp4 *.avi *.mov *.mkv *.webm *.flv *.wmv *.m4v *.mpg *.mpeg);;%2 (*.gif)")
            .arg(EASYTR("Videos"))
            .arg(EASYTR("GIFs"))
        );
    }
    commonOptionWidget_->setOutputFileFilter(QStringLiteral("%1 (*.mcpack)").arg(EASYTR("MC Pack")));
}

ImageOStream* PixelArtGeneratorPageWidget::createInputStream(const QString& inputFilePath) const
{
    if (mode_ == ImageMode)
        return new SingleImageOStream(toFsPath(inputFilePath));
    if (inputFilePath.endsWith(".gif"))
        return new GifImageOStream(toFsPath(inputFilePath), commonOptionWidget_->maxFrameCount());
    return new VideoImageOStream(toFsPath(inputFilePath), commonOptionWidget_->maxFrameCount());
}

bool PixelArtGeneratorPageWidget::validateExtraOptions()
{
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

    return true;
}

QWidget* PixelArtGeneratorPageWidget::createExtraOptionWidget()
{
    mcpackOptionWidget_ = new MCPackGeneratorOptionWidget(this);
    mcpackOptionWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    const auto& config = mcpackConfigRef();
    mcpackOptionWidget_->setPackType(static_cast<MCPackGeneratorOptionWidget::PackType>(config.packType));
    mcpackOptionWidget_->setPackName(config.packName);
    mcpackOptionWidget_->setPackDescription(config.packDescription);
    mcpackOptionWidget_->setPackNamespace(config.packNamespace);
    mcpackOptionWidget_->setPackVersion(config.packVersion);
    mcpackOptionWidget_->setAutoFillPackName(config.autoFillPackName);
    mcpackOptionWidget_->setAutoFillPackDescription(config.autoFillPackDescription);
    mcpackOptionWidget_->setAssistFunctionEnabled(config.assistFunctionEnabled);

    GENERATOR_CONFIG_CONNECT(
        mcpackOptionWidget_, &MCPackGeneratorOptionWidget::packTypeChanged,
        (MCPackGeneratorOptionWidget::PackType type), mcpackConfigRef().packType = type
    );
    GENERATOR_CONFIG_CONNECT(
        mcpackOptionWidget_, &MCPackGeneratorOptionWidget::packNameChanged,
        (QString name), mcpackConfigRef().packName = name
    );
    GENERATOR_CONFIG_CONNECT(
        mcpackOptionWidget_, &MCPackGeneratorOptionWidget::packDescriptionChanged,
        (QString description), mcpackConfigRef().packDescription = description
    );
    GENERATOR_CONFIG_CONNECT(
        mcpackOptionWidget_, &MCPackGeneratorOptionWidget::packNamespaceChanged,
        (QString nameSpace), mcpackConfigRef().packNamespace = nameSpace
    );
    GENERATOR_CONFIG_CONNECT(
        mcpackOptionWidget_, &MCPackGeneratorOptionWidget::packVersionChanged,
        (Version version), mcpackConfigRef().packVersion = version
    );
    GENERATOR_CONFIG_CONNECT(
        mcpackOptionWidget_, &MCPackGeneratorOptionWidget::autoFillNameToggled,
        (bool enabled), mcpackConfigRef().autoFillPackName = enabled
    );
    GENERATOR_CONFIG_CONNECT(
        mcpackOptionWidget_, &MCPackGeneratorOptionWidget::autoFillDescriptionToggled,
        (bool enabled), mcpackConfigRef().autoFillPackDescription = enabled
    );
    GENERATOR_CONFIG_CONNECT(
        mcpackOptionWidget_, &MCPackGeneratorOptionWidget::assistFunctionEnabledToggled,
        (bool enabled), mcpackConfigRef().assistFunctionEnabled = enabled
    );

    return mcpackOptionWidget_;
}

int PixelArtGeneratorPageWidget::videoProgressUpdateInterval() const
{
    return VIDEO_MODE_PROGRESS_UPDATE_INTERVAL;
}

void PixelArtGeneratorPageWidget::launchGenerateThread()
{
    mcpackOptionWidget_->setProcessInfoForAutoFill(
        commonOptionWidget_->inputFilePath(),
        commonOptionWidget_->mode(),
        commonOptionWidget_->fallbackBlock(),
        commonOptionWidget_->surfaceDirection(),
        streamFrameSize_,
        streamFrameCount_
    );

    const bool blocksChanged = lastAvailableBlocks_ != availableBlockWidget_->selecteds();
    if (blocksChanged)
    {
        lastAvailableBlocks_ = availableBlockWidget_->selecteds();
        structureFactory_->setBlocks(lastAvailableBlocks_);
        functionFactory_->setBlocks(lastAvailableBlocks_);
    }

    MCPackGeneratorOption option{};
    option.desiredSurface            = commonOptionWidget_->surfaceDirection();
    option.fallbackBlock             = fallbackBlock_.second ? &fallbackBlock_ : nullptr;
    option.callback                  = &GeneratorPageWidgetBase::progressCallback;
    option.userdata                  = this;
    option.outFilePath               = toFsPath(commonOptionWidget_->outputFilePath(mcpackOptionWidget_->packName()));
    option.numThreads                = commonOptionWidget_->numThreads();
    option.assistFunctionEnabled     = mcpackOptionWidget_->assistFunctionEnabled();
    option.detachFileEnabled         = (mode_ == VideoMode);
    option.packOption.formatVersion  = 2;
    option.packOption.name           = toFsPath(mcpackOptionWidget_->packName());
    option.packOption.desciption     = mcpackOptionWidget_->packDescription().toStdString();
    option.packOption.nameSpace      = mcpackOptionWidget_->packNamespace().toStdString();
    option.packOption.version        = mcpackOptionWidget_->packVersion();
    option.packOption.minVersion     = MCPACK_MIN_VERSION;

    ImageOStream*       stream           = stream_;
    MCStructureFactory* structureFactory = structureFactory_;
    MCFunctionFactory*  functionFactory  = functionFactory_;

    const auto packType = mcpackOptionWidget_->packType();
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
