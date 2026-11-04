#include "settings_widget.h"

#include <qbuttongroup.h>
#include <qcombobox.h>
#include <qframe.h>
#include <qgridlayout.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qradiobutton.h>
#include <qscrollarea.h>

#include <easy_translate.hpp>

#include "about_card_widget.h"

SettingsWidget::SettingsWidget(QWidget* parent)
    : AdaptiveWidget(parent)
{
    // 构造 UI 元素。
    languageLabel_       = new QLabel(this);
    languageCombo_       = new QComboBox(this);

    colorSchemeLabel_    = new QLabel(this);
    colorSchemeAutoBtn_  = new QRadioButton(this);
    colorSchemeLightBtn_ = new QRadioButton(this);
    colorSchemeDarkBtn_  = new QRadioButton(this);

    auto* colorSchemeButtonGroup = new QButtonGroup(this);
    colorSchemeButtonGroup->setExclusive(true);
    colorSchemeButtonGroup->addButton(colorSchemeAutoBtn_,  ColorSchemeManager::COLOR_SCHEME_AUTO);
    colorSchemeButtonGroup->addButton(colorSchemeLightBtn_, ColorSchemeManager::COLOR_SCHEME_LIGHT);
    colorSchemeButtonGroup->addButton(colorSchemeDarkBtn_,  ColorSchemeManager::COLOR_SCHEME_DARK);

    switch (ColorSchemeManager::instance().colorScheme())
    {
        case ColorSchemeManager::COLOR_SCHEME_LIGHT: colorSchemeLightBtn_->setChecked(true); break;
        case ColorSchemeManager::COLOR_SCHEME_DARK:  colorSchemeDarkBtn_->setChecked(true);  break;
        default:                                     colorSchemeAutoBtn_->setChecked(true);  break;
    }

    aboutCard_ = new AboutCardWidget(this);

    auto* colorSchemeButtonsWidget = new QWidget(this);
    auto* colorSchemeButtonsLayout = new QHBoxLayout(colorSchemeButtonsWidget);
    colorSchemeButtonsLayout->setContentsMargins(0, 0, 0, 0);
    colorSchemeButtonsLayout->setSpacing(16);
    colorSchemeButtonsLayout->addWidget(colorSchemeAutoBtn_);
    colorSchemeButtonsLayout->addWidget(colorSchemeLightBtn_);
    colorSchemeButtonsLayout->addWidget(colorSchemeDarkBtn_);
    colorSchemeButtonsLayout->addStretch(1);

    auto* optionsLayout = new QGridLayout();
    optionsLayout->setHorizontalSpacing(8);
    optionsLayout->setVerticalSpacing(16);
    optionsLayout->addWidget(languageLabel_,           0, 0);
    optionsLayout->addWidget(languageCombo_,           0, 1);
    optionsLayout->addWidget(colorSchemeLabel_,        1, 0);
    optionsLayout->addWidget(colorSchemeButtonsWidget, 1, 1);
    optionsLayout->setColumnStretch(0, 0);
    optionsLayout->setColumnStretch(1, 1);

    auto* content       = new QWidget;
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(24, 24, 24, 24);
    contentLayout->setSpacing(16);
    contentLayout->addLayout(optionsLayout);
    contentLayout->addSpacing(8);
    contentLayout->addWidget(aboutCard_);
    contentLayout->addStretch(1);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(content);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(scrollArea);

    // Connections
    connect(languageCombo_, &QComboBox::activated, this, [this](int index)
    {
        const auto language = static_cast<LanguageManager::Language>(languageCombo_->itemData(index).toInt());
        LanguageManager::instance().setLanguage(language);
    });
    connect(colorSchemeButtonGroup, &QButtonGroup::idClicked, this, [](int id)
    {
        ColorSchemeManager::instance().setColorScheme(static_cast<ColorSchemeManager::ColorScheme>(id));
    });

    refreshText();
}

void SettingsWidget::refreshText()
{
    languageLabel_->setText(EASYTR("Language:"));
    colorSchemeLabel_->setText(EASYTR("Theme:"));
    colorSchemeAutoBtn_->setText(EASYTR("Auto"));
    colorSchemeLightBtn_->setText(EASYTR("Light"));
    colorSchemeDarkBtn_->setText(EASYTR("Dark"));
    rebuildLanguageCombo();
}

void SettingsWidget::rebuildLanguageCombo()
{
    const auto current = LanguageManager::instance().language();

    languageCombo_->clear();

    languageCombo_->addItem(EASYTR("Auto"), static_cast<int>(LanguageManager::LANGUAGE_AUTO));
    for (int lang = LanguageManager::LANGUAGE_FIRST; lang <= LanguageManager::LANGUAGE_LAST; ++lang)
    {
        const auto language = static_cast<LanguageManager::Language>(lang);
        if (language == LanguageManager::LANGUAGE_AUTO)
            continue;

        const auto id = LanguageManager::languageId(language);
        if (!easytr::languageMapping().has(id.toStdString()))
            continue;
        languageCombo_->addItem(EASYTR("LANG_" + id.toStdString()), static_cast<int>(language));
    }

    for (int i = 0; i < languageCombo_->count(); ++i)
    {
        if (static_cast<LanguageManager::Language>(languageCombo_->itemData(i).toInt()) == current)
        {
            languageCombo_->setCurrentIndex(i);
            break;
        }
    }
}
