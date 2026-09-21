#pragma once

#include <qstring.h>

#include "language.h"

struct Settings
{
    Language language;
};

Settings loadSettings();

void saveSettings(const Settings& settings);
