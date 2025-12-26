#include <QDesktopServices>
#include <QDir>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QProcessEnvironment>
#include <QScrollArea>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include "aisettingswidget.h"

AISettingsWidget::AISettingsWidget(QWidget *parent)
    : QWidget(parent), m_IsInstalling(false)
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(12);
    
    // Enable AI
    m_CheckEnabled = new QCheckBox(tr("Enable AI Assistant"), this);
    m_CheckEnabled->setStyleSheet("font-weight: bold; font-size: 13px;");
    layout->addWidget(m_CheckEnabled);
    
    // Provider selection
    auto providerGroup = new QGroupBox(tr("AI Provider Configuration"), this);
    auto providerLayout = new QFormLayout(providerGroup);
    providerLayout->setSpacing(8);
    
    m_ComboProvider = new QComboBox(this);
    m_ComboProvider->addItem("🤖 Google Gemini", "gemini");
    m_ComboProvider->addItem("🐙 GitHub Copilot", "copilot");
    m_ComboProvider->addItem("🧠 OpenAI GPT", "openai");
    m_ComboProvider->addItem("🔮 Anthropic Claude", "anthropic");
    connect(m_ComboProvider, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &AISettingsWidget::onProviderChanged);
    providerLayout->addRow(tr("Provider:"), m_ComboProvider);
    
    m_EditApiKey = new QLineEdit(this);
    m_EditApiKey->setEchoMode(QLineEdit::Password);
    m_EditApiKey->setPlaceholderText(tr("Enter your API key..."));
    providerLayout->addRow(tr("API Key:"), m_EditApiKey);
    
    m_EditModel = new QLineEdit(this);
    m_EditModel->setPlaceholderText(tr("e.g., gemini-2.0-flash, gpt-4o, claude-3-sonnet"));
    providerLayout->addRow(tr("Model:"), m_EditModel);
    
    layout->addWidget(providerGroup);
    
    // CLI Agent options - Enhanced
    auto cliGroup = new QGroupBox(tr("🔧 CLI Agent (Advanced Mode)"), this);
    auto cliLayout = new QVBoxLayout(cliGroup);
    cliLayout->setSpacing(10);
    
    m_CheckUseCliAgent = new QCheckBox(tr("Use CLI Agent mode for enhanced capabilities"), this);
    cliLayout->addWidget(m_CheckUseCliAgent);
    
    auto cliInfoLabel = new QLabel(tr(
        "<p style='color: #888; font-size: 11px;'>"
        "CLI Agent provides interactive terminal-based AI assistance with direct file system access. "
        "This mode uses the official CLI tools from each provider for maximum capabilities.</p>"
        "<p style='color: #4ec9b0; font-size: 11px;'>"
        "<b>Gemini:</b> @google/gemini-cli | <b>Copilot:</b> gh copilot extension</p>"), this);
    cliInfoLabel->setWordWrap(true);
    cliInfoLabel->setTextFormat(Qt::RichText);
    cliLayout->addWidget(cliInfoLabel);
    
    // Smart detection button
    auto smartLayout = new QHBoxLayout();
    m_SmartDetectButton = new QPushButton(tr("🔍 Smart Detect Environment"), this);
    m_SmartDetectButton->setStyleSheet(R"(
        QPushButton {
            background: #0e639c;
            color: white;
            border: none;
            padding: 8px 16px;
            border-radius: 4px;
            font-weight: bold;
        }
        QPushButton:hover { background: #1177bb; }
        QPushButton:pressed { background: #0d5a8c; }
    )");
    connect(m_SmartDetectButton, &QPushButton::clicked, this, &AISettingsWidget::runSmartDetection);
    smartLayout->addWidget(m_SmartDetectButton);
    smartLayout->addStretch();
    cliLayout->addLayout(smartLayout);
    
    // Progress bar for installation
    m_ProgressBar = new QProgressBar(this);
    m_ProgressBar->setVisible(false);
    m_ProgressBar->setTextVisible(true);
    m_ProgressBar->setStyleSheet(R"(
        QProgressBar {
            border: 1px solid #3c3c3c;
            border-radius: 4px;
            text-align: center;
            background: #1e1e1e;
        }
        QProgressBar::chunk {
            background: #0e639c;
            border-radius: 3px;
        }
    )");
    cliLayout->addWidget(m_ProgressBar);
    
    // Status display
    m_CliStatusLabel = new QLabel(tr("Click 'Smart Detect Environment' to check your setup"), this);
    m_CliStatusLabel->setStyleSheet("font-size: 12px; padding: 8px; background: #252526; border-radius: 4px;");
    m_CliStatusLabel->setWordWrap(true);
    cliLayout->addWidget(m_CliStatusLabel);
    
    // Action buttons grid
    auto buttonGrid = new QGridLayout();
    buttonGrid->setSpacing(8);
    
    m_CheckCliButton = new QPushButton(tr("🔄 Refresh Status"), this);
    m_CheckCliButton->setStyleSheet("padding: 6px 12px;");
    connect(m_CheckCliButton, &QPushButton::clicked, this, &AISettingsWidget::checkCliStatus);
    buttonGrid->addWidget(m_CheckCliButton, 0, 0);
    
    m_InstallNvmButton = new QPushButton(tr("📦 Install NVM"), this);
    m_InstallNvmButton->setStyleSheet("padding: 6px 12px;");
    m_InstallNvmButton->setToolTip(tr("Install Node Version Manager for easier Node.js management"));
    connect(m_InstallNvmButton, &QPushButton::clicked, this, &AISettingsWidget::installNvm);
    buttonGrid->addWidget(m_InstallNvmButton, 0, 1);
    
    m_InstallNodeButton = new QPushButton(tr("🟢 Install Node.js"), this);
    m_InstallNodeButton->setStyleSheet("padding: 6px 12px;");
    m_InstallNodeButton->setToolTip(tr("Download and install Node.js LTS"));
    connect(m_InstallNodeButton, &QPushButton::clicked, this, &AISettingsWidget::installNodeViaWeb);
    buttonGrid->addWidget(m_InstallNodeButton, 0, 2);
    
    m_InstallCliButton = new QPushButton(tr("⚡ Install CLI Agent"), this);
    m_InstallCliButton->setStyleSheet("padding: 6px 12px; background: #28a745; color: white;");
    connect(m_InstallCliButton, &QPushButton::clicked, this, &AISettingsWidget::installCliAgent);
    buttonGrid->addWidget(m_InstallCliButton, 1, 0, 1, 3);
    
    cliLayout->addLayout(buttonGrid);
    
    // Detailed status area (collapsible)
    auto detailsBtn = new QPushButton(tr("📋 Show Detailed Status"), this);
    detailsBtn->setStyleSheet("padding: 4px 8px; font-size: 11px;");
    connect(detailsBtn, &QPushButton::clicked, this, &AISettingsWidget::showDetailedStatus);
    cliLayout->addWidget(detailsBtn);
    
    m_DetailedStatus = new QTextEdit(this);
    m_DetailedStatus->setReadOnly(true);
    m_DetailedStatus->setMaximumHeight(150);
    m_DetailedStatus->setVisible(false);
    m_DetailedStatus->setStyleSheet(R"(
        QTextEdit {
            background: #1e1e1e;
            color: #d4d4d4;
            font-family: 'Cascadia Code', 'Consolas', monospace;
            font-size: 11px;
            border: 1px solid #3c3c3c;
            border-radius: 4px;
        }
    )");
    cliLayout->addWidget(m_DetailedStatus);
    
    layout->addWidget(cliGroup);
    
    // Analysis options
    auto analysisGroup = new QGroupBox(tr("📊 Analysis Options"), this);
    auto analysisLayout = new QVBoxLayout(analysisGroup);
    
    m_CheckAutoAnalyze = new QCheckBox(tr("Automatically analyze projects when opened"), this);
    analysisLayout->addWidget(m_CheckAutoAnalyze);
    
    auto infoLabel = new QLabel(tr(
        "<p style='color: #888; font-size: 11px;'>"
        "When enabled, the AI will analyze your APK project structure, "
        "manifest, permissions, and code to provide insights and suggestions.</p>"), this);
    infoLabel->setWordWrap(true);
    infoLabel->setTextFormat(Qt::RichText);
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
    
    // Trigger provider change to set placeholders
    onProviderChanged(m_ComboProvider->currentIndex());
    
    // Auto-detect environment on load
    QTimer::singleShot(500, this, &AISettingsWidget::runSmartDetection);
}

void AISettingsWidget::onProviderChanged(int index)
{
    QString provider = m_ComboProvider->itemData(index).toString();
    if (provider == "gemini") {
        m_EditModel->setPlaceholderText("gemini-2.0-flash-exp");
    } else if (provider == "copilot" || provider == "openai") {
        m_EditModel->setPlaceholderText("gpt-4o");
    } else if (provider == "anthropic") {
        m_EditModel->setPlaceholderText("claude-3-5-sonnet-20241022");
    }
    
    // Update install button text based on provider
    m_InstallCliButton->setText(tr("⚡ Install %1 CLI").arg(
        provider == "gemini" ? "Gemini" : 
        provider == "copilot" ? "GitHub Copilot" : 
        provider == "openai" ? "OpenAI" : "Claude"));
}

QString AISettingsWidget::detectNvmPath()
{
    QString nvmPath;
    QString home = QDir::homePath();
    
#ifdef Q_OS_WIN
    // Windows: Check for nvm-windows in multiple locations
    QStringList possiblePaths = {
        QProcessEnvironment::systemEnvironment().value("NVM_HOME"),
        home + "/AppData/Roaming/nvm",
        "C:/nvm",
        "C:/Users/" + QDir(home).dirName() + "/nvm",
        QProcessEnvironment::systemEnvironment().value("APPDATA") + "/nvm"
    };
    
    for (const QString &path : possiblePaths) {
        if (!path.isEmpty() && QFile::exists(path + "/nvm.exe")) {
            return path;
        }
    }
    
    // Also check if nvm command is available
    QProcess nvmCheck;
    nvmCheck.start("cmd", QStringList() << "/c" << "where nvm");
    if (nvmCheck.waitForFinished(5000) && nvmCheck.exitCode() == 0) {
        QString output = nvmCheck.readAllStandardOutput().trimmed();
        if (!output.isEmpty()) {
            QFileInfo info(output.split("\n").first().trimmed());
            return info.absolutePath();
        }
    }
#else
    // macOS/Linux: Check for nvm
    QStringList possiblePaths = {
        QProcessEnvironment::systemEnvironment().value("NVM_DIR"),
        home + "/.nvm",
        "/usr/local/opt/nvm",
        "/opt/homebrew/opt/nvm"
    };
    
    for (const QString &path : possiblePaths) {
        if (!path.isEmpty() && QFile::exists(path + "/nvm.sh")) {
            return path;
        }
    }
#endif
    
    return nvmPath;
}

QString AISettingsWidget::detectNodePath()
{
    QProcess nodeCheck;
    
#ifdef Q_OS_WIN
    nodeCheck.start("cmd", QStringList() << "/c" << "where node");
#else
    nodeCheck.start("which", QStringList() << "node");
#endif
    
    if (nodeCheck.waitForFinished(5000) && nodeCheck.exitCode() == 0) {
        QString output = nodeCheck.readAllStandardOutput().trimmed();
        if (!output.isEmpty()) {
            return output.split("\n").first().trimmed();
        }
    }
    
    return QString();
}

bool AISettingsWidget::checkCommandAvailable(const QString &command, const QStringList &args, QString &output)
{
    QProcess proc;
    
#ifdef Q_OS_WIN
    // On Windows, prepend cmd /c for shell commands
    QStringList cmdArgs;
    cmdArgs << "/c" << command;
    cmdArgs.append(args);
    proc.start("cmd", cmdArgs);
#else
    proc.start(command, args);
#endif
    
    if (proc.waitForFinished(10000)) {
        output = proc.readAllStandardOutput().trimmed();
        if (output.isEmpty()) {
            output = proc.readAllStandardError().trimmed();
        }
        return proc.exitCode() == 0 || !output.isEmpty();
    }
    
    return false;
}

EnvironmentStatus AISettingsWidget::detectFullEnvironment()
{
    EnvironmentStatus status;
    QString output;
    
    // 1. Detect NVM
    status.nvmPath = detectNvmPath();
    status.nvmAvailable = !status.nvmPath.isEmpty();
    
    // 2. Detect Node.js
    if (checkCommandAvailable("node", QStringList() << "--version", output)) {
        status.nodeAvailable = output.startsWith("v");
        status.nodeVersion = output;
    }
    
    // 3. Detect npm
    if (checkCommandAvailable("npm", QStringList() << "--version", output)) {
        status.npmAvailable = true;
        status.npmVersion = output;
    }
    
    // 4. Detect Gemini CLI (@google/gemini-cli)
    // The gemini CLI from Google
    if (checkCommandAvailable("gemini", QStringList() << "--version", output)) {
        status.geminiCliAvailable = true;
        status.geminiVersion = output;
    } else if (checkCommandAvailable("gemini", QStringList() << "--help", output)) {
        // Some versions might not have --version
        status.geminiCliAvailable = output.contains("gemini") || output.contains("Gemini");
        status.geminiVersion = "installed";
    }
    
    // 5. Detect GitHub CLI (gh)
    if (checkCommandAvailable("gh", QStringList() << "--version", output)) {
        status.ghAvailable = true;
        
        // Check if copilot extension is installed
        QString copilotOutput;
        if (checkCommandAvailable("gh", QStringList() << "copilot" << "--version", copilotOutput)) {
            status.copilotCliAvailable = true;
            status.copilotVersion = copilotOutput;
        } else if (checkCommandAvailable("gh", QStringList() << "extension" << "list", copilotOutput)) {
            status.copilotCliAvailable = copilotOutput.contains("copilot");
            if (status.copilotCliAvailable) {
                status.copilotVersion = "extension installed";
            }
        }
    }
    
    return status;
}

void AISettingsWidget::runSmartDetection()
{
    m_SmartDetectButton->setEnabled(false);
    m_SmartDetectButton->setText(tr("🔍 Detecting..."));
    m_ProgressBar->setVisible(true);
    m_ProgressBar->setRange(0, 0); // Indeterminate
    
    // Run detection in a slight delay to allow UI update
    QTimer::singleShot(100, this, [this]() {
        m_EnvStatus = detectFullEnvironment();
        updateStatusDisplay(m_EnvStatus);
        
        m_SmartDetectButton->setEnabled(true);
        m_SmartDetectButton->setText(tr("🔍 Smart Detect Environment"));
        m_ProgressBar->setVisible(false);
    });
}

void AISettingsWidget::updateStatusDisplay(const EnvironmentStatus &status)
{
    QString html = "<div style='line-height: 1.6;'>";
    
    // NVM Status
    if (status.nvmAvailable) {
        html += QString("<span style='color: #4ec9b0;'>✅ NVM:</span> %1<br>").arg(status.nvmPath);
    } else {
        html += "<span style='color: #f14c4c;'>❌ NVM:</span> Not installed (optional but recommended)<br>";
    }
    
    // Node.js Status
    if (status.nodeAvailable) {
        html += QString("<span style='color: #4ec9b0;'>✅ Node.js:</span> %1<br>").arg(status.nodeVersion);
    } else {
        html += "<span style='color: #f14c4c;'>❌ Node.js:</span> <b>Required for CLI agents</b><br>";
    }
    
    // npm Status
    if (status.npmAvailable) {
        html += QString("<span style='color: #4ec9b0;'>✅ npm:</span> %1<br>").arg(status.npmVersion);
    } else if (status.nodeAvailable) {
        html += "<span style='color: #cca700;'>⚠️ npm:</span> Not found (usually comes with Node.js)<br>";
    }
    
    // Gemini CLI Status
    QString provider = m_ComboProvider->currentData().toString();
    if (provider == "gemini") {
        if (status.geminiCliAvailable) {
            html += QString("<span style='color: #4ec9b0;'>✅ Gemini CLI:</span> %1<br>").arg(
                status.geminiVersion.isEmpty() ? "Installed" : status.geminiVersion);
        } else {
            html += "<span style='color: #f14c4c;'>❌ Gemini CLI:</span> npm install -g @google/gemini-cli<br>";
        }
    }
    
    // GitHub Copilot Status
    if (provider == "copilot") {
        if (status.ghAvailable) {
            html += "<span style='color: #4ec9b0;'>✅ GitHub CLI:</span> Installed<br>";
            if (status.copilotCliAvailable) {
                html += QString("<span style='color: #4ec9b0;'>✅ Copilot Extension:</span> %1<br>").arg(
                    status.copilotVersion.isEmpty() ? "Installed" : status.copilotVersion);
            } else {
                html += "<span style='color: #f14c4c;'>❌ Copilot Extension:</span> gh extension install github/gh-copilot<br>";
            }
        } else {
            html += "<span style='color: #f14c4c;'>❌ GitHub CLI:</span> Required for Copilot<br>";
        }
    }
    
    html += "</div>";
    
    m_CliStatusLabel->setText(html);
    m_CliStatusLabel->setTextFormat(Qt::RichText);
    
    // Update button states based on what's missing
    m_InstallNvmButton->setEnabled(!status.nvmAvailable);
    m_InstallNodeButton->setEnabled(!status.nodeAvailable);
    
    bool canInstallCli = status.nodeAvailable && status.npmAvailable;
    if (provider == "copilot") {
        canInstallCli = status.ghAvailable;
    }
    m_InstallCliButton->setEnabled(canInstallCli);
    
    // Store status
    m_EnvStatus = status;
}

void AISettingsWidget::checkCliStatus()
{
    runSmartDetection();
}

void AISettingsWidget::showDetailedStatus()
{
    bool visible = !m_DetailedStatus->isVisible();
    m_DetailedStatus->setVisible(visible);
    
    if (visible) {
        QString details;
        details += "=== Environment Detection Details ===\n\n";
        
        details += QString("Platform: %1\n").arg(
#ifdef Q_OS_WIN
            "Windows"
#elif defined(Q_OS_MAC)
            "macOS"
#else
            "Linux"
#endif
        );
        
        details += QString("Home Directory: %1\n").arg(QDir::homePath());
        details += QString("NVM Path: %1\n").arg(m_EnvStatus.nvmPath.isEmpty() ? "Not found" : m_EnvStatus.nvmPath);
        details += QString("Node Version: %1\n").arg(m_EnvStatus.nodeVersion.isEmpty() ? "Not found" : m_EnvStatus.nodeVersion);
        details += QString("npm Version: %1\n").arg(m_EnvStatus.npmVersion.isEmpty() ? "Not found" : m_EnvStatus.npmVersion);
        details += QString("Gemini CLI: %1\n").arg(m_EnvStatus.geminiCliAvailable ? "Available" : "Not installed");
        details += QString("GitHub CLI: %1\n").arg(m_EnvStatus.ghAvailable ? "Available" : "Not installed");
        details += QString("Copilot Extension: %1\n").arg(m_EnvStatus.copilotCliAvailable ? "Available" : "Not installed");
        
        details += "\n=== Recommended Commands ===\n\n";
        
        QString provider = m_ComboProvider->currentData().toString();
        if (provider == "gemini") {
            details += "Install Gemini CLI:\n";
            details += "  npm install -g @google/gemini-cli\n\n";
            details += "Run Gemini CLI:\n";
            details += "  gemini\n";
        } else if (provider == "copilot") {
            details += "Install GitHub CLI (if not installed):\n";
#ifdef Q_OS_WIN
            details += "  winget install GitHub.cli\n\n";
#elif defined(Q_OS_MAC)
            details += "  brew install gh\n\n";
#else
            details += "  sudo apt install gh\n\n";
#endif
            details += "Install Copilot extension:\n";
            details += "  gh auth login\n";
            details += "  gh extension install github/gh-copilot\n\n";
            details += "Run Copilot:\n";
            details += "  gh copilot suggest\n";
        }
        
        m_DetailedStatus->setPlainText(details);
    }
}

void AISettingsWidget::installNvm()
{
    QString url;
    QString message;
    
#ifdef Q_OS_WIN
    url = "https://github.com/coreybutler/nvm-windows/releases/latest";
    message = tr("This will open the NVM for Windows download page.\n\n"
                 "Download and run the installer, then restart ApkStudio.");
#else
    url = "https://github.com/nvm-sh/nvm#installing-and-updating";
    message = tr("This will open the NVM installation guide.\n\n"
                 "You can install NVM with:\n"
                 "curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.40.1/install.sh | bash");
#endif
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, 
        tr("Install NVM"), message + "\n\n" + tr("Open download page?"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        QDesktopServices::openUrl(QUrl(url));
    }
}

void AISettingsWidget::installNodeViaWeb()
{
    QString message = tr("This will open the Node.js download page.\n\n"
                        "Recommended version: LTS (Long Term Support)\n"
                        "This is required for CLI agents.\n\n"
                        "Open download page?");
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, 
        tr("Install Node.js"), message,
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        QDesktopServices::openUrl(QUrl("https://nodejs.org/en/download/"));
    }
}

