#pragma once

#include <qevent.h>
#include <qmetatype.h>
#include <qobject.h>

class ColorSchemeManager : public QObject
{
    Q_OBJECT

public:
    enum ColorScheme : int
    {
        COLOR_SCHEME_AUTO,
        COLOR_SCHEME_LIGHT,
        COLOR_SCHEME_DARK
    };
    static constexpr ColorScheme COLOR_SCHEME_FIRST    = COLOR_SCHEME_AUTO;
    static constexpr ColorScheme COLOR_SCHEME_LAST     = COLOR_SCHEME_DARK;
    static constexpr ColorScheme FALLBACK_COLOR_SCHEME = COLOR_SCHEME_LIGHT;

    static ColorSchemeManager& getInstance();
    /** 获得系统颜色模式，如果是不支持的模式返回 #FALLBACK_COLOR_SCHEME。 */
    static ColorScheme         getSystemColorScheme();

    /** 设置程序颜色模式。 */
    void        setColorScheme(ColorScheme colorScheme);
    /** 获得程序颜色模式。 */
    ColorScheme getColorScheme()  const;
    /** 获得实际加载的颜色模式（不包含 #COLOR_SCHEME_AUTO）。 */
    ColorScheme getLoadedColorScheme() const;

signals:
    /** 程序颜色模式发生变化时发出。 */
    void colorSchemeChanged(ColorScheme);
    /** 仅当实际颜色模式发生变化时发出（不包含 #COLOR_SCHEME_AUTO）。 */
    void loadedColorSchemeChanged(ColorScheme);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    ColorSchemeManager();

    ColorSchemeManager(const ColorSchemeManager&)           = delete;
    ColorSchemeManager operator=(const ColorSchemeManager&) = delete;

    void loadColorScheme();

    ColorScheme colorScheme_;
    ColorScheme loadedColorScheme_;
};

Q_DECLARE_METATYPE(ColorSchemeManager::ColorScheme)
