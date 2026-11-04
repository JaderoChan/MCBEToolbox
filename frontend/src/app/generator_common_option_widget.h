#pragma once

#include <components/adaptive_widget.h>
#include <components/output_file_selector.h>
#include "fallback_block_selector.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QSpinBox;
class QToolButton;

/** #BlockImageGenerator 和 #PixelArtGenerator 通用的选项配置界面。 */
class GeneratorCommonOptionWidget : public AdaptiveWidget
{
    Q_OBJECT

public:
    enum Mode
    {
        ImageMode,
        VideoMode
    };

    explicit GeneratorCommonOptionWidget(QWidget* parent = nullptr);

    void setMode(Mode mode);
    Mode mode() const;

    /** \p filter 为未翻译的原始文本，内部在 refreshText() 时将自动应用本地化文本。 */
    void setInputFileFilter(QString filter);
    const QString& inputFileFilter() const;

    /** \p filter 为未翻译的原始文本，内部在 refreshText() 时将自动应用本地化文本。 */
    void setOutputFileFilter(QString filter);
    const QString& outputFileFilter() const;

    void setMaxFrameSizeRange(const QSize& min, const QSize& max);
    void setMaxFrameCountRange(int min, int max);
    void setNumThreadsRange(int min, int max);

    void setSurfaceDirection(SurfaceDirection surfaceDirection);
    void setMaxFrameSize(const QSize& size);
    void setMaxFrameCount(int count);
    void setNumThreads(int num);

    QString          inputFilePath()    const;
    QString          outputFilePath()   const;
    SurfaceDirection surfaceDirection() const;
    BlockDataPair    fallbackBlock()    const;
    QSize            maxFrameSize()     const;
    int              maxFrameCount()    const;
    int              numThreads()       const;

protected:
    void refreshText() override;

private:
    friend class BlockImageGeneratorPageWidget;
    friend class PixelArtGeneratorPageWidget;

    void onInputFileSelectorClicked();

    void setInputFilePath(const QString& filePath);
    void refreshUiState();

    Mode    mode_ = VideoMode;
    QString inputFileFilter_;

    QLabel*                inputFileLabel_            = nullptr;
    QLineEdit*             inputFileInputer_          = nullptr;
    QToolButton*           inputFileSelector_         = nullptr;

    QLabel*                outputFileLabel_           = nullptr;
    OutputFileSelector*    outputFileSelector_        = nullptr;

    QLabel*                surfaceDirectionLabel_     = nullptr;
    QComboBox*             surfaceDirectionSelector_  = nullptr;

    QLabel*                fallbackBlockLabel_        = nullptr;
    FallbackBlockSelector* fallbackBlockSelector_     = nullptr;

    QLabel*                maxFrameSizeLabel_         = nullptr;
    QSpinBox*              maxFrameSizeWidthInputer_  = nullptr;
    QSpinBox*              maxFrameSizeHeightInputer_ = nullptr;

    QLabel*                maxFrameCountLabel_        = nullptr;
    QSpinBox*              maxFrameCountInputer_      = nullptr;

    QLabel*                numThreadsLabel_           = nullptr;
    QSpinBox*              numThreadsInputer_         = nullptr;
};
