#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSettings>
#include <QVBoxLayout>
#include "aisettingswidget.h"

AISettingsWidget::AISettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    
    // Enable AI
    m_CheckEnabled = new QCheckBox(tr("Enable AI Assistant"), this);
    layout->addWidget(m_CheckEnabled);
    
    // Provider selection
    auto providerGroup = new QGroupBox(tr("AI Provider"), this);
    auto providerLayout = new QFormLayout(providerGroup);
    
    m_ComboProvider = new QComboBox(this);
    m_ComboProvider->addItem("Google Gemini", "gemini");
    m_ComboProvider->addItem("GitHub Copilot", "copilot");
    m_ComboProvider->addItem("OpenAI", "openai");
    m_ComboProvider->addItem("Anthropic Claude", "anthropic");
    providerLayout->addRow(tr("Provider:"), m_ComboProvider);
    
    m_EditApiKey = new QLineEdit(this);
    m_EditApiKey->setEchoMode(QLineEdit::Password);
    m_EditApiKey->setPlaceholderText(tr("Enter your API key..."));
    providerLayout->addRow(tr("API Key:"), m_EditApiKey);
    
    m_EditModel = new QLineEdit(this);
    m_EditModel->setPlaceholderText(tr("e.g., gemini-2.0-flash, gpt-4o, claude-3-sonnet"));
    providerLayout->addRow(tr("Model:"), m_EditModel);
    
    layout->addWidget(providerGroup);
    
    // Analysis options
    auto analysisGroup = new QGroupBox(tr("Analysis Options"), this);
    auto analysisLayout = new QVBoxLayout(analysisGroup);
    
    m_CheckAutoAnalyze = new QCheckBox(tr("Automatically analyze projects when opened"), this);
    analysisLayout->addWidget(m_CheckAutoAnalyze);
    
    auto infoLabel = new QLabel(tr("When enabled, the AI will analyze your APK project structure, "
                                    "manifest, permissions, and code to provide insights and suggestions."), this);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: gray; font-size: 11px;");
    analysisLayout->addWidget(infoLabel);
    
    layout->addWidget(analysisGroup);
    layout->addStretch();
    
    // Load settings
    QSettings settings;
    m_CheckEnabled->setChecked(settings.value("ai_enabled", false).toBool());
    QString provider = settings.value("ai_provider", "gemini").toString();
    int providerIndex = m_ComboProvider->findData(provider);
    if (providerIndex >= 0) {
        m_ComboProvider->setCurrentIndex(providerIndex);
    }
    m_EditApiKey->setText(settings.value("ai_api_key").toString());
    m_EditModel->setText(settings.value("ai_model", "gemini-2.0-flash-exp").toString());
    m_CheckAutoAnalyze->setChecked(settings.value("ai_auto_analyze", true).toBool());
    
    // Update model placeholder based on provider
    connect(m_ComboProvider, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        QString provider = m_ComboProvider->itemData(index).toString();
        if (provider == "gemini") {
            m_EditModel->setPlaceholderText("gemini-2.0-flash-exp");
        } else if (provider == "copilot" || provider == "openai") {
            m_EditModel->setPlaceholderText("gpt-4o");
        } else if (provider == "anthropic") {
            m_EditModel->setPlaceholderText("claude-3-5-sonnet-20241022");
        }
    });
}

void AISettingsWidget::save()
{
    QSettings settings;
    settings.setValue("ai_enabled", m_CheckEnabled->isChecked());
    settings.setValue("ai_provider", m_ComboProvider->currentData().toString());
    settings.setValue("ai_api_key", m_EditApiKey->text());
    settings.setValue("ai_model", m_EditModel->text());
    settings.setValue("ai_auto_analyze", m_CheckAutoAnalyze->isChecked());
    settings.sync();
}
