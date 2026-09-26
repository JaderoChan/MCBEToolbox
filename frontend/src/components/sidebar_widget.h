#pragma once

#include <qevent.h>
#include <qicon.h>
#include <qstring.h>
#include <qvariant.h>
#include <qvector.h>
#include <qwidget.h>

class QVBoxLayout;
class QVariantAnimation;
class SidebarEntryWidget;

class SidebarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SidebarWidget(QWidget* parent = nullptr);

    /** 添加条目至末尾，返回条目索引。 */
    int  addEntry(const QIcon& icon, const QString& text, const QVariant& data = QVariant());
    /** 插入条目至指定位置，返回条目索引。 */
    int  insertEntry(int index, const QIcon& icon, const QString& text, const QVariant& data = QVariant());
    /** 移除指定位置的条目，如果索引超出范围则不做任何事。 */
    void removeEntry(int index);
    /** 移除所有条目。 */
    void clearEntries();

    /** 获取条目数量。 */
    int  entryCount() const;

    QIcon    entryIcon(int index) const;
    QString  entryText(int index) const;
    QVariant entryData(int index) const;
    void     setEntryIcon(int index, const QIcon&    icon);
    void     setEntryText(int index, const QString&  text);
    void     setEntryData(int index, const QVariant& data);

    /** 获取条目之间的间距。 */
    int  entrySpacing() const;
    /** 设置条目之间的间距。 */
    void setEntrySpacing(int spacing);

    /** 添加固定高度的间距占位。 */
    void addSpacing(int size);
    /** 添加可伸缩的弹簧占位。 */
    void addStretch(int stretch = 0);

    /** 是否处于展开状态。 */
    bool isExpanded()  const;
    /** 是否处于折叠状态。 */
    bool isCollapsed() const;

    /** 展开/折叠按钮是否可用。 */
    bool isToggleVisible() const;
    /** 设置是否显示展开/折叠按钮。 */
    void setToggleVisible(bool visible);

    int  expandedWidth()  const;
    int  collapsedWidth() const;
    void setExpandedWidth(int width);
    void setCollapsedWidth(int width);

    /** 获取当前选中的条目索引，未选中时为 -1。 */
    int currentIndex() const;

public slots:
    void expand();
    void fold();
    void toggle();

    void setCurrentIndex(int index);

signals:
    void currentIndexChanged(int index);
    void expandedChanged(bool expanded);

protected:
    bool event(QEvent* e) override;

private:
    void updateToggleIcon();
    void animateWidthTo(int targetWidth);

    QVBoxLayout*       layout_ = nullptr;
    QVariantAnimation* anim_   = nullptr;

    SidebarEntryWidget*          toggleEntry_ = nullptr;
    QVector<SidebarEntryWidget*> entries_;

    bool expanded_       = true;
    bool toggleVisible_  = true;
    int  expandedWidth_  = 240;
    int  collapsedWidth_ = 48;
    int  entrySpacing_   = 4;
    int  currentIndex_   = -1;
};
