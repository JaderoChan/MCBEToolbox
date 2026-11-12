#pragma once

#include <qstring.h>

#include "adaptive_widget.h"

class QHBoxLayout;
class QLineEdit;
class QRadioButton;
class QToolButton;

class OutputFileSelector : public AdaptiveWidget
{
    Q_OBJECT

public:
    enum Mode
    {
        ToSourceDirectoryMode,
        ToCustomDirectoryMode,
        ToCustomFileMode,
    };

    explicit OutputFileSelector(QWidget* parent = nullptr);

    void setMode(Mode mode);
    Mode mode() const;

    void setInputFilePath(QString filePath);
    void setCustomOutputDirectoryPath(QString dirPath);
    void setCustomOutputFilePath(QString filePath);
    const QString& inputFilePath()             const;
    const QString& customOutputDirectoryPath() const;
    const QString& customOutputFilePath()      const;

    void setOutputFileExtension(QString extension);
    const QString& outputFileExtension() const;

    /** \p filter 为未翻译的原始文本，内部在 refreshText() 时将自动应用本地化文本。 */
    void setOutputFileFilter(QString filter);
    const QString& outputFileFilter() const;

    /**
     * @param fileName 指定输出文件的文件名（不含扩展名），如果为空字符串则采用“输入文件路径”的文件名。
     *                （仅 ToSourceDirectoryMode 和 ToCustomDirectoryMode 有效）
     * @param attachTimeStamp 指定文件名尾部后是否附带时间戳。
     *                       （仅 ToSourceDirectoryMode 和 ToCustomDirectoryMode 有效）
     * @return 如果关键参数未配置或用户未输入路径则返回空字符串。
     */
    QString outputFilePath(QString fileName = QString(), bool attachTimestamp = false) const;

signals:
    void modeChanged(Mode mode);
    void customOutputDirectoryChanged(QString dirPath);
    void customOutputFileChanged(QString filePath);

protected:
    void refreshText() override;

private:
    void refreshUiState();

    void onCustomFilePathTextChanged(const QString&);
    void onCustomFilePathSelectorClicked();

    Mode mode_ = ToSourceDirectoryMode;

    QRadioButton* toSourceDirectoryButton_ = nullptr;
    QRadioButton* toCustomDirectoryButton_ = nullptr;
    QRadioButton* toCustomFileButton_      = nullptr;
    QWidget*      customFilePathWidget_    = nullptr;
    QLineEdit*    customFilePathInputer_   = nullptr;
    QToolButton*  customFilePathSelector_  = nullptr;

    QString inputFilePath_;
    QString customOutputDirectoryPath_;
    QString customOutputFilePath_;
    QString outputFileExtension_;
    QString outputFileFilter_;
};
