#pragma once

#include <components/color_scheme_manager.h>
#include <components/language_manager.h>

/** 程序的全局设置结构体。 */
struct Settings
{
    ColorSchemeManager::ColorScheme colorScheme;
    LanguageManager::Language       language;
};

Settings loadSettings();

void saveSettings(const Settings& settings);
