#include "stylesheet_table.h"

#include <utils/file_io.h>
#include "color_scheme_manager.h"

const QString& toolButtonStyleSheet()
{
    static const auto& dark  = readFileContent(QStringLiteral(":/stylesheets/toolbutton_dark.qss"));
    static const auto& light = readFileContent(QStringLiteral(":/stylesheets/toolbutton_light.qss"));
    return
        ColorSchemeManager::getInstance().getLoadedColorScheme() == ColorSchemeManager::COLOR_SCHEME_DARK
        ? dark : light;
}
