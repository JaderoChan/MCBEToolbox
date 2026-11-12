#pragma once

#include "generator_page_widget_base.h"

class MCPackGeneratorOptionWidget;

class PixelArtGeneratorPageWidget : public GeneratorPageWidgetBase
{
    Q_OBJECT

public:
    explicit PixelArtGeneratorPageWidget(Mode mode, QWidget* parent = nullptr);
    ~PixelArtGeneratorPageWidget() override;

protected:
    AppConfiguration::CommonOptionConfig& commonConfigRef() const override;
    void                                  syncConfig()      const override;

    void          refreshFileFilters()                                  override;
    ImageOStream* createInputStream(const QString& inputFilePath) const override;
    void          launchGenerateThread()                                override;
    bool          validateExtraOptions()                                override;
    QWidget*      createExtraOptionWidget()                             override;
    int           videoProgressUpdateInterval()                   const override;

private:
    friend class PixelArtGeneratorWidget;

    AppConfiguration::PixelArtGeneratorConfig& configRef()       const;
    AppConfiguration::MCPackOptionConfig&      mcpackConfigRef() const;

    MCPackGeneratorOptionWidget* mcpackOptionWidget_ = nullptr;

    MCStructureFactory* structureFactory_ = nullptr;
    MCFunctionFactory*  functionFactory_  = nullptr;

    BlockDataMap lastAvailableBlocks_;
};
