#pragma once

#include <qstring.h>

#include <base_factory.hpp>
#include <block.hpp>
#include <components/adaptive_widget.h>

class QLabel;
class QPushButton;
class QStackedWidget;
class QVBoxLayout;
class ImageGridProgressBar;
class AdaptiveDialog;
class ExtendedBlockUsageWidget;

/** 生成器界面的覆盖层界面，主要用于展示生成中的进度条组件以及生成结束后的提示界面等。 */
class GeneratorOverlayWidget : public AdaptiveWidget
{
    Q_OBJECT

public:
    explicit GeneratorOverlayWidget(QWidget* parent = nullptr);

    /** 设置每多少次 #setProgress() 调用才真正刷新一次界面。 */
    void setProgressUpdateInterval(int interval);
    int  progressUpdateInterval() const;

    void setProgress(double progress);

    /**
     * 设置结束页面所需的信息。
     *
     * @param success          指示生成是否成功，不成功将展示 errorMessage
     * @param errorMessage     错误信息
     * @param inputFilePath    输入文件路径
     * @param outputFilePath   输出文件路径
     * @param fallbackBlockId  Fallback 游戏内方块 ID
     * @param frameSize        输入流限制后的帧尺寸
     * @param frameCount       输入流限制后的总帧数量，传入 -1 则不在 Markdown 中显示 frameCount 相关文
     * @param blocks           用于方块用量界面展示方块图标/颜色预览的方块集合
     * @param surfaceDirection 用于方块用量界面展示方块颜色预览的朝向
     * @param blockUsage       方块用量
     */
    void setEndViewInfo(
        bool                       success,
        QString                    errorMessage,
        QString                    inputFilePath,
        QString                    outputFilePath,
        BlockDataPair              fallbackBlock,
        QSize                      frameSize,
        int                        frameCount,
        SurfaceDirection           surfaceDirection,
        BlockDataMap               blocks,
        BaseFactory::BlockUsageMap blockUsage);

    /** 显示进度条页面并将进度置为 0，每次开始新任务前调用。 */
    void showProgressView();
    /** 显示结束页面。 */
    void showEndView();

signals:
    void cancelRequested();
    void confirmed();

protected:
    void refreshText() override;

private:
    void onOpenOutputFolderClicked();
    void onViewBlockUsageClicked();

    QString makeMarkdownSummary() const;
    void refreshEndWidgetUi();

    struct EndViewInfo
    {
        bool                       success          = false;
        QString                    errorMessage;
        QString                    inputFilePath;
        QString                    outputFilePath;
        BlockDataPair              fallbackBlock;
        QSize                      frameSize;
        int                        frameCount       = 0;
        SurfaceDirection           surfaceDirection;
        BlockDataMap               blocks;
        BaseFactory::BlockUsageMap blockUsage;
    } endViewInfo_{};

    ImageGridProgressBar* progressBar_            = nullptr;
    QLabel*               generatingLabel_        = nullptr;
    QLabel*               eportingLabel_          = nullptr;
    QStackedWidget*       labelWidget_            = nullptr;
    QPushButton*          cancelButton_           = nullptr;
    QWidget*              progressWidget_         = nullptr;

    QLabel*               endWidgetTitleLabel_    = nullptr;
    QLabel*               errorMessageLabel_      = nullptr;
    QPushButton*          confirmButton_          = nullptr;
    QPushButton*          openOutputFolderButton_ = nullptr;
    QPushButton*          viewBlockUsageButton_   = nullptr;
    QWidget*              endExtraButtonWidget_   = nullptr;
    QWidget*              endWidget_              = nullptr;
    QVBoxLayout*          endLayout_              = nullptr;

    QStackedWidget*       container_              = nullptr;
    QWidget*              panel_                  = nullptr;

    ExtendedBlockUsageWidget* blockUsageWidget_   = nullptr;
    AdaptiveDialog*           blockUsageDialog_   = nullptr;

    int progressUpdateInterval_ = 1;
};
