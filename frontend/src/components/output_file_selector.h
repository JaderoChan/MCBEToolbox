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

    QString outputFilePath() const;

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
