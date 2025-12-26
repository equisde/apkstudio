#include "aisettingswidget.h"
#include <QSettings>
#include <QFormLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>

AISettingsWidget::AISettingsWidget(QWidget *parent) : QWidget(parent)
{
    QSettings settings;
    auto layout = new QVBoxLayout(this);

    auto group = new QGroupBox(tr("🤖 AI Assistant & Agent Mode"));
    auto form = new QFormLayout(group);

    m_CheckEnabled = new QCheckBox(tr("Enable AI Features"));
    m_CheckEnabled->setChecked(settings.value("ai_enabled", true).toBool());
    form->addRow(m_CheckEnabled);

    m_CheckUseCliAgent = new QCheckBox(tr("Enable AI Agent Mode (Interactive CLI)"));
    m_CheckUseCliAgent->setToolTip(tr("Provides enhanced capabilities like multi-file editing and automatic patches."));
    m_CheckUseCliAgent->setChecked(settings.value("ai_use_cli_agent", true).toBool());
    form->addRow(m_CheckUseCliAgent);

    m_ComboProvider = new QComboBox();
    m_ComboProvider->addItem("Google Gemini", "gemini");
    m_ComboProvider->addItem("OpenAI GPT", "openai");
    m_ComboProvider->setCurrentText(settings.value("ai_provider", "gemini").toString());
    form->addRow(tr("Provider:"), m_ComboProvider);

    m_EditApiKey = new QLineEdit();
    m_EditApiKey->setEchoMode(QLineEdit::Password);
    m_EditApiKey->setText(settings.value("ai_api_key").toString());
    form->addRow(tr("API Key:"), m_EditApiKey);

    m_EditModel = new QLineEdit();
    m_EditModel->setText(settings.value("ai_model", "gemini-2.0-flash-exp").toString());
    form->addRow(tr("Model Name:"), m_EditModel);

    m_CheckAutoAnalyze = new QCheckBox(tr("Automatically analyze projects on open"));
    m_CheckAutoAnalyze->setChecked(settings.value("ai_auto_analyze", true).toBool());
    form->addRow(m_CheckAutoAnalyze);

    layout->addWidget(group);
    layout->addStretch();
}

void AISettingsWidget::save()
{
    QSettings settings;
    settings.setValue("ai_enabled", m_CheckEnabled->isChecked());
    settings.setValue("ai_use_cli_agent", m_CheckUseCliAgent->isChecked());
    settings.setValue("ai_provider", m_ComboProvider->currentData().toString());
    settings.setValue("ai_api_key", m_EditApiKey->text());
    settings.setValue("ai_model", m_EditModel->text());
    settings.setValue("ai_auto_analyze", m_CheckAutoAnalyze->isChecked());
    settings.sync();
}

// Stubs for missing methods if any
void AISettingsWidget::checkCliStatus() {}
void AISettingsWidget::installCliAgent() {}
void AISettingsWidget::installNodeViaWeb() {}
void AISettingsWidget::installNvm() {}
void AISettingsWidget::onProviderChanged(int) {}
void AISettingsWidget::runSmartDetection() {}
void AISettingsWidget::showDetailedStatus() {}