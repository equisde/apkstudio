#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QScrollBar>
#include <QSettings>
#include <QTextStream>
#include <QVBoxLayout>
#include "aiconsolewidget.h"

AIConsoleWidget::AIConsoleWidget(QWidget *parent)
    : QWidget(parent), m_CurrentReply(nullptr)
{
    m_NetworkManager = new QNetworkAccessManager(this);
    
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    
    // Header with buttons
    auto headerLayout = new QHBoxLayout();
    auto titleLabel = new QLabel(tr("🤖 AI Assistant"), this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 13px;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    
    m_AnalyzeButton = new QPushButton(tr("Analyze Project"), this);
    m_AnalyzeButton->setIcon(QIcon(":/icons/icons8/icons8-search-48.png"));
    connect(m_AnalyzeButton, &QPushButton::clicked, this, &AIConsoleWidget::analyzeProject);
    headerLayout->addWidget(m_AnalyzeButton);
    
    m_ClearButton = new QPushButton(tr("Clear"), this);
    connect(m_ClearButton, &QPushButton::clicked, this, &AIConsoleWidget::clear);
    headerLayout->addWidget(m_ClearButton);
    
    layout->addLayout(headerLayout);
    
    // Console output
    m_OutputConsole = new QPlainTextEdit(this);
    m_OutputConsole->setReadOnly(true);
    m_OutputConsole->setPlaceholderText(tr("AI responses will appear here...\n\nConfigure your API key in Settings > AI Assistant to get started."));
    
    // Modern monospace font
    QFont font;
#ifdef Q_OS_WIN
    font.setFamily("Cascadia Code");
#elif defined(Q_OS_MACOS)
    font.setFamily("SF Mono");
#else
    font.setFamily("JetBrains Mono");
#endif
    font.setPointSize(10);
    font.setStyleHint(QFont::Monospace);
    m_OutputConsole->setFont(font);
    
    layout->addWidget(m_OutputConsole, 1);
    
    // Input area
    auto inputLayout = new QHBoxLayout();
    m_InputLine = new QLineEdit(this);
    m_InputLine->setPlaceholderText(tr("Ask AI about this project... (e.g., 'find security issues', 'explain MainActivity')"));
    m_InputLine->setFont(font);
    connect(m_InputLine, &QLineEdit::returnPressed, this, &AIConsoleWidget::handleSendMessage);
    inputLayout->addWidget(m_InputLine);
    
    m_SendButton = new QPushButton(tr("Send"), this);
    m_SendButton->setDefault(true);
    connect(m_SendButton, &QPushButton::clicked, this, &AIConsoleWidget::handleSendMessage);
    inputLayout->addWidget(m_SendButton);
    
    layout->addLayout(inputLayout);
    
    // Check if AI is enabled
    QSettings settings;
    bool enabled = settings.value("ai_enabled", false).toBool();
    QString apiKey = settings.value("ai_api_key").toString();
    
    if (!enabled || apiKey.isEmpty()) {
        m_AnalyzeButton->setEnabled(false);
        m_SendButton->setEnabled(false);
        m_InputLine->setEnabled(false);
        appendSystemMessage(tr("AI Assistant is not configured. Go to Settings > AI Assistant to enable."));
    }
}

void AIConsoleWidget::setProjectPath(const QString &path)
{
    m_ProjectPath = path;
    m_CurrentProjectContext.clear();
    
    QSettings settings;
    bool autoAnalyze = settings.value("ai_auto_analyze", true).toBool();
    bool enabled = settings.value("ai_enabled", false).toBool();
    QString apiKey = settings.value("ai_api_key").toString();
    
    if (enabled && !apiKey.isEmpty() && autoAnalyze && !path.isEmpty()) {
        analyzeProject();
    }
}

void AIConsoleWidget::analyzeProject()
{
    if (m_ProjectPath.isEmpty()) {
        appendSystemMessage(tr("No project loaded. Open an APK project first."));
        return;
    }
    
    emit analysisStarted();
    appendSystemMessage(tr("🔍 Analyzing project: %1").arg(m_ProjectPath));
    
    // Build project context
    m_CurrentProjectContext = buildProjectContext();
    
    // Send analysis request
    QString prompt = tr("Analyze this Android APK project comprehensively. Provide:\n\n"
                        "1. **Overview**: Package name, version, target SDK\n"
                        "2. **Permissions Analysis**: List all permissions and their risk levels\n"
                        "3. **Security Assessment**: Identify potential vulnerabilities\n"
                        "4. **Code Structure**: Main components (Activities, Services, Receivers)\n"
                        "5. **Notable Findings**: Any interesting or suspicious patterns\n"
                        "6. **Recommendations**: Suggestions for modification or analysis\n\n"
                        "Format the output in clear markdown sections.");
    
    sendToAI(prompt);
}

QString AIConsoleWidget::buildProjectContext()
{
    QString context;
    QTextStream stream(&context);
    
    stream << "=== Android Project Analysis Context ===\n\n";
    stream << "Project Path: " << m_ProjectPath << "\n\n";
    
    // Read AndroidManifest.xml
    QString manifestPath = m_ProjectPath + "/AndroidManifest.xml";
    QFile manifestFile(manifestPath);
    if (manifestFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        stream << "=== AndroidManifest.xml ===\n";
        stream << manifestFile.readAll() << "\n\n";
        manifestFile.close();
    }
    
    // Read apktool.yml
    QString apktoolPath = m_ProjectPath + "/apktool.yml";
    QFile apktoolFile(apktoolPath);
    if (apktoolFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        stream << "=== apktool.yml ===\n";
        stream << apktoolFile.readAll() << "\n\n";
        apktoolFile.close();
    }
    
    // List smali directories structure
    stream << "=== Smali Code Structure ===\n";
    QDir smaliDir(m_ProjectPath + "/smali");
    if (smaliDir.exists()) {
        QDirIterator it(smaliDir.absolutePath(), QStringList() << "*.smali", QDir::Files, QDirIterator::Subdirectories);
        int count = 0;
        QStringList samples;
        while (it.hasNext() && count < 50) {
            QString filePath = it.next();
            QString relativePath = filePath.mid(m_ProjectPath.length() + 1);
            samples << relativePath;
            count++;
        }
        stream << "Total smali files found (first 50): " << count << "\n";
        for (const QString &s : samples) {
            stream << "  - " << s << "\n";
        }
    }
    stream << "\n";
    
    // List resources
    stream << "=== Resources Structure ===\n";
    QDir resDir(m_ProjectPath + "/res");
    if (resDir.exists()) {
        QStringList resDirs = resDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &dir : resDirs) {
            QDir subDir(resDir.absoluteFilePath(dir));
            int fileCount = subDir.entryList(QDir::Files).count();
            stream << "  - " << dir << "/ (" << fileCount << " files)\n";
        }
    }
    stream << "\n";
    
    // Read strings.xml for app info
    QString stringsPath = m_ProjectPath + "/res/values/strings.xml";
    QFile stringsFile(stringsPath);
    if (stringsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = stringsFile.readAll();
        if (content.length() > 5000) {
            content = content.left(5000) + "\n... (truncated)";
        }
        stream << "=== strings.xml (partial) ===\n";
        stream << content << "\n\n";
        stringsFile.close();
    }
    
    return context;
}

