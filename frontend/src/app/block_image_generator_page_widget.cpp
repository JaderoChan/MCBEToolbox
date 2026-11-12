#include "block_image_generator_page_widget.h"

#include <stdint.h>

#include <qdir.h>
#include <qmetaobject.h>
#include <qthread.h>

#include <app_defines.h>
#include <utils/qstring_path.h>
#include "extended_block_list_widget.h"
#include "fallback_block_selector.h"
#include "generator_common_option_widget.h"

namespace
{

constexpr int   MAX_FRAME_COUNT_RANGE_MIN      = 1;
constexpr int   MAX_FRAME_COUNT_RANGE_MAX      = INT32_MAX;
constexpr int   NUM_THREADS_RANGE_MIN          = 1;
constexpr int   NUM_THREADS_RANGE_MAX          = 16;
constexpr QSize MAX_FRAME_SIZE_RANGE_MIN       = QSize(1,    1   );
constexpr QSize IMAGE_MAX_FRAME_SIZE_RANGE_MAX = QSize(4096, 4096);
constexpr QSize VIDEO_MAX_FRAME_SIZE_RANGE_MAX = QSize(256,  128 );

} // namespace

BlockImageGeneratorPageWidget::BlockImageGeneratorPageWidget(Mode mode, QWidget* parent)
    : GeneratorPageWidgetBase(mode, parent)
{
    // 配置硬编码值。
    commonOptionWidget_->setOutputFileExtension(mode_ == ImageMode ? QStringLiteral("png") : QStringLiteral("mp4"));
    commonOptionWidget_->setMaxFrameSizeRange(
        MAX_FRAME_SIZE_RANGE_MIN,
        mode_ == ImageMode ? IMAGE_MAX_FRAME_SIZE_RANGE_MAX : VIDEO_MAX_FRAME_SIZE_RANGE_MAX
    );
    commonOptionWidget_->setMaxFrameCountRange(MAX_FRAME_COUNT_RANGE_MIN, MAX_FRAME_COUNT_RANGE_MAX);
    commonOptionWidget_->setNumThreadsRange(NUM_THREADS_RANGE_MIN, NUM_THREADS_RANGE_MAX);
    commonOptionWidget_->setModeButtonVisible(FallbackBlockSelector::AirBlockMode, false);

    // 工厂实例。
    factory_ = new BlockImageFactory(BlockDataMap(), static_cast<SurfaceDirection>(commonConfigRef().desiredSurface));

    setupGeneratorUi();
}

BlockImageGeneratorPageWidget::~BlockImageGeneratorPageWidget()
{
    delete factory_;
}

AppConfiguration::CommonOptionConfig& BlockImageGeneratorPageWidget::commonConfigRef() const
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

void BlockImageGeneratorPageWidget::refreshFileFilters()
{
    if (mode_ == ImageMode)
    {
        commonOptionWidget_->setInputFileFilter(
            QStringLiteral("%1 (*.png *.jpg *.jpeg *.bmp *.webp *.tiff *.tif)")
            .arg(EASYTR("Images"))
        );
        commonOptionWidget_->setOutputFileFilter(QStringLiteral("%1 (*.png)").arg(EASYTR("PNG Image")));
    }
    else
    {
        commonOptionWidget_->setInputFileFilter(
            QStringLiteral("%1 (*.mp4 *.avi *.mov *.mkv *.webm *.flv *.wmv *.m4v *.mpg *.mpeg)")
            .arg(EASYTR("Videos"))
        );
        commonOptionWidget_->setOutputFileFilter(QStringLiteral("%1 (*.mp4)").arg(EASYTR("MP4 Video")));
    }
}

ImageOStream* BlockImageGeneratorPageWidget::createInputStream(const QString& inputFilePath) const
{
    if (mode_ == ImageMode)
        return new SingleImageOStream(toFsPath(inputFilePath));
    return new VideoImageOStream(toFsPath(inputFilePath), commonOptionWidget_->maxFrameCount());
}

void BlockImageGeneratorPageWidget::launchGenerateThread()
{
    const BlockDataMap& selectedBlocks = availableBlockWidget_->selecteds();
    const bool          blocksChanged  = !blocksApplied_ || selectedBlocks != appliedBlocks_;

    BlockImageGeneratorOption option{};
    option.needUpdateBlocks = blocksChanged;
    option.blocks           = selectedBlocks;
    option.desiredSurface   = commonOptionWidget_->surfaceDirection();
    option.fallbackBlock    = fallbackBlock_.second ? &fallbackBlock_ : nullptr;
    option.callback         = &GeneratorPageWidgetBase::progressCallback;
    option.userdata         = this;
    option.outFilePath      = toFsPath(commonOptionWidget_->outputFilePath());
    option.numThreads       = commonOptionWidget_->numThreads();
    option.generateVideo    = (mode_ == VideoMode);
    option.textureDirPath   = toFsPath(QDir(APP_RESOURCES_DIRPATH).absoluteFilePath(QStringLiteral(APP_BLOCK_TEXTURES_DIRPATH)));

    if (blocksChanged)
    {
        appliedBlocks_ = selectedBlocks;
        blocksApplied_ = true;
    }

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
