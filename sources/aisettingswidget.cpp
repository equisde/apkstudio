#include "aisettingswidget.h"
#include <QSettings>
#include <QFormLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QDir>
#include <QStandardPaths>

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

// ==================== Environment Detection ====================

bool AISettingsWidget::checkCommandAvailable(const QString &command, const QStringList &args, QString &output)
{
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(command, args);
    if (!process.waitForFinished(5000)) {
        return false;
    }
    output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    return process.exitCode() == 0;
}

QString AISettingsWidget::detectNvmPath()
{
#ifdef Q_OS_WIN
    QString nvmHome = qEnvironmentVariable("NVM_HOME");
    if (!nvmHome.isEmpty() && QDir(nvmHome).exists()) {
        return nvmHome;
    }
    QString userProfile = qEnvironmentVariable("USERPROFILE");
    QString defaultPath = userProfile + "/AppData/Roaming/nvm";
    if (QDir(defaultPath).exists()) return defaultPath;
#else
    QString home = QDir::homePath();
    QStringList paths = {home + "/.nvm", "/usr/local/opt/nvm"};
    for (const QString &p : paths) {
        if (QDir(p).exists()) return p;
    }
#endif
    return QString();
}

QString AISettingsWidget::detectNodePath()
{
    QString output;
#ifdef Q_OS_WIN
    if (checkCommandAvailable("node.exe", {"--version"}, output)) return output;
    if (checkCommandAvailable("node", {"--version"}, output)) return output;
#else
    if (checkCommandAvailable("node", {"--version"}, output)) return output;
#endif
    return QString();
}

EnvironmentStatus AISettingsWidget::detectFullEnvironment()
{
    EnvironmentStatus status;
    QString output;
    
    // Check NVM
    status.nvmPath = detectNvmPath();
    status.nvmAvailable = !status.nvmPath.isEmpty();
    
    // Check Node
    status.nodeVersion = detectNodePath();
    status.nodeAvailable = !status.nodeVersion.isEmpty();
    
    // Check NPM
    if (checkCommandAvailable("npm", {"--version"}, output)) {
        status.npmAvailable = true;
        status.npmVersion = output;
    }
    
    // Check Gemini CLI (@google/generative-ai)
    if (checkCommandAvailable("npx", {"--yes", "@anthropic-ai/claude-cli", "--version"}, output) ||
        checkCommandAvailable("gemini", {"--version"}, output)) {
        status.geminiCliAvailable = true;
        status.geminiVersion = output;
    }
    
    // Check GitHub Copilot CLI
    if (checkCommandAvailable("gh", {"copilot", "--version"}, output)) {
        status.copilotCliAvailable = true;
        status.copilotVersion = output;
    }
    
    // Check GitHub CLI
    if (checkCommandAvailable("gh", {"--version"}, output)) {
        status.ghAvailable = true;
    }
    
    return status;
}

void AISettingsWidget::updateStatusDisplay(const EnvironmentStatus &status)
{
    if (!m_CliStatusLabel) return;
    
    QString statusText;
    QString color;
    
    if (status.nodeAvailable && status.npmAvailable) {
        color = "#7ee787";
        statusText = tr("✅ Environment Ready - Node %1").arg(status.nodeVersion);
    } else if (status.nvmAvailable) {
        color = "#d29922";
        statusText = tr("⚠️ NVM found but Node not active");
    } else {
        color = "#f85149";
        statusText = tr("❌ Node.js not found - Install required");
    }
    
    m_CliStatusLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(color));
    m_CliStatusLabel->setText(statusText);
}

QString AISettingsWidget::getInstallCommand(const QString &provider)
{
    if (provider == "gemini") {
        return "npm install -g @google/generative-ai";
    } else if (provider == "openai") {
        return "npm install -g openai";
    }
    return QString();
}

QString AISettingsWidget::getCliCommand(const QString &provider)
{
    if (provider == "gemini") return "gemini";
    if (provider == "openai") return "openai";
    return QString();
}

bool AISettingsWidget::installPackageWithNpm(const QString &package, bool global)
{
    QProcess process;
    QStringList args = {"install"};
    if (global) args << "-g";
    args << package;
    
    process.start("npm", args);
    return process.waitForFinished(120000) && process.exitCode() == 0;
}

// ==================== Slot Implementations ====================

