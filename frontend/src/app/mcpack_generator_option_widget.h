#pragma once

#include <mcbe_toolbox_api.hpp>
#include <components/adaptive_widget.h>
#include "generator_common_option_widget.h"

class QCheckBox;
class QLabel;
class QLineEdit;
class QRadioButton;
class QSpibBox;

/** MC Pack Generator（Pixel Art Generator）使用的选项配置界面。 */
class MCPackGeneratorOptionWidget : public AdaptiveWidget
{
    Q_OBJECT

public:
    enum PackType
    {
        StructurePackType,
        FunctionPackType
    };

    explicit MCPackGeneratorOptionWidget(QWidget* parent = nullptr);

    /** 设置处理进程的信息用于自动生成包名与包描述。 */
    void setProcessInfoForAutoFill(
        QString inputFilePath, GeneratorCommonOptionWidget::Mode mode, BlockDataPair fallbackBlock,
        SurfaceDirection surfaceDirection, QSize frameSize, int frameCount = 0);

    void setPackType(PackType type);
    /** 如果输入的名称不合法则不做任何事。 */
    void setPackName(const QString& name);
    void setPackDescription(const QString& description);
    /** 如果输入的命名空间不合法则不做任何事。 */
    void setPackNamespace(const QString& nameSpace);
    void setPackVersion(const Version& version);
    void setAssistFunctionEnabled(bool enabled);

    void setAutoFillPackName(bool enabled);
    void setAutoFillPackDescription(bool enabled);
    bool autoFillPackName()          const;
    bool autoFillPackDescription()   const;

    PackType packType()              const;
    QString  packName()              const;
    QString  packDescription()       const;
    QString  packNamespace()         const;
    Version  packVersion()           const;
    bool     assistFunctionEnabled() const;

protected:
    void refreshText() override;

private:
    static bool isValidName(const QString& name);
    static bool isValidNamespace(const QString& nameSpace);

    struct ProcessInfo
    {
        QString                           inputFilePath;
        GeneratorCommonOptionWidget::Mode mode;
        BlockDataPair                     fallbackBlock;
        SurfaceDirection                  surfaceDirection;
        QSize                             frameSize;
        int                               frameCount = -1;
    } processInfo_{};

    QLabel*       packTypeLabel_                = nullptr;
    QRadioButton* packTypeStructureButton_      = nullptr;
    QRadioButton* packTypeFunctionButton_       = nullptr;

    QLabel*       packNameLabel_                = nullptr;
    QLineEdit*    packNameInputer_              = nullptr;
    QCheckBox*    packNameAutoFillCheck_        = nullptr;

    QLabel*       packDescriptionLabel_         = nullptr;
    QLineEdit*    packDescriptionInputer_       = nullptr;
    QCheckBox*    packDescriptionAutoFillCheck_ = nullptr;

    QLabel*       packNamespaceLabel_           = nullptr;
    QLineEdit*    packNamespaceInputer_         = nullptr;

    QLabel*       packVersionLabel_             = nullptr;
    QLineEdit*    packVersionInputer_           = nullptr;

    QLabel*       assistFunctionEnabledLabel_   = nullptr;
    QCheckBox*    assistFunctionEnabledCheck_   = nullptr;
};
