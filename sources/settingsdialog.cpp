#include <QHBoxLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QIcon>
#include "settingsdialog.h"
#include "appearancesettingswidget.h"
#include "binarysettingswidget.h"
#include "signingconfigwidget.h"
#include "aisettingswidget.h"
#include "securitysettingswidget.h"
#include "languagesettingswidget.h"

SettingsDialog::SettingsDialog(const int page, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Settings"));
    setMinimumSize(850, 600);
    setStyleSheet("QDialog { background-color: #1e1e1e; color: #cccccc; } "
                  "QListWidget { background-color: #252526; border: none; outline: none; } "
                  "QListWidget::item { padding: 10px; color: #cccccc; } "
                  "QListWidget::item:selected { background-color: #37373d; color: #ffffff; border-left: 3px solid #007acc; }");

    auto mainLayout = new QVBoxLayout(this);
    
    auto contentLayout = new QHBoxLayout();
    m_OptionsList = new QListWidget(this);
    m_OptionsList->setFixedWidth(200);
    
    m_WidgetStack = new QStackedWidget(this);
    
    // Pages
    m_AppearanceSettingsWidget = new AppearanceSettingsWidget(this);
    m_BinarySettingsWidget = new BinarySettingsWidget(this);
    m_SigningConfigWidget = new SigningConfigWidget(this);
    m_AISettingsWidget = new AISettingsWidget(this);
    m_SecuritySettingsWidget = new SecuritySettingsWidget(this);
    m_LanguageSettingsWidget = new LanguageSettingsWidget(this);

    auto addPage = [&](const QString &name, const QString &icon, QWidget *widget) {
        m_OptionsList->addItem(new QListWidgetItem(QIcon(icon), name));
        m_WidgetStack->addWidget(widget);
    };

    addPage(tr("Appearance"), ":/icons/fugue/color.png", m_AppearanceSettingsWidget);
    addPage(tr("Binaries"), ":/icons/fugue/application-terminal.png", m_BinarySettingsWidget);
    addPage(tr("Signing"), ":/icons/fugue/edit-signiture.png", m_SigningConfigWidget);
    addPage(tr("AI Assistant"), ":/icons/icons8/icons8-gear-48.png", m_AISettingsWidget);
    addPage(tr("Security Hub"), ":/icons/icons8/icons8-software-installer-48.png", m_SecuritySettingsWidget);
    addPage(tr("Language"), ":/icons/fugue/gear.png", m_LanguageSettingsWidget);

    contentLayout->addWidget(m_OptionsList);
    contentLayout->addWidget(m_WidgetStack, 1);
    
    mainLayout->addLayout(contentLayout);
    
    m_ButtonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    m_ButtonBox->setStyleSheet("QPushButton { background-color: #0e639c; color: white; padding: 5px 15px; border: none; } "
                               "QPushButton:hover { background-color: #1177bb; }");
    
    connect(m_ButtonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::handleSave);
    connect(m_ButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(m_ButtonBox);

    connect(m_OptionsList, &QListWidget::currentRowChanged, m_WidgetStack, &QStackedWidget::setCurrentIndex);
    
    if (page >= 0 && page < m_OptionsList->count()) {
        m_OptionsList->setCurrentRow(page);
    } else {
        m_OptionsList->setCurrentRow(0);
    }
}

void SettingsDialog::handleSave()
{
    m_AppearanceSettingsWidget->save();
    m_BinarySettingsWidget->save();
    m_SigningConfigWidget->save();
    m_AISettingsWidget->save();
    m_SecuritySettingsWidget->save();
    m_LanguageSettingsWidget->save();
    accept();
}

SettingsDialog::~SettingsDialog() {}