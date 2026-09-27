#pragma once

#include <qlayout.h>
#include <qlayoutitem.h>
#include <qlist.h>

class FlowGridLayout : public QLayout
{
public:
    explicit FlowGridLayout(
        int margin      = 6,  int spacing     = 6,
        int minItemSize = 48, int maxItemSize = 72,
        QWidget* parent = nullptr);
    ~FlowGridLayout();

    int  minItemSize() const       { return minItemSize_;  }
    int  maxItemSize() const       { return maxItemSize_;  }
    void setMinItemSize(int width) { minItemSize_ = width; invalidate(); }
    void setMaxItemSize(int width) { maxItemSize_ = width; invalidate(); }

    void             invalidate()                         override;
    void             addItem(QLayoutItem* item)           override;
    int              count()                        const override;
    QLayoutItem*     itemAt(int index)              const override;
    QLayoutItem*     takeAt(int index)                    override;
    Qt::Orientations expandingDirections()          const override;
    bool             hasHeightForWidth()            const override;
    int              heightForWidth(int width)      const override;
    QSize            minimumSize()                  const override;
    QSize            sizeHint()                     const override;
    void             setGeometry(const QRect& rect)       override;

private:
    void updateCached(int width) const;

    QList<QLayoutItem*> items_;
    int minItemSize_;
    int maxItemSize_;

    mutable bool cacheDirty_     = true;
    mutable int  cachedItemSize_ = 0;
    mutable int  cachedRows_     = 0;
    mutable int  cachedCols_     = 0;
    mutable int  cachedWidth_    = 0;
};
