#include "image_grid_progress_bar.h"

#include <qpainter.h>

ImageGridProgressBar::ImageGridProgressBar(QWidget* parent)
    : AdaptiveWidget(parent)
{
    updateToolTip();
}

void ImageGridProgressBar::setImage(const QPixmap& image)
{
    image_ = image;
    updateGeometry();
    update();
}

QPixmap ImageGridProgressBar::image() const
{
    return image_;
}

void ImageGridProgressBar::setGridSize(int rows, int cols)
{
    gridRows_ = qMax(1, rows);
    gridCols_ = qMax(1, cols);
    update();
}

int ImageGridProgressBar::gridRows() const
{
    return gridRows_;
}

int ImageGridProgressBar::gridCols() const
{
    return gridCols_;
}

void ImageGridProgressBar::setProgress(double progress)
{
    const double clamped = qBound(0.0, progress, 100.0);
    if (progress_ == clamped)
        return;

    progress_ = clamped;
    updateToolTip();
    update();
    emit progressChanged(progress_);
}

double ImageGridProgressBar::progress() const
{
    return progress_;
}

void ImageGridProgressBar::setUnlitColor(const QColor& color)
{
    unlitColor_ = color;
    update();
}

QColor ImageGridProgressBar::unlitColor() const
{
    return unlitColor_;
}

void ImageGridProgressBar::updateToolTip()
{
    setToolTip(QStringLiteral("%1%").arg(progress_, 5, 'f', 2, QLatin1Char('0')));
}

QSize ImageGridProgressBar::sizeHint() const
{
    return image_.isNull() ? QSize(240, 240) : image_.size();
}

void ImageGridProgressBar::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setPen(Qt::NoPen);

    const int    cellCount  = gridRows_ * gridCols_;
    const double litExact   = progress_ / 100.0 * cellCount;
    const int    litCount   = static_cast<int>(litExact);
    const double fractional = litExact - litCount;

    const qreal cellWidth     = qreal(width())  / gridCols_;
    const qreal cellHeight    = qreal(height()) / gridRows_;
    const qreal srcCellWidth  = image_.isNull() ? 0.0 : qreal(image_.width())  / gridCols_;
    const qreal srcCellHeight = image_.isNull() ? 0.0 : qreal(image_.height()) / gridRows_;

    for (int i = 0; i < cellCount; ++i)
    {
        const int rowFromBottom = i / gridCols_;
        const int col           = i % gridCols_;
        const int rowFromTop    = gridRows_ - 1 - rowFromBottom;

        const QRectF cellRect(col * cellWidth, rowFromTop * cellHeight, cellWidth, cellHeight);

        if (i > litCount || (i == litCount && fractional == 0.0))
        {
            if (unlitColor_.alpha() > 0)
                painter.fillRect(cellRect, unlitColor_);
            continue;
        }

        if (i == litCount && unlitColor_.alpha() > 0)
            painter.fillRect(cellRect, unlitColor_);

        if (!image_.isNull())
        {
            const QRectF srcRect(col * srcCellWidth, rowFromTop * srcCellHeight, srcCellWidth, srcCellHeight);
            if (i == litCount)
                painter.setOpacity(fractional);
            painter.drawPixmap(cellRect, image_, srcRect);
            painter.setOpacity(1.0);
        }
    }
}
