#pragma once

#include "color_scheme_manager.h"
#include "language_manager.h"

struct Settings
{
    ColorSchemeManager::ColorScheme colorScheme;
    LanguageManager::Language       language;
};

Settings loadSettings();

void saveSettings(const Settings& settings);
