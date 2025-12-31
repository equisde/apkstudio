#include "nativesstudio.h"
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QTime>
#include <QDirIterator>
#include <QMessageBox>
#include <QTimer>
#include <QProcess>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>

NativeStudio::NativeStudio(const QString &projectPath, QWidget *parent)
    : QWidget(parent), m_ProjectPath(projectPath)
{
    m_NetworkManager = new QNetworkAccessManager(this);
    setupUI();
    if (!m_ProjectPath.isEmpty()) scanNativeLibs();
}

void NativeStudio::setupUI() {
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);

    auto header = new QLabel(tr("<b>NATIVE BINARY STUDIO</b>"));
    header->setStyleSheet("color: #f85149; font-size: 14px;");
    layout->addWidget(header);

    m_LibsList = new QListWidget();
    m_LibsList->setStyleSheet("background-color: #161b22; color: #c9d1d9; border: 1px solid #30363d;");
    layout->addWidget(new QLabel(tr("Detected Native Libraries (.so):")));
    layout->addWidget(m_LibsList);

    auto btnAnalyze = new QPushButton(tr("🤖 AI Binary Analysis"));
    btnAnalyze->setStyleSheet("background-color: #238636; color: white; padding: 10px;");
    connect(btnAnalyze, &QPushButton::clicked, this, &NativeStudio::runAiBinaryAnalysis);
    layout->addWidget(btnAnalyze);

    m_AnalysisReport = new QTextBrowser();
    m_AnalysisReport->setStyleSheet("background-color: #0d1117; color: #d1d5da; font-family: monospace;");
    layout->addWidget(new QLabel(tr("<b>AI Reversing Insights:</b>")));
    layout->addWidget(m_AnalysisReport);

    auto btnHook = new QPushButton(tr("⚡ Generate Frida Hook Script"));
    btnHook->setStyleSheet("background-color: #1f6feb; color: white;");
    connect(btnHook, &QPushButton::clicked, this, &NativeStudio::generateNativeHook);
    layout->addWidget(btnHook);
}

void NativeStudio::setProjectPath(const QString &path) {
    m_ProjectPath = path;
    scanNativeLibs();
}

void NativeStudio::scanNativeLibs() {
    m_LibsList->clear();
    QDir libDir(m_ProjectPath + "/lib");
    if (!libDir.exists()) return;

    QDirIterator it(libDir.absolutePath(), {"*.so"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        m_LibsList->addItem(QFileInfo(it.next()).filePath().remove(m_ProjectPath + "/"));
    }
}

void NativeStudio::runAiBinaryAnalysis() {
    auto item = m_LibsList->currentItem();
    if (!item) {
        QMessageBox::warning(this, "Native Studio", "Select a .so library first.");
        return;
    }

    QString libRelPath = item->text();
    QString libFullPath = m_ProjectPath + "/" + libRelPath;
    logMessage("AI analyzing: " + libRelPath);
    
    // Extract library information
    QString libInfo = extractLibraryInfo(libFullPath);
    
    QString prompt = QString(
        "You are a low-level Android reverse engineering expert specializing in native code analysis.\n\n"
        "Analyze this native library information:\n"
        "**Library:** %1\n"
        "**Info:**\n%2\n\n"
        "Provide:\n"
        "1. **Architecture Analysis**: Identify the target architecture and any optimizations\n"
        "2. **Security Mechanisms**: Look for anti-debug (ptrace), anti-tampering, root detection in native code\n"
        "3. **Interesting Functions**: JNI functions, crypto operations, network calls\n"
        "4. **Hooking Targets**: Suggest Frida hook points with example code\n"
        "5. **Vulnerabilities**: Memory corruption, format strings, hardcoded secrets\n\n"
        "Format as Markdown with code examples."
    ).arg(libRelPath, libInfo);

    m_AnalysisReport->append("<h3>AI Native Analysis: " + libRelPath + "</h3>");
    m_AnalysisReport->append("<i>Analyzing binary structure...</i>");
    
    askAI(prompt, [this](const QString &response) {
        m_AnalysisReport->append("<div style='color: #7ee787;'>" + response + "</div>");
        logMessage("AI Analysis complete.");
    });
}

void NativeStudio::generateNativeHook() {
    if (m_LibsList->selectedItems().isEmpty()) return;
    QString libRelPath = m_LibsList->currentItem()->text();
    QString libName = QFileInfo(libRelPath).fileName();
    logMessage("Generating Frida script for " + libName);
    
    QString libInfo = extractLibraryInfo(m_ProjectPath + "/" + libRelPath);
    
    QString prompt = QString(
        "You are a Frida scripting expert. Generate a comprehensive Frida hook script for this Android native library.\n\n"
        "**Library:** %1\n"
        "**Info:**\n%2\n\n"
        "Generate a complete Frida script that:\n"
        "1. Hooks common security functions (ptrace, fopen on /proc/self/*, fork)\n"
        "2. Hooks JNI_OnLoad if present\n"
        "3. Hooks any crypto functions\n"
        "4. Logs function arguments and return values\n"
        "5. Includes bypass logic for common protections\n\n"
        "Output ONLY the JavaScript code, no markdown formatting."
    ).arg(libName, libInfo);
    
    askAI(prompt, [this, libName](const QString &response) {
        QString script = response;
        // Clean up if wrapped in code blocks
        script.replace(QRegularExpression("```javascript\\n?"), "");
        script.replace(QRegularExpression("```\\n?"), "");
        
        m_AnalysisReport->append("<h3>Frida Hook Generated</h3><pre style='background: #0d1117; padding: 10px;'>" + script + "</pre>");
        
        // Save to file
        QString scriptPath = m_ProjectPath + "/frida_hook_" + libName.replace(".so", "") + ".js";
        QFile f(scriptPath);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(script.toUtf8());
            f.close();
            logMessage("Script saved: " + scriptPath);
        }
    });
}

