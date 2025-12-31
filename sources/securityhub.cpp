#include "securityhub.h"
#include <QFormLayout>
#include <QLabel>
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QDateTime>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QMessageBox>
#include <QRegularExpression>

SecurityHub::SecurityHub(const QString &projectPath, QWidget *parent)
    : QWidget(parent), m_ProjectPath(projectPath)
{
    m_NetworkManager = new QNetworkAccessManager(this);
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
    logMessage(tr("Scanning for hardcoded endpoints and certificate logic..."), "info");
    
    if (m_ProjectPath.isEmpty()) {
        logMessage(tr("No project loaded."), "error");
        return;
    }
    
    // Collect relevant smali code context
    QString context = collectSmaliContext("http|https|TrustManager|Certificate|X509|SSLContext|HostnameVerifier");
    
    QString prompt = QString(
        "You are a mobile security expert. Analyze this Android smali/Java code for network security issues:\n\n"
        "%1\n\n"
        "Identify:\n"
        "1. Hardcoded URLs and endpoints\n"
        "2. Custom TrustManager implementations that bypass SSL validation\n"
        "3. HostnameVerifier bypasses\n"
        "4. Certificate pinning implementations\n"
        "5. Insecure HTTP usage\n\n"
        "Provide a risk assessment and recommendations. Format as Markdown."
    ).arg(context.left(15000));
    
    askAI(prompt, [this](const QString &response) {
        logMessage(response, "ai");
    });
}

void SecurityHub::analyzeTampering() {
    logMessage(tr("Searching for signature checks and root detection..."), "info");
    
    if (m_ProjectPath.isEmpty()) {
        logMessage(tr("No project loaded."), "error");
        return;
    }
    
    QString context = collectSmaliContext("getSignature|PackageInfo|su|magisk|root|superuser|isRooted|isEmulator|Build.FINGERPRINT");
    
    QString prompt = QString(
        "You are a mobile security expert. Analyze this Android smali code for anti-tampering protections:\n\n"
        "%1\n\n"
        "Identify:\n"
        "1. Signature verification checks (PackageManager.getPackageInfo with GET_SIGNATURES)\n"
        "2. Root detection methods (checking for su, magisk, superuser apps)\n"
        "3. Emulator detection (Build.FINGERPRINT, Build.PRODUCT checks)\n"
        "4. Integrity checks\n"
        "5. Debug detection\n\n"
        "For each detection found, provide:\n"
        "- File path and method name\n"
        "- A Smali patch to bypass it (change return value or remove check)\n"
        "Format as Markdown with code blocks."
    ).arg(context.left(15000));
    
    askAI(prompt, [this](const QString &response) {
        logMessage(response, "ai");
    });
}

void SecurityHub::runSSLPinningBypass() {
    logMessage(tr("Generating Universal SSL Unpinning Patch..."), "info");
    
    if (m_ProjectPath.isEmpty()) {
        logMessage(tr("No project loaded."), "error");
        return;
    }
    
    QString context = collectSmaliContext("CertificatePinner|OkHttp|TrustManager|X509TrustManager|checkServerTrusted|checkClientTrusted");
    
    QString prompt = QString(
        "You are a mobile security expert specializing in SSL/TLS bypass. Analyze this Android smali code:\n\n"
        "%1\n\n"
        "Generate complete Smali patches to bypass ALL SSL pinning implementations found.\n\n"
        "For each pinning mechanism:\n"
        "1. Identify the exact file and method\n"
        "2. Provide the COMPLETE replacement smali code\n"
        "3. For X509TrustManager: make checkServerTrusted return without throwing\n"
        "4. For OkHttp CertificatePinner: bypass the check method\n"
        "5. For HostnameVerifier: always return true\n\n"
        "Also generate a Frida script for runtime bypass.\n"
        "Format as Markdown with proper smali code blocks."
    ).arg(context.left(15000));
    
    askAI(prompt, [this](const QString &response) {
        logMessage(response, "ai");
        
        // Save Frida script if found
        if (response.contains("Interceptor.attach") || response.contains("Java.perform")) {
            QFile script(m_ProjectPath + "/frida_ssl_bypass.js");
            if (script.open(QIODevice::WriteOnly)) {
                // Extract JS code from response
                QRegularExpression jsRegex("```javascript\\n([\\s\\S]*?)```");
                auto match = jsRegex.match(response);
                if (match.hasMatch()) {
                    script.write(match.captured(1).toUtf8());
                }
                script.close();
                logMessage(tr("Frida script saved to: frida_ssl_bypass.js"), "success");
            }
        }
    });
}

