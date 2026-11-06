#pragma once

#include <qstring.h>

#include <base_factory.hpp>
#include <block.hpp>
#include <components/adaptive_widget.h>

class QLabel;
class QPushButton;
class QStackedWidget;
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

    /** 重置为进度条视图并将进度置为 0，每次开始新任务前调用。 */
    void showProgressView();

    /**
     * 设置结束页面所需的信息并展示结束页面。
     *
     * @param success          指示生成是否成功，不成功将展示 errorMessage
     * @param errorMessage     错误信息
     * @param outputFilePath   输出文件路径
     * @param blocks           用于方块用量界面展示方块图标/颜色预览的方块集合
     * @param surfaceDirection 用于方块用量界面展示方块颜色预览的朝向
     * @param blockUsage       方块用量
     */
    void showEndView(
        bool success, const QString& errorMessage, const QString& outputFilePath,
        const BlockDataMap& blocks, SurfaceDirection surfaceDirection,
        const BaseFactory::BlockUsageMap& blockUsage);

signals:
    void cancelRequested();
    void confirmed();

protected:
    void refreshText() override;

private:
    void onOpenOutputFolderClicked();
    void onViewBlockUsageClicked();

    ImageGridProgressBar* progressBar_    = nullptr;
    QLabel*               progressLabel_  = nullptr;
    QLabel*               loadingLabel_   = nullptr;
    QStackedWidget*       labelWidget_    = nullptr;
    QPushButton*          cancelButton_   = nullptr;
    QWidget*              progressWidget_ = nullptr;

    QLabel*      errorMessageLabel_       = nullptr;
    QPushButton* openOutputFolderButton_  = nullptr;
    QPushButton* viewBlockUsageButton_    = nullptr;
    QPushButton* confirmButton_           = nullptr;
    QWidget*     endWidget_               = nullptr;

    QStackedWidget* container_ = nullptr;
    QWidget*        panel_     = nullptr;

    int progressUpdateInterval_ = 1;

    QString                    outputFilePath_;
    BlockDataMap               blocks_;
    SurfaceDirection           surfaceDirection_ = SURFACE_DIRECTION_UP;
    BaseFactory::BlockUsageMap blockUsage_;

    ExtendedBlockUsageWidget*  blockUsageWidget_ = nullptr;
    AdaptiveDialog*            blockUsageDialog_ = nullptr;
};
