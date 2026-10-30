#pragma once

#include <qmetatype.h>
#include <qobject.h>

/** 管理程序颜色模式的全局单例类，负责同步 qApp 调色板并在模式变化时发出信号。 */
class ColorSchemeManager : public QObject
{
    Q_OBJECT

public:
    enum ColorScheme : int
    {
        COLOR_SCHEME_AUTO,
        COLOR_SCHEME_DARK,
        COLOR_SCHEME_LIGHT
    };
    static constexpr ColorScheme COLOR_SCHEME_FIRST    = COLOR_SCHEME_AUTO;
    static constexpr ColorScheme COLOR_SCHEME_LAST     = COLOR_SCHEME_LIGHT;
    static constexpr ColorScheme FALLBACK_COLOR_SCHEME = COLOR_SCHEME_DARK;

    static ColorSchemeManager& instance();
    /** 获取系统颜色模式；如果是不支持的模式返回 #FALLBACK_COLOR_SCHEME。 */
    static ColorScheme         systemColorScheme();

    /** 设置程序颜色模式。 */
    void        setColorScheme(ColorScheme colorScheme);
    /** 获取程序颜色模式。 */
    ColorScheme colorScheme()       const;
    /** 获取实际加载的颜色模式（不包含 #COLOR_SCHEME_AUTO）。 */
    ColorScheme loadedColorScheme() const;

signals:
    /** 程序颜色模式发生变化时发出。 */
    void colorSchemeChanged(ColorScheme);
    /** 仅当实际颜色模式发生变化时发出（不包含 #COLOR_SCHEME_AUTO）。 */
    void loadedColorSchemeChanged(ColorScheme);

private:
    ColorSchemeManager();

    ColorSchemeManager(const ColorSchemeManager&)           = delete;
    ColorSchemeManager operator=(const ColorSchemeManager&) = delete;

    void loadColorScheme();

    ColorScheme colorScheme_;
    ColorScheme loadedColorScheme_;
};

Q_DECLARE_METATYPE(ColorSchemeManager::ColorScheme)
