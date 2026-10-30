#pragma once

#include <components/color_scheme_manager.h>
#include <components/language_manager.h>

#include <block.hpp>

/** 程序的设置/配置结构体。 */
struct ConfigurationData
{
    ColorSchemeManager::ColorScheme colorScheme;
    LanguageManager::Language       language;

    BlockEntryMap blockEntries;
};

ConfigurationData loadConfiguration();

void saveConfiguration(const ConfigurationData& config);

ConfigurationData& appConfiguration();

void setAppConfiguration(ConfigurationData config);
