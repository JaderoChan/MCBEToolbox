#include "logo_icon.h"

static QIcon createLogoIcon()
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

QIcon logoIcon()
{
    static const auto& icon = createLogoIcon();
    return icon;
}

QPixmap logoPixmap(int extent)
{
    return logoIcon().pixmap(extent);
}
