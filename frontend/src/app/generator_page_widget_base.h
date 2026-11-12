#pragma once

#include <cstddef>
#include <atomic>

#include <mcbe_toolbox_api.hpp>
#include <components/adaptive_widget.h>
#include "app_configuration.h"

class QScrollArea;
class QStackedLayout;
class QThread;
class QToolButton;

class ExtendedBlockListWidget;
class GeneratorCommonOptionWidget;
class GeneratorOverlayWidget;

class GeneratorPageWidgetBase : public AdaptiveWidget
{
    Q_OBJECT

public:
    enum Mode
    {
        ImageMode,
        VideoMode
    };

    Mode mode() const;

    void startGenerate();
    void stopGenerate();

protected:
    explicit GeneratorPageWidgetBase(Mode mode, QWidget* parent = nullptr);
    ~GeneratorPageWidgetBase() override;

    /**
     * 初始化通用选项界面、回填通用配置、创建通用配置项的双向绑定等。
     *
     * 必须由派生类在自己的构造函数体中（设置好硬编码范围、创建好独有的成员之后）显式调用，
     * 不能放进本类的构造函数——此时本类的虚函数（如 #commonConfigRef()、#createExtraOptionWidget()）
     * 还无法正确指向派生类的重写版本。
     */
    void setupGeneratorUi();

    void refreshText() override;
    void resizeEvent(QResizeEvent* e) override;

    // =========================================================================
    // 派生类必须重写的函数！！！
    // =========================================================================

    /** 获取页面实际绑定的 AppConfiguration::CommonOptionConfig。 */
    virtual AppConfiguration::CommonOptionConfig& commonConfigRef() const = 0;
    /** 异步保存当前配置。 */
    virtual void syncConfig() const = 0;
    /** 设置输入/输出文件过滤器文本，由 #refreshText() 调用。 */
    virtual void refreshFileFilters() = 0;
    /** 根据当前输入文件路径创建对应的输入流。 */
    virtual ImageOStream* createInputStream(const QString& inputFilePath) const = 0;
    /** 启动后台生成线程。 */
    virtual void launchGenerateThread() = 0;

    // =========================================================================
    // 派生类可选重写的函数
    // =========================================================================

    /** 额外选项校验（用于校验 MCPack 选项），默认无额外校验。 */
    virtual bool     validateExtraOptions()              { return true; }
    /** #commonOptionWidget_ 与 #availableBlockWidget_ 之间的额外选项界面（用于 MCPack 选项）。 */
    virtual QWidget* createExtraOptionWidget()           { return nullptr; }
    /** Video 模式下每隔多少次进度回调才真正刷新一次界面。 */
    virtual int      videoProgressUpdateInterval() const { return 1; }

    static void progressCallback(std::size_t current, std::size_t total, bool& stop, void* userdata);

    void onGenerationFinished(FactoryResult result);
    void returnToOptionView();

    Mode mode_;

    GeneratorCommonOptionWidget* commonOptionWidget_   = nullptr;
    ExtendedBlockListWidget*     availableBlockWidget_ = nullptr;
    QScrollArea*                 optionWidget_         = nullptr;
    QToolButton*                 startButton_          = nullptr;
    GeneratorOverlayWidget*      overlayWidget_        = nullptr;
    QStackedLayout*              mainLayout_           = nullptr;

    QThread*      workThread_ = nullptr;
    ImageOStream* stream_     = nullptr;
    BlockDataPair fallbackBlock_{std::string_view(), nullptr};
    QSize         streamFrameSize_;
    int           streamFrameCount_;

    std::atomic<bool>        cancelRequested_{false};
    std::atomic<std::size_t> progressCounter_{0};
    int                      progressInterval_ = 1;

private:
    bool validateOptionAndOpenStream();
    void updateStartButtonGeometry();
};

#define GENERATOR_CONFIG_CONNECT(sender, signal, lambdaParams, assignExpr) \
    connect(sender, signal, this, [this] lambdaParams { assignExpr; syncConfig(); })
