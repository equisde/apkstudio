#include "appmodstudio.h"
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QDir>
#include <QDirIterator>
#include <QTime>
#include <QFile>
#include <QRegularExpression>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>

AppModStudio::AppModStudio(const QString &projectPath, QWidget *parent)
    : QWidget(parent), m_ProjectPath(projectPath)
{
    m_NetworkManager = new QNetworkAccessManager(this);
    setupUI();
}

void AppModStudio::setProjectPath(const QString &path) {
    m_ProjectPath = path;
    m_ModTable->setRowCount(0);
    m_AiLog->append("Project analysis started for: " + path);
}

void AppModStudio::setupUI() {
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(15);

    auto group = new QGroupBox(tr("🔓 App Unlock & Mod"));
    auto innerLayout = new QVBoxLayout(group);

    m_ModTable = new QTableWidget(0, 3);
    m_ModTable->setHorizontalHeaderLabels({tr("Target Function"), tr("Mod Type"), tr("Status")});
    m_ModTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_ModTable->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4;");
    innerLayout->addWidget(m_ModTable);

    auto btnScan = new QPushButton(tr("🤖 AI Scan for Premium Features"));
    connect(btnScan, &QPushButton::clicked, this, &AppModStudio::scanForPremiumLogic);
    innerLayout->addWidget(btnScan);

    auto btnAiSuggest = new QPushButton(tr("💡 AI Suggest Modifications"));
    connect(btnAiSuggest, &QPushButton::clicked, this, &AppModStudio::aiSuggestModifications);
    innerLayout->addWidget(btnAiSuggest);

    layout->addWidget(group);

    m_AiLog = new QTextBrowser();
    m_AiLog->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4;");
    layout->addWidget(new QLabel(tr("<b>AI Modification Log</b>")));
    layout->addWidget(m_AiLog);

    auto btnPatch = new QPushButton(tr("⚡ Apply Selected AI Patches"));
    btnPatch->setStyleSheet("background-color: #1f6feb; color: white; font-weight: bold; padding: 10px;");
    connect(btnPatch, &QPushButton::clicked, this, &AppModStudio::applyUnlockPatch);
    layout->addWidget(btnPatch);
}

void AppModStudio::logMessage(const QString &msg) {
    m_AiLog->append("<i>[" + QTime::currentTime().toString() + "]</i> " + msg);
}

void AppModStudio::scanForPremiumLogic() {
    logMessage(tr("Scanning for premium/license logic..."));
    
    if (m_ProjectPath.isEmpty()) {
        logMessage(tr("Error: No project loaded."));
        return;
    }
    
    m_ModTable->setRowCount(0);
    m_ModTargets.clear();
    
    // Patterns that indicate premium/license checks
    QStringList patterns = {
        "isPremium", "isProUser", "isPurchased", "isLicensed", "isSubscribed",
        "hasPurchased", "checkLicense", "verifyLicense", "validatePurchase",
        "isTrialExpired", "isAdFree", "removeAds", "unlockPremium",
        "LicenseChecker", "BillingClient", "InAppPurchase", "getPurchase"
    };
    
    QDirIterator it(m_ProjectPath, {"*.smali"}, QDir::Files, QDirIterator::Subdirectories);
    
    while (it.hasNext()) {
        QString filePath = it.next();
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        
        QStringList lines = QString::fromUtf8(file.readAll()).split('\n');
        file.close();
        
        QString relativePath = filePath.mid(m_ProjectPath.length() + 1);
        QString currentMethod;
        
        for (int i = 0; i < lines.size(); ++i) {
            const QString &line = lines[i];
            
            // Track current method
            if (line.startsWith(".method")) {
                currentMethod = line;
            }
            
            // Check for patterns
            for (const QString &pattern : patterns) {
                if (line.contains(pattern, Qt::CaseInsensitive)) {
                    ModTarget target;
                    target.filePath = filePath;
                    target.lineNumber = i + 1;
                    target.functionName = currentMethod.isEmpty() ? "Unknown" : currentMethod.mid(8, 50);
                    target.modType = "Premium Check";
                    target.originalCode = line.trimmed();
                    
                    // Determine patch based on return type
                    if (line.contains("Z") || line.contains("boolean")) {
                        target.patchCode = "const/4 v0, 0x1  # Force return true";
                    } else {
                        target.patchCode = "# NOP or modify return value";
                    }
                    
                    m_ModTargets.append(target);
                    
                    int row = m_ModTable->rowCount();
                    m_ModTable->insertRow(row);
                    m_ModTable->setItem(row, 0, new QTableWidgetItem(target.functionName));
                    m_ModTable->setItem(row, 1, new QTableWidgetItem(target.modType));
                    m_ModTable->setItem(row, 2, new QTableWidgetItem("Found"));
                    m_ModTable->item(row, 0)->setData(Qt::UserRole, m_ModTargets.size() - 1);
                    
                    break; // One match per line is enough
                }
            }
        }
    }
    
    logMessage(tr("Found %1 potential premium check locations.").arg(m_ModTargets.size()));
}

