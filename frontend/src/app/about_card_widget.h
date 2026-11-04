#pragma once

#include <components/adaptive_widget.h>

class QLabel;
class QToolButton;

/** 展示程序图标、名称、版本、作者及 GitHub/邮箱链接的浮动卡片。 */
class AboutCardWidget : public AdaptiveWidget
{
    Q_OBJECT

public:
    explicit AboutCardWidget(QWidget* parent = nullptr);

protected:
    void refreshText()  override;
    void refreshColor() override;

private:
    QLabel*      iconLabel_    = nullptr;
    QLabel*      nameLabel_    = nullptr;
    QLabel*      versionLabel_ = nullptr;
    QLabel*      authorLabel_  = nullptr;
    QToolButton* githubButton_ = nullptr;
    QToolButton* emailButton_  = nullptr;
};
