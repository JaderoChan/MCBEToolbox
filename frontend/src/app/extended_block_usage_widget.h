#pragma once

#include <components/adaptive_widget.h>
#include "block_usage_widget.h"

class QComboBox;
class QLabel;
class QToolBox;

/** #BlockUsageWidget 的扩展控件，提供排序方式下拉框以及 Markdown 导出按钮。 */
class ExtendedBlockUsageWidget : public AdaptiveWidget
{
    Q_OBJECT

public:
    explicit ExtendedBlockUsageWidget(QWidget* parent = nullptr);

    void setMarkdownTitle(QString title);
    void setMarkdownSummary(QString summary);
    const QString& markdownTitle()   const;
    const QString& markdownSummary() const;

    BlockUsageWidget* blockUsageWidget() const;
    QString toMarkdown() const;

protected:
    void refreshText() override;

private:
    static QString sortModeText(BlockUsageWidget::SortMode mode);

    void onComboBoxIndexChanged(int index);
    void onExportButtonClicked();
    void onSortModeChanged(BlockUsageWidget::SortMode mode);

    QString markdownTitle_;
    QString markdownSummary_;

    QLabel*           comboBoxLabel_ = nullptr;
    QComboBox*        comboBox_      = nullptr;
    QToolButton*      exportButton_  = nullptr;
    BlockUsageWidget* widget_        = nullptr;
};