QString NativeStudio::extractLibraryInfo(const QString &libPath)
{
    QString info;
    QFileInfo fi(libPath);
    
    info += "Size: " + QString::number(fi.size() / 1024) + " KB\n";
    
    // Try to get ELF info using file command or readelf
    QProcess process;
    
#ifdef Q_OS_WIN
    // On Windows, just report basic info
    info += "Platform: Android Native Library (.so)\n";
#else
    // Try file command
    process.start("file", {libPath});
    if (process.waitForFinished(3000)) {
        info += "Type: " + QString::fromUtf8(process.readAllStandardOutput()).trimmed() + "\n";
    }
    
    // Try readelf for symbols
    process.start("readelf", {"-s", "--wide", libPath});
    if (process.waitForFinished(5000)) {
        QString symbols = QString::fromUtf8(process.readAllStandardOutput());
        QStringList lines = symbols.split('\n');
        QStringList interestingSymbols;
        
        for (const QString &line : lines) {
            if (line.contains("FUNC") && line.contains("GLOBAL")) {
                // Extract function name
                QStringList parts = line.split(QRegularExpression("\\s+"));
                if (parts.size() > 7) {
                    QString funcName = parts.last();
                    if (!funcName.startsWith("_") || funcName.startsWith("Java_") || 
                        funcName.contains("JNI") || funcName.contains("ptrace") ||
                        funcName.contains("anti") || funcName.contains("check")) {
                        interestingSymbols.append(funcName);
                    }
                }
            }
        }
        
        if (!interestingSymbols.isEmpty()) {
            info += "\nInteresting Symbols (" + QString::number(interestingSymbols.size()) + "):\n";
            for (int i = 0; i < qMin(30, interestingSymbols.size()); ++i) {
                info += "  - " + interestingSymbols[i] + "\n";
            }
        }
    }
#endif
    
    return info;
}

void NativeStudio::askAI(const QString &prompt, std::function<void(const QString&)> callback)
{
    QSettings settings;
    QString key = settings.value("ai_api_key").toString();
    QString model = settings.value("ai_model", "gemini-2.0-flash-exp").toString();
    
    if (key.isEmpty()) {
        logMessage("Error: API Key not set in Settings.");
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
            logMessage("AI request failed: " + reply->errorString());
        }
        reply->deleteLater();
    });
}

void NativeStudio::logMessage(const QString &msg) {
    m_AnalysisReport->append("<span style='color: #8b949e;'>[" + QTime::currentTime().toString() + "]</span> " + msg);
}