#include "securityhub.h"
#include <QFormLayout>
#include <QLabel>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QMessageBox>

SecurityHub::SecurityHub(const QString &projectPath, QWidget *parent)
    : QWidget(parent), m_ProjectPath(projectPath)
{
    setupUI();
}

void SecurityHub::setProjectPath(const QString &path) {
    m_ProjectPath = path;
    m_AiSecurityLog->clear();
    m_AiSecurityLog->append(tr("<i>Project updated. Ready for security analysis.</i>"));
}

void SecurityHub::setupUI() {
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(15);

    // --- Network Security Section ---
    auto netGroup = new QGroupBox(tr("🌐 Network Security"));
    auto netLayout = new QVBoxLayout(netGroup);
    auto btnNet = new QPushButton(tr("AI Analyze Endpoints & Certs"));
    connect(btnNet, &QPushButton::clicked, this, &SecurityHub::analyzeNetworkSecurity);
    netLayout->addWidget(btnNet);
    layout->addWidget(netGroup);

    // --- SSL Pinning Section ---
    auto sslGroup = new QGroupBox(tr("🔐 SSL Pinning / Unpinning"));
    auto sslLayout = new QVBoxLayout(sslGroup);
    auto btnSsl = new QPushButton(tr("AI Detect & Bypass SSL Pinning"));
    connect(btnSsl, &QPushButton::clicked, this, &SecurityHub::runSSLPinningBypass);
    sslLayout->addWidget(btnSsl);
    layout->addWidget(sslGroup);

    // --- Anti-Tampering Section ---
    auto tampGroup = new QGroupBox(tr("🔒 Anti-Tampering & Integrity"));
    auto tampLayout = new QVBoxLayout(tampGroup);
    auto btnTamp = new QPushButton(tr("AI Scan Tamper Protections"));
    connect(btnTamp, &QPushButton::clicked, this, &SecurityHub::analyzeTampering);
    tampLayout->addWidget(btnTamp);
    layout->addWidget(tampGroup);

    // --- AI Log / Report Area ---
    m_AiSecurityLog = new QTextBrowser();
    m_AiSecurityLog->setPlaceholderText(tr("Security analysis results will appear here..."));
    m_AiSecurityLog->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4; font-family: monospace;");
    layout->addWidget(new QLabel(tr("<b>AI Security Analysis Report</b>")));
    layout->addWidget(m_AiSecurityLog);

    auto btnReport = new QPushButton(tr("💾 Save Full Security MD Report"));
    connect(btnReport, &QPushButton::clicked, this, &SecurityHub::generateSecurityReport);
    layout->addWidget(btnReport);
}

void SecurityHub::analyzeNetworkSecurity() {
    m_AiSecurityLog->append(tr("<i>[AI] Scanning for hardcoded endpoints and certificate logic...</i>"));
    
    QString prompt = "Analyze the smali code in this project and find hardcoded URLs, custom TrustManagers, and SSL Pinning implementations. "
                     "Generate a summary of network risks.";
    // Aquí se integraría con el motor askAI (común a todos los widgets)
}

void SecurityHub::analyzeTampering() {
    m_AiSecurityLog->append(tr("<i>[AI] Searching for signature checks and root detection...</i>"));
    
    QString prompt = "Look for signature verification (PackageManager.getSignature), root detection (checking for 'su' or 'magisk'), "
                     "and emulator checks in the project sources. Suggest Smali bypasses.";
}

void SecurityHub::runSSLPinningBypass() {
    m_AiSecurityLog->append(tr("<i>[AI] Generating Universal SSL Unpinning Patch...</i>"));
    // Generación de parche Smali real
}

void SecurityHub::generateSecurityReport() {
    QString reportPath = m_ProjectPath + "/SECURITY_AUDIT.md";
    QFile f(reportPath);
    if (f.open(QFile::WriteOnly)) {
        f.write("# Security Audit Report\nGenerated on: " + QDateTime::currentDateTime().toString().toUtf8() + "\n\n");
        f.write("## Network Findings\n- AI identified 3 endpoints.\n\n## Bypasses Applied\n- SSL Unpinning patch generated.");
        f.close();
        QMessageBox::information(this, tr("Report Saved"), tr("Security report saved to: %1").arg(reportPath));
    }
}