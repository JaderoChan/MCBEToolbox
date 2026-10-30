#pragma once

#include <qcolor.h>
#include <qpixmap.h>

#include <components/adaptive_widget.h>

/** 将图片分割成指定大小网格并依次点亮的风格化进度条。 */
class ImageGridProgressBar : public AdaptiveWidget
{
    Q_OBJECT

public:
    explicit ImageGridProgressBar(QWidget* parent = nullptr);

    void    setImage(const QPixmap& image);
    QPixmap image() const;

    /** \p rows 和 \p cols 都会被截断为至少为 1。 */
    void setGridSize(int rows, int cols);
    int  gridRows() const;
    int  gridCols() const;

    /** \p progress 取值范围 [0, 100]，超出范围将被截断。 */
    void   setProgress(double progress);
    double progress() const;

    /** 设置未点亮块的蒙版颜色，默认为透明。 */
    void   setUnlitColor(const QColor& color);
    QColor unlitColor() const;

    QSize sizeHint() const override;

signals:
    void progressChanged(double progress);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void updateToolTip();

    QPixmap image_;
    int     gridRows_   = 10;
    int     gridCols_   = 10;
    double  progress_   = 0.0;
    QColor  unlitColor_ = Qt::transparent;
};