void AIConsoleWidget::appendMessage(const QString &role, const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString formattedRole = role.toUpper();
    
    QString html;
    if (role == "user") {
        html = QString("[%1] 👤 YOU:\n%2\n\n").arg(timestamp, message);
    } else if (role == "assistant") {
        html = QString("[%1] 🤖 AI:\n%2\n\n").arg(timestamp, message);
    } else {
        html = QString("[%1] %2:\n%3\n\n").arg(timestamp, formattedRole, message);
    }
    
    m_OutputConsole->appendPlainText(html);
    m_OutputConsole->verticalScrollBar()->setValue(m_OutputConsole->verticalScrollBar()->maximum());
}

void AIConsoleWidget::appendSystemMessage(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_OutputConsole->appendPlainText(QString("[%1] ⚙️ SYSTEM: %2\n").arg(timestamp, message));
    m_OutputConsole->verticalScrollBar()->setValue(m_OutputConsole->verticalScrollBar()->maximum());
}

void AIConsoleWidget::clear()
{
    m_OutputConsole->clear();
    m_ConversationHistory.clear();
}

void AIConsoleWidget::handleSendMessage()
{
    QString message = m_InputLine->text().trimmed();
    if (message.isEmpty()) return;
    
    m_InputLine->clear();
    appendMessage("user", message);
    sendToAI(message);
}

