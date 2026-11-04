#pragma once

#include <components/adaptive_widget.h>

class PixelArtGeneratorPageWidget;
class TabbedPageWidget;

class PixelArtGeneratorWidget : public AdaptiveWidget
{
    Q_OBJECT

public:
    explicit PixelArtGeneratorWidget(QWidget* parent = nullptr);

protected:
    void refreshText()  override;
    void refreshColor() override;

private:
    TabbedPageWidget*            container_ = nullptr;
    PixelArtGeneratorPageWidget* imagePage_ = nullptr;
    PixelArtGeneratorPageWidget* videoPage_ = nullptr;
};