void SecurityHub::generateSecurityReport() {
    QString reportPath = m_ProjectPath + "/SECURITY_AUDIT.md";
    QFile f(reportPath);
    if (f.open(QFile::WriteOnly)) {
        QString report = "# Security Audit Report\n\n";
        report += "**Generated:** " + QDateTime::currentDateTime().toString(Qt::ISODate) + "\n\n";
        report += "**Project:** " + m_ProjectPath + "\n\n";
        report += "---\n\n";
        report += "## Analysis Log\n\n";
        report += m_AiSecurityLog->toPlainText();
        report += "\n\n---\n\n";
        report += "*Report generated by APK Studio Security Hub*\n";
        
        f.write(report.toUtf8());
        f.close();
        QMessageBox::information(this, tr("Report Saved"), tr("Security report saved to: %1").arg(reportPath));
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Could not write report file."));
    }
}

// ==================== Helper Methods ====================

QString SecurityHub::collectSmaliContext(const QString &searchPattern)
{
    QString context;
    QRegularExpression regex(searchPattern, QRegularExpression::CaseInsensitiveOption);
    
    QDirIterator it(m_ProjectPath, {"*.smali", "*.java"}, QDir::Files, QDirIterator::Subdirectories);
    
    while (it.hasNext() && context.length() < 30000) {
        QString filePath = it.next();
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        
        QString content = QString::fromUtf8(file.readAll());
        file.close();
        
        if (regex.match(content).hasMatch()) {
            // Extract relevant portions around matches
            QStringList lines = content.split('\n');
            QString relativePath = filePath.mid(m_ProjectPath.length() + 1);
            
            for (int i = 0; i < lines.size(); ++i) {
                if (regex.match(lines[i]).hasMatch()) {
                    context += "\n--- " + relativePath + " (line " + QString::number(i + 1) + ") ---\n";
                    // Get context: 5 lines before and after
                    int start = qMax(0, i - 5);
                    int end = qMin(lines.size() - 1, i + 5);
                    for (int j = start; j <= end; ++j) {
                        context += lines[j] + "\n";
                    }
                }
            }
        }
    }
    
    return context;
}

void SecurityHub::logMessage(const QString &msg, const QString &type)
{
    QString color = "#d4d4d4";
    QString prefix = "";
    
    if (type == "success") {
        color = "#7ee787";
        prefix = "✅ ";
    } else if (type == "error") {
        color = "#f85149";
        prefix = "❌ ";
    } else if (type == "warning") {
        color = "#d29922";
        prefix = "⚠️ ";
    } else if (type == "ai") {
        color = "#58a6ff";
        prefix = "🤖 AI: ";
    } else if (type == "info") {
        color = "#8b949e";
        prefix = "ℹ️ ";
    }
    
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    QString html = QString("<p><span style='color: #666;'>[%1]</span> <span style='color: %2;'>%3%4</span></p>")
                       .arg(timestamp, color, prefix, msg);
    m_AiSecurityLog->append(html);
}

void SecurityHub::askAI(const QString &prompt, std::function<void(const QString&)> callback)
{
    QSettings settings;
    QString key = settings.value("ai_api_key").toString();
    QString model = settings.value("ai_model", "gemini-2.0-flash-exp").toString();
    
    if (key.isEmpty()) {
        logMessage(tr("API Key not configured. Go to Settings > AI."), "error");
        return;
    }
    
    QJsonObject root;
    QJsonArray contents;
    QJsonObject content;
    QJsonArray parts;
    QJsonObject part;
    part["text"] = prompt;
    parts.append(part);
    content["parts"] = parts;
    contents.append(content);
    root["contents"] = contents;

    QNetworkRequest req;
    req.setUrl(QUrl(QString("https://generativelanguage.googleapis.com/v1beta/models/%1:generateContent?key=%2").arg(model, key)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = m_NetworkManager->post(req, QJsonDocument(root).toJson());
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QString text = doc.object()["candidates"].toArray()[0].toObject()["content"].toObject()["parts"].toArray()[0].toObject()["text"].toString();
            callback(text);
        } else {
            logMessage(tr("AI request failed: %1").arg(reply->errorString()), "error");
        }
        reply->deleteLater();
    });
}