QString AISettingsWidget::getInstallCommand(const QString &provider)
{
    if (provider == "gemini") {
        return "@google/gemini-cli";
    } else if (provider == "copilot") {
        return "gh extension install github/gh-copilot";
    } else if (provider == "openai") {
        return "@openai/cli"; // OpenAI doesn't have official CLI yet
    } else if (provider == "anthropic") {
        return "@anthropic-ai/claude-code"; // Anthropic CLI
    }
    return QString();
}

QString AISettingsWidget::getCliCommand(const QString &provider)
{
    if (provider == "gemini") {
        return "gemini";
    } else if (provider == "copilot") {
        return "gh copilot";
    }
    return QString();
}

void AISettingsWidget::installCliAgent()
{
    if (m_IsInstalling) return;
    
    QString provider = m_ComboProvider->currentData().toString();
    
    // Special handling for GitHub Copilot
    if (provider == "copilot") {
        if (!m_EnvStatus.ghAvailable) {
            QMessageBox::warning(this, tr("GitHub CLI Required"),
                tr("GitHub CLI (gh) is required for Copilot.\n\n"
                   "Please install it first:\n"
#ifdef Q_OS_WIN
                   "winget install GitHub.cli"
#elif defined(Q_OS_MAC)
                   "brew install gh"
#else
                   "sudo apt install gh"
#endif
                ));
            return;
        }
        
        m_IsInstalling = true;
        m_InstallCliButton->setEnabled(false);
        m_InstallCliButton->setText(tr("Installing..."));
        m_ProgressBar->setVisible(true);
        m_ProgressBar->setRange(0, 0);
        
        QProcess *installer = new QProcess(this);
        connect(installer, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, installer](int exitCode, QProcess::ExitStatus) {
            m_IsInstalling = false;
            m_InstallCliButton->setEnabled(true);
            m_InstallCliButton->setText(tr("⚡ Install GitHub Copilot CLI"));
            m_ProgressBar->setVisible(false);
            
            if (exitCode == 0) {
                QMessageBox::information(this, tr("Success"),
                    tr("GitHub Copilot extension installed successfully!\n\n"
                       "You may need to run 'gh auth login' if not already authenticated."));
                runSmartDetection();
            } else {
                QString error = installer->readAllStandardError();
                QString output = installer->readAllStandardOutput();
                QMessageBox::warning(this, tr("Installation Failed"),
                    tr("Failed to install Copilot extension:\n%1\n%2").arg(error, output));
            }
            installer->deleteLater();
        });
        
#ifdef Q_OS_WIN
        installer->start("cmd", QStringList() << "/c" << "gh" << "extension" << "install" << "github/gh-copilot");
#else
        installer->start("gh", QStringList() << "extension" << "install" << "github/gh-copilot");
#endif
        return;
    }
    
    // For npm-based installations (Gemini, etc.)
    if (!m_EnvStatus.nodeAvailable || !m_EnvStatus.npmAvailable) {
        QMessageBox::warning(this, tr("Node.js Required"),
            tr("Node.js and npm are required for this CLI agent.\n\n"
               "Please install Node.js first."));
        return;
    }
    
    QString package = getInstallCommand(provider);
    if (package.isEmpty()) {
        QMessageBox::information(this, tr("Not Available"),
            tr("CLI agent is not available for the selected provider."));
        return;
    }
    
    m_IsInstalling = true;
    m_InstallCliButton->setEnabled(false);
    m_InstallCliButton->setText(tr("Installing %1...").arg(package));
    m_ProgressBar->setVisible(true);
    m_ProgressBar->setRange(0, 0);
    
    QProcess *installer = new QProcess(this);
    connect(installer, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, installer, package](int exitCode, QProcess::ExitStatus) {
        m_IsInstalling = false;
        m_InstallCliButton->setEnabled(true);
        m_InstallCliButton->setText(tr("⚡ Install CLI Agent"));
        m_ProgressBar->setVisible(false);
        
        if (exitCode == 0) {
            QMessageBox::information(this, tr("Success"),
                tr("Successfully installed %1!\n\n"
                   "You can now use CLI Agent mode.").arg(package));
            runSmartDetection();
        } else {
            QString error = installer->readAllStandardError();
            QString output = installer->readAllStandardOutput();
            
            // Provide helpful error messages
            QString helpMsg;
            if (error.contains("EACCES") || error.contains("permission")) {
                helpMsg = tr("\n\nTry running with administrator privileges or use:\nnpm config set prefix ~/.npm-global");
            }
            
            QMessageBox::warning(this, tr("Installation Failed"),
                tr("Failed to install %1:\n%2%3").arg(package, error + "\n" + output, helpMsg));
        }
        installer->deleteLater();
    });
    
#ifdef Q_OS_WIN
    installer->start("cmd", QStringList() << "/c" << "npm" << "install" << "-g" << package);
#else
    installer->start("npm", QStringList() << "install" << "-g" << package);
#endif
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
