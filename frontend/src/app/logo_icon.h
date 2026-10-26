#pragma once

#include <qicon.h>
#include <qpixmap.h>

/** 获取程序的 Logo 图标。 */
const QIcon& getLogoIcon();

/** 获取程序的 Logo 图像。 */
QPixmap getLogoPixmap(int extent = 1024);
