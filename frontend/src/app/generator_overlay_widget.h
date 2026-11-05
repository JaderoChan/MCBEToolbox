#pragma once

#include <qstring.h>

#include <base_factory.hpp>
#include <components/adaptive_widget.h>

class QLabel;
class QPushButton;
class QStackedWidget;
class QToolButton;
class ImageGridProgressBar;

/** 生成器界面的覆盖层界面，主要用于展示生成中的进度条组件以及生成结束后的提示界面等。 */
class GeneratorOverlayWidget : public AdaptiveWidget
{
    Q_OBJECT

public:
    /** 进程阶段：影响进度条下方的文本和按钮显示。 */
    enum ProgressStage
    {
        GeneratingStage,
        ExportingStage
    };

    explicit GeneratorOverlayWidget(QWidget* parent = nullptr);

    void setProgress(double progress);

    /**
     * 设置 End 页面所需的信息。
     *
     * @param success 指示生成是否成功，如果不成功将展示 failMessage
     * @param failMessage 失败信息
     * @param outputFilePath 输出文件路径
     * @param blockUsage 方块用量
     */
    void setEndViewInfo(
        bool success, const QString& failMessage,
        const QString& outputFilePath, const BaseFactory::BlockUsageMap& blockUsage);

    void showProgressView(ProgressStage stage);
    void showEndView();

signals:
    void cancelRequested();

protected:
    void refreshText() override;

private:
    ImageGridProgressBar* progressBar_     = nullptr;
    QLabel*               progressLabel_   = nullptr;
    QLabel*               loadingLabel_    = nullptr;
    QStackedWidget*       labelWidget_     = nullptr;
    QPushButton*          cancelButton_    = nullptr;
    QWidget*              progressWidget_  = nullptr;

    QStackedWidget*       endWidget_       = nullptr;

    QStackedWidget*       container_       = nullptr;

    bool    success_ = false;
    QString failMessage_;
    QString outputFilePath_;
    BaseFactory::BlockUsageMap blockUsage_;
};
