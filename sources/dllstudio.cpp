#include "dllstudio.h"
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QTime>
#include <QFile>
#include <QMessageBox>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDirIterator>

DllStudio::DllStudio(const QString &projectPath, QWidget *parent)
    : QWidget(parent), m_ProjectPath(projectPath)
{
    m_NetworkManager = new QNetworkAccessManager(this);
    setupUI();
    if (!m_ProjectPath.isEmpty()) scanDlls();
}

void DllStudio::setupUI() {
    auto layout = new QVBoxLayout(this);
    
    auto header = new QLabel(tr("<b>UNITY C# STUDIO</b>"));
    header->setStyleSheet("color: #58a6ff; font-size: 14px;");
    layout->addWidget(header);

    m_DllList = new QListWidget();
    m_DllList->setStyleSheet("background-color: #161b22; color: #c9d1d9;");
    layout->addWidget(new QLabel(tr("Managed DLLs:")));
    layout->addWidget(m_DllList);

    auto btnAnalyze = new QPushButton(tr("🤖 AI Code Analysis (C#)"));
    btnAnalyze->setStyleSheet("background-color: #238636; color: white;");
    connect(btnAnalyze, &QPushButton::clicked, this, &DllStudio::analyzeWithAI);
    layout->addWidget(btnAnalyze);

    m_AiInsights = new QTextBrowser();
    m_AiInsights->setStyleSheet("background-color: #0d1117; color: #d1d5da; font-family: monospace;");
    layout->addWidget(m_AiInsights);
}

void DllStudio::setProjectPath(const QString &path) {
    m_ProjectPath = path;
    scanDlls();
    scanExtractedSource();
}

void DllStudio::scanDlls() {
    m_DllList->clear();
    QDir managedDir(m_ProjectPath + "/assets/bin/Data/Managed");
    for (const auto &info : managedDir.entryInfoList({"*.dll"}, QDir::Files)) {
        m_DllList->addItem(info.fileName());
    }
}

void DllStudio::scanExtractedSource() {
    QDir srcDir(m_ProjectPath + "/csharp_src");
    if (srcDir.exists()) {
        m_AiInsights->append(tr("Extracted C# sources found in /csharp_src/"));
    }
}

void DllStudio::analyzeWithAI() {
    auto item = m_DllList->currentItem();
    if (!item) {
        QMessageBox::warning(this, "DLL Studio", "Select a DLL first.");
        return;
    }
    
    QString dllName = item->text();
    QString dllPath = m_ProjectPath + "/assets/bin/Data/Managed/" + dllName;
    
    logMessage("AI analyzing: " + dllName);
    
    // Check for extracted C# source
    QString srcDir = m_ProjectPath + "/csharp_src";
    QString csharpContext;
    
    if (QDir(srcDir).exists()) {
        // Collect C# source code context
        QDirIterator it(srcDir, {"*.cs"}, QDir::Files, QDirIterator::Subdirectories);
        int filesRead = 0;
        while (it.hasNext() && csharpContext.length() < 25000 && filesRead < 20) {
            QString filePath = it.next();
            QFile file(filePath);
            if (file.open(QIODevice::ReadOnly)) {
                QString content = QString::fromUtf8(file.readAll());
                file.close();
                
                // Look for interesting patterns
                if (content.contains("Purchase") || content.contains("Premium") || 
                    content.contains("License") || content.contains("Player") ||
                    content.contains("Health") || content.contains("Money") ||
                    content.contains("Coin") || content.contains("Score")) {
                    
                    QString relativePath = filePath.mid(srcDir.length() + 1);
                    csharpContext += "\n--- " + relativePath + " ---\n";
                    csharpContext += content.left(3000) + "\n";
                    filesRead++;
                }
            }
        }
    }
    
    QString dllInfo = extractDllInfo(dllPath);
    
    QString prompt = QString(
        "You are a Unity game reverse engineering expert specializing in C# and IL2CPP analysis.\n\n"
        "**DLL:** %1\n"
        "**Info:** %2\n\n"
        "%3\n\n"
        "Analyze this Unity game code and provide:\n\n"
        "1. **Game Mechanics**: Identify player stats, currency, health, score systems\n"
        "2. **IAP/Purchase Logic**: Find in-app purchase validation and bypass methods\n"
        "3. **Cheat Vectors**: Methods that control game values (SetMoney, AddHealth, etc.)\n"
        "4. **Mod Recommendations**: Specific code changes to:\n"
        "   - Unlock premium features\n"
        "   - Get unlimited currency/resources\n"
        "   - Bypass license checks\n"
        "   - Disable ads\n\n"
        "Provide actual C# code modifications or IL2CPP offset patches."
    ).arg(dllName, dllInfo, csharpContext.isEmpty() ? "" : "**Extracted C# Source:**\n" + csharpContext);
    
    m_AiInsights->append("<h3>AI Analysis: " + dllName + "</h3>");
    m_AiInsights->append("<i>Analyzing Unity C# code...</i>");
    
    askAI(prompt, [this, dllName](const QString &response) {
        m_AiInsights->append("<div style='color: #7ee787; white-space: pre-wrap;'>" + response + "</div>");
        
        // Save analysis to file
        QString baseName = dllName;
        baseName.replace(".dll", "");
        QString reportPath = m_ProjectPath + "/AI_DLL_ANALYSIS_" + baseName + ".md";
        QFile report(reportPath);
        if (report.open(QIODevice::WriteOnly)) {
            report.write(("# AI Analysis: " + dllName + "\n\n" + response).toUtf8());
            report.close();
            logMessage("Report saved: " + reportPath);
        }
    });
}

QString DllStudio::extractDllInfo(const QString &dllPath)
{
    QString info;
    QFileInfo fi(dllPath);
    
    if (!fi.exists()) {
        return "DLL not found";
    }
    
    info += "Size: " + QString::number(fi.size() / 1024) + " KB\n";
    info += "Last Modified: " + fi.lastModified().toString() + "\n";
    
    // Read first bytes to check if it's a valid .NET assembly
    QFile file(dllPath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray header = file.read(512);
        file.close();
        
        if (header.contains("MZ")) {
            info += "Type: .NET Assembly (Managed DLL)\n";
        }
    }
    
    return info;
}

void DllStudio::logMessage(const QString &msg)
{
    m_AiInsights->append("<span style='color: #8b949e;'>[" + QTime::currentTime().toString() + "]</span> " + msg);
}

void DllStudio::askAI(const QString &prompt, std::function<void(const QString&)> callback)
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
