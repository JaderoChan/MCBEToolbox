#pragma once

#include <components/adaptive_widget.h>

class QComboBox;
class QLabel;
class QRadioButton;
class AboutCardWidget;

class SettingsWidget : public AdaptiveWidget
{
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget* parent = nullptr);

protected:
    void refreshText() override;

private:
    void rebuildLanguageCombo();

    QLabel*          languageLabel_       = nullptr;
    QComboBox*       languageCombo_       = nullptr;
    QLabel*          colorSchemeLabel_    = nullptr;
    QRadioButton*    colorSchemeAutoBtn_  = nullptr;
    QRadioButton*    colorSchemeLightBtn_ = nullptr;
    QRadioButton*    colorSchemeDarkBtn_  = nullptr;
    AboutCardWidget* aboutCard_           = nullptr;
};
