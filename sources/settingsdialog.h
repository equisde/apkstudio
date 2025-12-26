#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QStackedWidget>
#include <QWidget>
#include "aisettingswidget.h"
#include "appearancesettingswidget.h"
#include "binarysettingswidget.h"
#include "languagesettingswidget.h"
#include "signingconfigwidget.h"
#include "securitysettingswidget.h"

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(const int page = 0, QWidget *parent = nullptr);
    ~SettingsDialog();
private slots:
    void handleSave();
private:
    AISettingsWidget *m_AISettingsWidget;
    AppearanceSettingsWidget *m_AppearanceSettingsWidget;
    BinarySettingsWidget *m_BinarySettingsWidget;
    LanguageSettingsWidget *m_LanguageSettingsWidget;
    SigningConfigWidget *m_SigningConfigWidget;
    SecuritySettingsWidget *m_SecuritySettingsWidget;
    QDialogButtonBox *m_ButtonBox;
    QListWidget *m_OptionsList;
    QStackedWidget *m_WidgetStack;
};

#endif // SETTINGSDIALOG_H