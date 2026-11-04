#pragma once

#include <components/adaptive_widget.h>

class BlockImageGeneratorPageWidget;
class TabbedPageWidget;

class BlockImageGeneratorWidget : public AdaptiveWidget
{
    Q_OBJECT

public:
    explicit BlockImageGeneratorWidget(QWidget* parent = nullptr);

protected:
    void refreshText()  override;
    void refreshColor() override;

private:
    TabbedPageWidget*              container_ = nullptr;
    BlockImageGeneratorPageWidget* imagePage_ = nullptr;
    BlockImageGeneratorPageWidget* videoPage_ = nullptr;
};
