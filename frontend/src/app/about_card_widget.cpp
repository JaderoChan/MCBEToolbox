#include "about_card_widget.h"

#include <qapplication.h>
#include <qdesktopservices.h>
#include <qfont.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpalette.h>
#include <qtoolbutton.h>
#include <qurl.h>

#include <app_defines.h>
#include <components/stylesheet_table.h>
#include "app_logo_icon.h"

AboutCardWidget::AboutCardWidget(QWidget* parent)
    : AdaptiveWidget(parent)
{
    setObjectName(QStringLiteral("AboutCardWidget"));
    setAttribute(Qt::WA_StyledBackground, true);

    iconLabel_ = new QLabel(this);
    iconLabel_->setFixedSize(64, 64);
    iconLabel_->setScaledContents(true);
    iconLabel_->setPixmap(getAppLogoPixmap(128));

    nameLabel_ = new QLabel(this);
    auto nameFont = nameLabel_->font();
    nameFont.setBold(true);
    nameFont.setPointSize(nameFont.pointSize() + 3);
    nameLabel_->setFont(nameFont);

    versionLabel_ = new QLabel(this);
    authorLabel_  = new QLabel(this);

    githubButton_ = new QToolButton(this);
    githubButton_->setIconSize(QSize(20, 20));
    githubButton_->setCursor(Qt::PointingHandCursor);
    emailButton_  = new QToolButton(this);
    emailButton_->setIconSize(QSize(20, 20));
    emailButton_->setCursor(Qt::PointingHandCursor);

    auto* linkLayout = new QHBoxLayout();
    linkLayout->setContentsMargins(0, 0, 0, 0);
    linkLayout->addWidget(githubButton_);
    linkLayout->addWidget(emailButton_);
    linkLayout->addStretch(1);

    auto* textLayout = new QVBoxLayout();
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);
    textLayout->addWidget(nameLabel_);
    textLayout->addWidget(versionLabel_);
    textLayout->addWidget(authorLabel_);
    textLayout->addStretch(1);
    textLayout->addLayout(linkLayout);

    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(16);
    mainLayout->addWidget(iconLabel_, 0, Qt::AlignTop);
    mainLayout->addLayout(textLayout, 1);

    connect(githubButton_, &QToolButton::clicked, this, []()
    { QDesktopServices::openUrl(QUrl(QStringLiteral(APP_GITHUB_URL)));              });
    connect(emailButton_,  &QToolButton::clicked, this, []()
    { QDesktopServices::openUrl(QUrl(QStringLiteral("mailto:" APP_CONTACT_EMAIL))); });

    refreshText();
    refreshColor();
}

void AboutCardWidget::refreshText()
{
    nameLabel_->setText(EASYTR(APP_TITLE));
    versionLabel_->setText(QStringLiteral("%1 %2").arg(EASYTR("Version"), QStringLiteral(APP_VERSION)));
    authorLabel_->setText(QStringLiteral("%1 %2").arg(EASYTR("Author:"), QStringLiteral(APP_AUTHOR)));
    githubButton_->setToolTip(EASYTR("Open GitHub Repository"));
    emailButton_->setToolTip(EASYTR("Contact Author by Email"));
}

void AboutCardWidget::refreshColor()
{
    const auto base = qApp->palette().color(QPalette::Base);
    setStyleSheet(QStringLiteral(
        "#AboutCardWidget { background-color: rgb(%1, %2, %3); border: 1px solid rgba(127, 127, 127, 70); "
        "border-radius: 10px; }"
    ).arg(base.red()).arg(base.green()).arg(base.blue()));

    const bool dark = ColorSchemeManager::instance().loadedColorScheme() == ColorSchemeManager::COLOR_SCHEME_DARK;

    githubButton_->setStyleSheet(getNoFrameToolButtonStyleSheet());
    emailButton_->setStyleSheet(getNoFrameToolButtonStyleSheet());
    githubButton_->setIcon(QIcon(dark
        ? QStringLiteral(":/icons/app/github_light.svg")
        : QStringLiteral(":/icons/app/github_dark.svg")));
    emailButton_->setIcon(QIcon(dark
        ? QStringLiteral(":/icons/app/email_light.svg")
        : QStringLiteral(":/icons/app/email_dark.svg")));
}
