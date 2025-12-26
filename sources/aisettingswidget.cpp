#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
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
    
    // CLI Agent options
    auto cliGroup = new QGroupBox(tr("CLI Agent (Advanced)"), this);
    auto cliLayout = new QVBoxLayout(cliGroup);
    
    m_CheckUseCliAgent = new QCheckBox(tr("Use CLI Agent mode (requires Node.js)"), this);
    cliLayout->addWidget(m_CheckUseCliAgent);
    
    auto cliInfoLabel = new QLabel(tr("CLI Agent provides interactive terminal-based AI assistance with "
                                       "direct file system access. Requires Node.js and provider's CLI tool."), this);
    cliInfoLabel->setWordWrap(true);
    cliInfoLabel->setStyleSheet("color: gray; font-size: 11px;");
    cliLayout->addWidget(cliInfoLabel);
    
    // CLI Status
    auto cliStatusLayout = new QHBoxLayout();
    m_CliStatusLabel = new QLabel(tr("Status: Not checked"), this);
    m_CliStatusLabel->setStyleSheet("font-size: 11px;");
    cliStatusLayout->addWidget(m_CliStatusLabel);
    cliStatusLayout->addStretch();
    
    m_CheckCliButton = new QPushButton(tr("Check"), this);
    m_CheckCliButton->setFixedWidth(80);
    connect(m_CheckCliButton, &QPushButton::clicked, this, &AISettingsWidget::checkCliStatus);
    cliStatusLayout->addWidget(m_CheckCliButton);
    
    m_InstallCliButton = new QPushButton(tr("Install"), this);
    m_InstallCliButton->setFixedWidth(80);
    connect(m_InstallCliButton, &QPushButton::clicked, this, &AISettingsWidget::installCliAgent);
    cliStatusLayout->addWidget(m_InstallCliButton);
    
    cliLayout->addLayout(cliStatusLayout);
    layout->addWidget(cliGroup);
    
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
    m_CheckUseCliAgent->setChecked(settings.value("ai_use_cli_agent", false).toBool());
    
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

void AISettingsWidget::checkCliStatus()
{
    QString status;
    bool nodeOk = false;
    bool cliOk = false;
    
    // Check Node.js
    QProcess nodeCheck;
    nodeCheck.start("node", QStringList() << "--version");
    if (nodeCheck.waitForFinished(3000)) {
        QString nodeVersion = nodeCheck.readAllStandardOutput().trimmed();
        if (nodeVersion.startsWith("v")) {
            nodeOk = true;
            status += QString("✅ Node.js %1  ").arg(nodeVersion);
        }
    }
    if (!nodeOk) {
        status += "❌ Node.js not found  ";
    }
    
    // Check provider CLI
    QString provider = m_ComboProvider->currentData().toString();
    QString cliName;
    QStringList cliArgs;
    
    if (provider == "gemini") {
        cliName = "gemini";
        cliArgs << "--version";
    } else if (provider == "copilot") {
        cliName = "github-copilot-cli";
        cliArgs << "--version";
    }
    
    if (!cliName.isEmpty()) {
        QProcess cliCheck;
        cliCheck.start(cliName, cliArgs);
        if (cliCheck.waitForFinished(3000) && cliCheck.exitCode() == 0) {
            cliOk = true;
            status += QString("✅ %1 CLI").arg(provider);
        } else {
            status += QString("❌ %1 CLI not found").arg(provider);
        }
    }
    
    m_CliStatusLabel->setText(status);
    m_CliStatusLabel->setStyleSheet(QString("font-size: 11px; color: %1;")
        .arg((nodeOk && cliOk) ? "#4ec9b0" : "#f14c4c"));
}

void AISettingsWidget::installCliAgent()
{
    // Check Node.js first
    QProcess nodeCheck;
    nodeCheck.start("node", QStringList() << "--version");
    if (!nodeCheck.waitForFinished(3000) || !nodeCheck.readAllStandardOutput().trimmed().startsWith("v")) {
        QMessageBox::warning(this, tr("Node.js Required"),
            tr("Node.js is required for CLI agents.\n\n"
               "Please download and install from:\nhttps://nodejs.org/"));
        return;
    }
    
    QString provider = m_ComboProvider->currentData().toString();
    QString package;
    
    if (provider == "gemini") {
        package = "@anthropic-ai/gemini-cli";
    } else if (provider == "copilot") {
        package = "@anthropic-ai/github-copilot-cli";
    } else {
        QMessageBox::information(this, tr("Not Available"),
            tr("CLI agent is not available for the selected provider."));
        return;
    }
    
    m_InstallCliButton->setEnabled(false);
    m_InstallCliButton->setText(tr("Installing..."));
    
    QProcess *installer = new QProcess(this);
    connect(installer, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, installer, package](int exitCode, QProcess::ExitStatus) {
        m_InstallCliButton->setEnabled(true);
        m_InstallCliButton->setText(tr("Install"));
        
        if (exitCode == 0) {
            QMessageBox::information(this, tr("Success"),
                tr("Successfully installed %1").arg(package));
            checkCliStatus();
        } else {
            QString error = installer->readAllStandardError();
            QMessageBox::warning(this, tr("Installation Failed"),
                tr("Failed to install %1:\n%2").arg(package, error));
        }
        installer->deleteLater();
    });
    
    installer->start("npm", QStringList() << "install" << "-g" << package);
}

void AISettingsWidget::save()
{
    QSettings settings;
    settings.setValue("ai_enabled", m_CheckEnabled->isChecked());
    settings.setValue("ai_provider", m_ComboProvider->currentData().toString());
    settings.setValue("ai_api_key", m_EditApiKey->text());
    settings.setValue("ai_model", m_EditModel->text());
    settings.setValue("ai_auto_analyze", m_CheckAutoAnalyze->isChecked());
    settings.setValue("ai_use_cli_agent", m_CheckUseCliAgent->isChecked());
    settings.sync();
}
