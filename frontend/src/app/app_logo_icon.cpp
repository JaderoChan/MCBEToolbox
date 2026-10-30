#include "app_logo_icon.h"

static QIcon createAppLogoIcon()
{
    QIcon icon;

    icon.addFile(QStringLiteral(":/icons/app/app_16.png"));
    icon.addFile(QStringLiteral(":/icons/app/app_32.png"));
    icon.addFile(QStringLiteral(":/icons/app/app_64.png"));
    icon.addFile(QStringLiteral(":/icons/app/app_128.png"));
    icon.addFile(QStringLiteral(":/icons/app/app_256.png"));
    icon.addFile(QStringLiteral(":/icons/app/app_512.png"));
    icon.addFile(QStringLiteral(":/icons/app/app_1024.png"));

    return icon;
}

const QIcon& getAppLogoIcon()
{
    static const auto& icon = createAppLogoIcon();
    return icon;
}

QPixmap getAppLogoPixmap(int extent)
{
    return getAppLogoIcon().pixmap(extent);
}
