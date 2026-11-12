#pragma once

#include "generator_page_widget_base.h"

class BlockImageGeneratorPageWidget : public GeneratorPageWidgetBase
{
    Q_OBJECT

public:
    explicit BlockImageGeneratorPageWidget(Mode mode, QWidget* parent = nullptr);
    ~BlockImageGeneratorPageWidget() override;

protected:
    AppConfiguration::CommonOptionConfig& commonConfigRef() const override;
    void                                  syncConfig()      const override;

    void          refreshFileFilters()                                  override;
    ImageOStream* createInputStream(const QString& inputFilePath) const override;
    void          launchGenerateThread()                                override;

private:
    friend class BlockImageGeneratorWidget;

    BlockImageFactory* factory_ = nullptr;

    BlockDataMap appliedBlocks_;
    bool         blocksApplied_ = false;
};
