#pragma once

#include <components/color_scheme_manager.h>
#include <components/language_manager.h>

struct Settings
{
    ColorSchemeManager::ColorScheme colorScheme;
    LanguageManager::Language       language;
};

Settings loadSettings();

void saveSettings(const Settings& settings);
