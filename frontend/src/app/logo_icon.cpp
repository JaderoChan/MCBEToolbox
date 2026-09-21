#include "logo_icon.h"

static QIcon createLogoIcon()
{
    QIcon icon;

    icon.addFile(":/icons/app/16.png");
    icon.addFile(":/icons/app/32.png");
    icon.addFile(":/icons/app/64.png");
    icon.addFile(":/icons/app/128.png");
    icon.addFile(":/icons/app/256.png");
    icon.addFile(":/icons/app/512.png");
    icon.addFile(":/icons/app/1024.png");

    return icon;
}

QIcon getLogoIcon()
{
    static QIcon icon = createLogoIcon();
    return icon;
}

QPixmap getLogoPixmap(int extent)
{
    return getLogoIcon().pixmap(extent);
}