void AISettingsWidget::checkCliStatus()
{
    m_EnvStatus = detectFullEnvironment();
    updateStatusDisplay(m_EnvStatus);
    
    if (m_DetailedStatus) {
        QString report;
        report += tr("=== Environment Status ===\n");
        report += QString("NVM: %1 (%2)\n").arg(m_EnvStatus.nvmAvailable ? "Yes" : "No", m_EnvStatus.nvmPath);
        report += QString("Node: %1 (%2)\n").arg(m_EnvStatus.nodeAvailable ? "Yes" : "No", m_EnvStatus.nodeVersion);
        report += QString("NPM: %1 (%2)\n").arg(m_EnvStatus.npmAvailable ? "Yes" : "No", m_EnvStatus.npmVersion);
        report += QString("Gemini CLI: %1\n").arg(m_EnvStatus.geminiCliAvailable ? "Yes" : "No");
        report += QString("GitHub CLI: %1\n").arg(m_EnvStatus.ghAvailable ? "Yes" : "No");
        report += QString("Copilot CLI: %1\n").arg(m_EnvStatus.copilotCliAvailable ? "Yes" : "No");
        m_DetailedStatus->setText(report);
    }
}

void AISettingsWidget::installCliAgent()
{
    QString provider = m_ComboProvider->currentData().toString();
    QString installCmd = getInstallCommand(provider);
    
    if (installCmd.isEmpty()) {
        QMessageBox::warning(this, tr("Install CLI"), tr("Unknown provider selected."));
        return;
    }
    
    if (!m_EnvStatus.npmAvailable) {
        QMessageBox::warning(this, tr("Install CLI"), 
            tr("NPM is not available. Please install Node.js first."));
        return;
    }
    
    m_IsInstalling = true;
    if (m_ProgressBar) m_ProgressBar->setVisible(true);
    
    QProcess *process = new QProcess(this);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process](int exitCode, QProcess::ExitStatus) {
        m_IsInstalling = false;
        if (m_ProgressBar) m_ProgressBar->setVisible(false);
        
        if (exitCode == 0) {
            QMessageBox::information(this, tr("Install CLI"), tr("CLI agent installed successfully!"));
            checkCliStatus();
        } else {
            QMessageBox::critical(this, tr("Install CLI"), 
                tr("Installation failed: %1").arg(QString::fromUtf8(process->readAllStandardError())));
        }
        process->deleteLater();
    });
    
    QString package = (provider == "gemini") ? "@google/generative-ai" : "openai";
    process->start("npm", {"install", "-g", package});
}

void AISettingsWidget::installNodeViaWeb()
{
    QDesktopServices::openUrl(QUrl("https://nodejs.org/en/download/"));
    QMessageBox::information(this, tr("Install Node.js"),
        tr("Opening Node.js download page in your browser.\n\n"
           "Download and install the LTS version, then restart APK Studio."));
}

void AISettingsWidget::installNvm()
{
#ifdef Q_OS_WIN
    QDesktopServices::openUrl(QUrl("https://github.com/coreybutler/nvm-windows/releases"));
    QMessageBox::information(this, tr("Install NVM"),
        tr("Opening NVM for Windows download page.\n\n"
           "Download nvm-setup.exe and run the installer."));
#else
    QMessageBox::information(this, tr("Install NVM"),
        tr("Run this command in your terminal:\n\n"
           "curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.0/install.sh | bash\n\n"
           "Then restart your terminal and APK Studio."));
#endif
}

void AISettingsWidget::onProviderChanged(int index)
{
    QString provider = m_ComboProvider->itemData(index).toString();
    
    // Update default model based on provider
    if (provider == "gemini") {
        m_EditModel->setText("gemini-2.0-flash-exp");
    } else if (provider == "openai") {
        m_EditModel->setText("gpt-4o");
    }
    
    // Re-check CLI status for the new provider
    checkCliStatus();
}

void AISettingsWidget::runSmartDetection()
{
    if (m_CliStatusLabel) {
        m_CliStatusLabel->setText(tr("🔍 Detecting environment..."));
    }
    
    // Run full detection
    checkCliStatus();
    
    // Provide recommendations
    if (!m_EnvStatus.nodeAvailable) {
        int choice = QMessageBox::question(this, tr("Smart Detection"),
            tr("Node.js is not installed.\n\nWould you like to open the download page?"),
            QMessageBox::Yes | QMessageBox::No);
        if (choice == QMessageBox::Yes) {
            installNodeViaWeb();
        }
    } else if (!m_EnvStatus.geminiCliAvailable && m_ComboProvider->currentData().toString() == "gemini") {
        int choice = QMessageBox::question(this, tr("Smart Detection"),
            tr("Gemini CLI is not installed.\n\nWould you like to install it now?"),
            QMessageBox::Yes | QMessageBox::No);
        if (choice == QMessageBox::Yes) {
            installCliAgent();
        }
    } else {
        QMessageBox::information(this, tr("Smart Detection"),
            tr("✅ Your environment is fully configured and ready!"));
    }
}

void AISettingsWidget::showDetailedStatus()
{
    if (!m_DetailedStatus) return;
    
    bool isVisible = m_DetailedStatus->isVisible();
    m_DetailedStatus->setVisible(!isVisible);
    
    if (!isVisible) {
        checkCliStatus(); // Refresh status when showing
    }
}