void AIConsoleWidget::sendToAI(const QString &message)
{
    QSettings settings;
    QString apiKey = settings.value("ai_api_key").toString();
    
    if (apiKey.isEmpty()) {
        appendSystemMessage(tr("API key not configured. Go to Settings > AI Assistant."));
        return;
    }
    
    m_SendButton->setEnabled(false);
    m_AnalyzeButton->setEnabled(false);
    m_InputLine->setEnabled(false);
    appendSystemMessage(tr("Thinking..."));
    
    QString endpoint = getApiEndpoint();
    QByteArray body = buildRequestBody(message);
    
    QNetworkRequest request;
    request.setUrl(QUrl(endpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QString provider = settings.value("ai_provider", "gemini").toString();
    if (provider == "gemini") {
        // Gemini uses API key in URL
    } else if (provider == "openai" || provider == "copilot") {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());
    } else if (provider == "anthropic") {
        request.setRawHeader("x-api-key", apiKey.toUtf8());
        request.setRawHeader("anthropic-version", "2023-06-01");
    }
    
    m_CurrentReply = m_NetworkManager->post(request, body);
    connect(m_CurrentReply, &QNetworkReply::finished, this, &AIConsoleWidget::handleApiResponse);
    connect(m_CurrentReply, &QNetworkReply::errorOccurred, this, &AIConsoleWidget::handleApiError);
}

QString AIConsoleWidget::getApiEndpoint()
{
    QSettings settings;
    QString provider = settings.value("ai_provider", "gemini").toString();
    QString model = settings.value("ai_model", "gemini-2.0-flash-exp").toString();
    QString apiKey = settings.value("ai_api_key").toString();
    
    if (provider == "gemini") {
        return QString("https://generativelanguage.googleapis.com/v1beta/models/%1:generateContent?key=%2")
            .arg(model, apiKey);
    } else if (provider == "openai" || provider == "copilot") {
        return "https://api.openai.com/v1/chat/completions";
    } else if (provider == "anthropic") {
        return "https://api.anthropic.com/v1/messages";
    }
    
    return QString();
}

QByteArray AIConsoleWidget::buildRequestBody(const QString &message)
{
    QSettings settings;
    QString provider = settings.value("ai_provider", "gemini").toString();
    QString model = settings.value("ai_model", "gemini-2.0-flash-exp").toString();
    
    QJsonObject root;
    
    // Add project context to message if available
    QString fullMessage = message;
    if (!m_CurrentProjectContext.isEmpty()) {
        fullMessage = m_CurrentProjectContext + "\n\n=== USER REQUEST ===\n" + message;
    }
    
    if (provider == "gemini") {
        QJsonArray contents;
        QJsonObject content;
        QJsonArray parts;
        QJsonObject part;
        part["text"] = fullMessage;
        parts.append(part);
        content["parts"] = parts;
        contents.append(content);
        root["contents"] = contents;
        
        // Generation config
        QJsonObject genConfig;
        genConfig["temperature"] = 0.7;
        genConfig["maxOutputTokens"] = 8192;
        root["generationConfig"] = genConfig;
        
    } else if (provider == "openai" || provider == "copilot") {
        root["model"] = model;
        QJsonArray messages;
        QJsonObject systemMsg;
        systemMsg["role"] = "system";
        systemMsg["content"] = "You are an expert Android security researcher and reverse engineer. "
                               "Analyze APK projects, identify vulnerabilities, explain code, and suggest modifications. "
                               "Be thorough but concise. Format output in markdown.";
        messages.append(systemMsg);
        
        QJsonObject userMsg;
        userMsg["role"] = "user";
        userMsg["content"] = fullMessage;
        messages.append(userMsg);
        root["messages"] = messages;
        root["max_tokens"] = 4096;
        
    } else if (provider == "anthropic") {
        root["model"] = model;
        root["max_tokens"] = 4096;
        root["system"] = "You are an expert Android security researcher and reverse engineer. "
                         "Analyze APK projects, identify vulnerabilities, explain code, and suggest modifications.";
        QJsonArray messages;
        QJsonObject userMsg;
        userMsg["role"] = "user";
        userMsg["content"] = fullMessage;
        messages.append(userMsg);
        root["messages"] = messages;
    }
    
    return QJsonDocument(root).toJson();
}

void AIConsoleWidget::handleApiResponse()
{
    m_SendButton->setEnabled(true);
    m_AnalyzeButton->setEnabled(true);
    m_InputLine->setEnabled(true);
    
    if (!m_CurrentReply) return;
    
    if (m_CurrentReply->error() != QNetworkReply::NoError) {
        appendSystemMessage(tr("Error: %1").arg(m_CurrentReply->errorString()));
        m_CurrentReply->deleteLater();
        m_CurrentReply = nullptr;
        return;
    }
    
    QByteArray data = m_CurrentReply->readAll();
    parseResponse(data);
    
    m_CurrentReply->deleteLater();
    m_CurrentReply = nullptr;
}

void AIConsoleWidget::parseResponse(const QByteArray &data)
{
    QSettings settings;
    QString provider = settings.value("ai_provider", "gemini").toString();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        appendSystemMessage(tr("Invalid response format"));
        return;
    }
    
    QJsonObject root = doc.object();
    QString responseText;
    
    if (provider == "gemini") {
        QJsonArray candidates = root["candidates"].toArray();
        if (!candidates.isEmpty()) {
            QJsonObject candidate = candidates[0].toObject();
            QJsonObject content = candidate["content"].toObject();
            QJsonArray parts = content["parts"].toArray();
            if (!parts.isEmpty()) {
                responseText = parts[0].toObject()["text"].toString();
            }
        }
    } else if (provider == "openai" || provider == "copilot") {
        QJsonArray choices = root["choices"].toArray();
        if (!choices.isEmpty()) {
            responseText = choices[0].toObject()["message"].toObject()["content"].toString();
        }
    } else if (provider == "anthropic") {
        QJsonArray content = root["content"].toArray();
        if (!content.isEmpty()) {
            responseText = content[0].toObject()["text"].toString();
        }
    }
    
    if (responseText.isEmpty()) {
        appendSystemMessage(tr("Empty response from AI"));
        return;
    }
    
    appendMessage("assistant", responseText);
    
    // Save analysis to file if this was a project analysis
    if (!m_CurrentProjectContext.isEmpty()) {
        saveAnalysisToFile(responseText);
        m_CurrentProjectContext.clear(); // Clear after analysis
    }
}

void AIConsoleWidget::saveAnalysisToFile(const QString &analysis)
{
    if (m_ProjectPath.isEmpty()) return;
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString fileName = QString("AI_Analysis_%1.md").arg(timestamp);
    QString filePath = m_ProjectPath + "/" + fileName;
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "# AI Project Analysis\n\n";
        out << "**Generated:** " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n";
        out << "**Project:** " << m_ProjectPath << "\n\n";
        out << "---\n\n";
        out << analysis;
        file.close();
        
        appendSystemMessage(tr("📄 Analysis saved to: %1").arg(fileName));
        emit analysisComplete(filePath);
    }
}

void AIConsoleWidget::handleApiError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error)
    m_SendButton->setEnabled(true);
    m_AnalyzeButton->setEnabled(true);
    m_InputLine->setEnabled(true);
    
    if (m_CurrentReply) {
        appendSystemMessage(tr("Network error: %1").arg(m_CurrentReply->errorString()));
    }
}
