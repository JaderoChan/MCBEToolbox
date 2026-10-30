#pragma once

#include <qicon.h>
#include <qpixmap.h>

/** 获取程序的 Logo 图标。 */
const QIcon& getAppLogoIcon();

/** 获取程序的 Logo 图像。 */
QPixmap getAppLogoPixmap(int extent = 1024);
