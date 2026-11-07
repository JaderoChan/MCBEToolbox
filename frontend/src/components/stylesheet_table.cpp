#include "stylesheet_table.h"

#include <utils/file_io.h>
#include "color_scheme_manager.h"

const QString& getNoFrameToolButtonStyleSheet()
{
    static const auto dark  = readFileContent(QStringLiteral(":/stylesheets/noframe_toolbutton_dark.qss"));
    static const auto light = readFileContent(QStringLiteral(":/stylesheets/noframe_toolbutton_light.qss"));
    return
        ColorSchemeManager::instance().loadedColorScheme() == ColorSchemeManager::COLOR_SCHEME_DARK
        ? dark : light;
}