void AppModStudio::applyUnlockPatch() {
    if (m_ModTargets.isEmpty()) {
        QMessageBox::warning(this, tr("App Mod"), tr("No targets found. Run scan first."));
        return;
    }
    
    QList<int> selectedRows;
    for (int i = 0; i < m_ModTable->rowCount(); ++i) {
        if (m_ModTable->item(i, 2)->text() == "Found") {
            selectedRows.append(i);
        }
    }
    
    if (selectedRows.isEmpty()) {
        QMessageBox::warning(this, tr("App Mod"), tr("No targets to patch."));
        return;
    }
    
    int patchedCount = 0;
    
    for (int row : selectedRows) {
        int targetIdx = m_ModTable->item(row, 0)->data(Qt::UserRole).toInt();
        if (targetIdx < 0 || targetIdx >= m_ModTargets.size()) continue;
        
        const ModTarget &target = m_ModTargets[targetIdx];
        
        QFile file(target.filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        
        QStringList lines = QString::fromUtf8(file.readAll()).split('\n');
        file.close();
        
        int lineIdx = target.lineNumber - 1;
        if (lineIdx < 0 || lineIdx >= lines.size()) continue;
        
        // Find the method and patch the return
        // Look for the return statement in the method
        bool inMethod = false;
        for (int i = lineIdx; i < qMin(lineIdx + 20, lines.size()); ++i) {
            if (lines[i].contains("return") && lines[i].contains("v")) {
                // Patch: force return true for boolean methods
                if (i > 0 && (lines[i-1].contains("move-result") || lines[i-1].contains("const"))) {
                    lines[i-1] = "    const/4 v0, 0x1  # PATCHED: Force true";
                    patchedCount++;
                    break;
                }
            }
        }
        
        // Write back
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) continue;
        file.write(lines.join('\n').toUtf8());
        file.close();
        
        m_ModTable->item(row, 2)->setText("Patched");
        m_ModTable->item(row, 2)->setForeground(QColor("#7ee787"));
    }
    
    logMessage(tr("Applied %1 patches.").arg(patchedCount));
    QMessageBox::information(this, tr("Patch Applied"), 
        tr("Successfully patched %1 locations.\nRecompile to apply changes.").arg(patchedCount));
}

void AppModStudio::aiSuggestModifications() {
    logMessage(tr("AI analyzing for premium feature flags..."));
    
    if (m_ProjectPath.isEmpty()) {
        logMessage(tr("Error: No project loaded."));
        return;
    }
    
    QString context = collectCodeContext("isPremium|isProUser|isPurchased|checkLicense|BillingClient|InAppPurchase");
    
    QString prompt = QString(
        "You are an Android reverse engineering expert specializing in app modification.\n\n"
        "Analyze this smali code and identify ALL premium/license check methods:\n\n"
        "%1\n\n"
        "For each method found:\n"
        "1. Identify the exact file path and method signature\n"
        "2. Determine the return type (boolean, int, object)\n"
        "3. Provide the EXACT smali patch to bypass the check:\n"
        "   - For boolean methods: change return to always true\n"
        "   - For license checks: skip verification\n"
        "   - For purchase checks: force purchased state\n\n"
        "Format as a list with file path, method name, and complete smali patch code."
    ).arg(context.left(20000));
    
    askAI(prompt, [this](const QString &response) {
        logMessage(tr("AI Analysis complete:"));
        m_AiLog->append("<div style='color: #7ee787; white-space: pre-wrap;'>" + response + "</div>");
    });
}

QString AppModStudio::collectCodeContext(const QString &pattern)
{
    QString context;
    QRegularExpression regex(pattern, QRegularExpression::CaseInsensitiveOption);
    
    QDirIterator it(m_ProjectPath, {"*.smali", "*.java"}, QDir::Files, QDirIterator::Subdirectories);
    
    while (it.hasNext() && context.length() < 25000) {
        QString filePath = it.next();
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        
        QString content = QString::fromUtf8(file.readAll());
        file.close();
        
        if (regex.match(content).hasMatch()) {
            QString relativePath = filePath.mid(m_ProjectPath.length() + 1);
            QStringList lines = content.split('\n');
            
            for (int i = 0; i < lines.size(); ++i) {
                if (regex.match(lines[i]).hasMatch()) {
                    context += "\n--- " + relativePath + " (line " + QString::number(i + 1) + ") ---\n";
                    int start = qMax(0, i - 10);
                    int end = qMin(lines.size() - 1, i + 20);
                    for (int j = start; j <= end; ++j) {
                        context += lines[j] + "\n";
                    }
                }
            }
        }
    }
    
    return context;
}

void AppModStudio::askAI(const QString &prompt, std::function<void(const QString&)> callback)
{
    QSettings settings;
    QString key = settings.value("ai_api_key").toString();
    QString model = settings.value("ai_model", "gemini-2.0-flash-exp").toString();
    
    if (key.isEmpty()) {
        logMessage(tr("Error: API Key not set in Settings."));
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
            logMessage(tr("AI request failed: %1").arg(reply->errorString()));
        }
        reply->deleteLater();
    });
}
