#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSettings>
#include <QTextStream>
#include <QVBoxLayout>
#include "aiconsolewidget.h"

// CSS for the AI console
static const char* AI_CONSOLE_CSS = R"(
body {
    font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, sans-serif;
    font-size: 13px;
    line-height: 1.5;
    margin: 0;
    padding: 8px;
    background: #1e1e1e;
    color: #d4d4d4;
}
.message {
    margin-bottom: 16px;
    padding: 12px;
    border-radius: 8px;
}
.user-message {
    background: #264f78;
    border-left: 3px solid #569cd6;
}
.ai-message {
    background: #252526;
    border-left: 3px solid #4ec9b0;
}
.system-message {
    background: #3c3c3c;
    border-left: 3px solid #dcdcaa;
    font-size: 12px;
    color: #9cdcfe;
}
.role {
    font-weight: bold;
    margin-bottom: 6px;
    font-size: 11px;
    text-transform: uppercase;
    letter-spacing: 0.5px;
}
.user-role { color: #569cd6; }
.ai-role { color: #4ec9b0; }
.system-role { color: #dcdcaa; }
.timestamp {
    font-size: 10px;
    color: #808080;
    float: right;
}
.content { white-space: pre-wrap; }
code {
    font-family: 'Cascadia Code', 'JetBrains Mono', 'Fira Code', Consolas, monospace;
    background: #1e1e1e;
    padding: 2px 6px;
    border-radius: 4px;
    font-size: 12px;
}
pre {
    background: #1e1e1e;
    padding: 12px;
    border-radius: 6px;
    overflow-x: auto;
    border: 1px solid #3c3c3c;
}
pre code {
    background: none;
    padding: 0;
}
h1, h2, h3, h4 { color: #4ec9b0; margin: 16px 0 8px 0; }
h1 { font-size: 1.4em; border-bottom: 1px solid #3c3c3c; padding-bottom: 6px; }
h2 { font-size: 1.2em; }
h3 { font-size: 1.1em; }
ul, ol { margin: 8px 0; padding-left: 24px; }
li { margin: 4px 0; }
strong { color: #ce9178; }
em { color: #c586c0; }
a { color: #569cd6; }
hr { border: none; border-top: 1px solid #3c3c3c; margin: 16px 0; }
.warning { background: #4d3800; border-left-color: #cca700; }
.error { background: #4d1f1f; border-left-color: #f14c4c; }
.success { background: #1f4d1f; border-left-color: #4ec9b0; }
)";

AIConsoleWidget::AIConsoleWidget(QWidget *parent)
    : QWidget(parent), m_CurrentReply(nullptr), m_CliProcess(nullptr), 
      m_UseCliAgent(false), m_IsProcessingFileOps(false),
      m_NodeAvailable(false), m_GeminiCliAvailable(false), m_CopilotCliAvailable(false)
{
    m_NetworkManager = new QNetworkAccessManager(this);
    
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    // Header with buttons - VS Code style
    auto headerWidget = new QWidget(this);
    headerWidget->setStyleSheet("background: #252526; border-bottom: 1px solid #3c3c3c;");
    auto headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(12, 8, 12, 8);
    
    auto titleLabel = new QLabel(tr("AI ASSISTANT"), this);
    titleLabel->setStyleSheet("font-weight: 600; font-size: 11px; color: #cccccc; letter-spacing: 1px;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    
    // Mode selector (API vs CLI Agent)
    m_ModeCombo = new QComboBox(this);
    m_ModeCombo->addItem(tr("API Mode"), "api");
    m_ModeCombo->addItem(tr("CLI Agent"), "cli");
    m_ModeCombo->setStyleSheet(R"(
        QComboBox {
            background: #3c3c3c;
            color: #cccccc;
            border: 1px solid #3c3c3c;
            border-radius: 3px;
            padding: 4px 8px;
            font-size: 11px;
        }
        QComboBox:hover { border-color: #0e639c; }
        QComboBox::drop-down { border: none; }
    )");
    connect(m_ModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AIConsoleWidget::onModeChanged);
    headerLayout->addWidget(m_ModeCombo);
    
    // Dependencies button
    m_DepsButton = new QPushButton(tr("🔧"), this);
    m_DepsButton->setToolTip(tr("Check/Install Dependencies"));
    m_DepsButton->setFixedSize(28, 28);
    m_DepsButton->setStyleSheet(R"(
        QPushButton {
            background: transparent;
            color: #cccccc;
            border: 1px solid #3c3c3c;
            border-radius: 3px;
            font-size: 12px;
        }
        QPushButton:hover { background: #3c3c3c; }
    )");
    connect(m_DepsButton, &QPushButton::clicked, this, &AIConsoleWidget::checkDependencies);
    headerLayout->addWidget(m_DepsButton);
    
    m_AnalyzeButton = new QPushButton(tr("⚡ Analyze"), this);
    m_AnalyzeButton->setStyleSheet(R"(
        QPushButton {
            background: #0e639c;
            color: white;
            border: none;
            padding: 4px 12px;
            border-radius: 3px;
            font-size: 11px;
        }
        QPushButton:hover { background: #1177bb; }
        QPushButton:pressed { background: #0d5a8c; }
        QPushButton:disabled { background: #3c3c3c; color: #808080; }
    )");
    connect(m_AnalyzeButton, &QPushButton::clicked, this, &AIConsoleWidget::analyzeProject);
    headerLayout->addWidget(m_AnalyzeButton);
    
    m_ClearButton = new QPushButton(tr("Clear"), this);
    m_ClearButton->setStyleSheet(R"(
        QPushButton {
            background: transparent;
            color: #cccccc;
            border: 1px solid #3c3c3c;
            padding: 4px 12px;
            border-radius: 3px;
            font-size: 11px;
        }
        QPushButton:hover { background: #3c3c3c; }
    )");
    connect(m_ClearButton, &QPushButton::clicked, this, &AIConsoleWidget::clear);
    headerLayout->addWidget(m_ClearButton);
    
    layout->addWidget(headerWidget);
    
    // Console output - using QTextBrowser for HTML
    m_OutputConsole = new QTextBrowser(this);
    m_OutputConsole->setOpenExternalLinks(true);
    m_OutputConsole->setFrameStyle(QFrame::NoFrame);
    m_OutputConsole->setStyleSheet("background: #1e1e1e; border: none;");
    
    // Initialize HTML content
    m_HtmlContent = QString("<html><head><style>%1</style></head><body>").arg(AI_CONSOLE_CSS);
    m_HtmlContent += "<div class='message system-message'>";
    m_HtmlContent += "<div class='content'>Welcome to AI Assistant! Configure your API key in <strong>Settings → AI Assistant</strong> to get started.</div>";
    m_HtmlContent += "</div>";
    m_OutputConsole->setHtml(m_HtmlContent + "</body></html>");
    
    layout->addWidget(m_OutputConsole, 1);
    
    // Input area - VS Code style
    auto inputWidget = new QWidget(this);
    inputWidget->setStyleSheet("background: #252526; border-top: 1px solid #3c3c3c;");
    auto inputLayout = new QHBoxLayout(inputWidget);
    inputLayout->setContentsMargins(8, 8, 8, 8);
    inputLayout->setSpacing(8);
    
    m_InputLine = new QLineEdit(this);
    m_InputLine->setPlaceholderText(tr("Ask AI about this project..."));
    m_InputLine->setStyleSheet(R"(
        QLineEdit {
            background: #3c3c3c;
            border: 1px solid #3c3c3c;
            border-radius: 4px;
            padding: 8px 12px;
            color: #cccccc;
            font-size: 13px;
        }
        QLineEdit:focus {
            border-color: #0e639c;
        }
    )");
    connect(m_InputLine, &QLineEdit::returnPressed, this, &AIConsoleWidget::handleSendMessage);
    inputLayout->addWidget(m_InputLine);
    
    m_SendButton = new QPushButton(tr("→"), this);
    m_SendButton->setFixedSize(36, 36);
    m_SendButton->setStyleSheet(R"(
        QPushButton {
            background: #0e639c;
            color: white;
            border: none;
            border-radius: 4px;
            font-size: 16px;
            font-weight: bold;
        }
        QPushButton:hover { background: #1177bb; }
        QPushButton:pressed { background: #0d5a8c; }
        QPushButton:disabled { background: #3c3c3c; color: #808080; }
    )");
    connect(m_SendButton, &QPushButton::clicked, this, &AIConsoleWidget::handleSendMessage);
    inputLayout->addWidget(m_SendButton);
    
    layout->addWidget(inputWidget);
    
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
    m_FullProjectContext.clear();
    
    if (!path.isEmpty()) {
        // Build full project context for ongoing conversations
        m_FullProjectContext = buildFullProjectContext();
        appendSystemMessage(tr("📁 Project loaded: %1").arg(QDir(path).dirName()));
    }
    
    QSettings settings;
    bool autoAnalyze = settings.value("ai_auto_analyze", true).toBool();
    bool enabled = settings.value("ai_enabled", false).toBool();
    QString apiKey = settings.value("ai_api_key").toString();
    
    if (enabled && !apiKey.isEmpty() && autoAnalyze && !path.isEmpty()) {
        // Check if analysis already exists
        if (!hasExistingAnalysis()) {
            analyzeProject();
        } else {
            appendSystemMessage(tr("📄 Previous analysis found. Use 'Analyze Project' button to re-analyze."));
        }
    }
    
    // Detect environment for CLI agents
    detectEnvironment();
}

bool AIConsoleWidget::hasExistingAnalysis()
{
    if (m_ProjectPath.isEmpty()) return false;
    
    QDir projectDir(m_ProjectPath);
    QStringList filters;
    filters << "AI_Analysis_*.md";
    QStringList files = projectDir.entryList(filters, QDir::Files, QDir::Time);
    
    if (!files.isEmpty()) {
        // Check if the most recent analysis is less than 24 hours old
        QString latestFile = projectDir.absoluteFilePath(files.first());
        QFileInfo info(latestFile);
        QDateTime lastModified = info.lastModified();
        QDateTime now = QDateTime::currentDateTime();
        
        // If analysis is less than 24 hours old, consider it valid
        if (lastModified.secsTo(now) < 86400) {
            return true;
        }
    }
    return false;
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
    QString messageClass, roleClass, roleText;
    
    if (role == "user") {
        messageClass = "user-message";
        roleClass = "user-role";
        roleText = "YOU";
    } else if (role == "assistant") {
        messageClass = "ai-message";
        roleClass = "ai-role";
        roleText = "AI ASSISTANT";
    } else {
        messageClass = "system-message";
        roleClass = "system-role";
        roleText = role.toUpper();
    }
    
    QString formattedContent = markdownToHtml(message);
    
    QString html = QString(
        "<div class='message %1'>"
        "<span class='timestamp'>%2</span>"
        "<div class='role %3'>%4</div>"
        "<div class='content'>%5</div>"
        "</div>"
    ).arg(messageClass, timestamp, roleClass, roleText, formattedContent);
    
    m_HtmlContent += html;
    m_OutputConsole->setHtml(m_HtmlContent + "</body></html>");
    m_OutputConsole->verticalScrollBar()->setValue(m_OutputConsole->verticalScrollBar()->maximum());
}

void AIConsoleWidget::appendSystemMessage(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    
    QString html = QString(
        "<div class='message system-message'>"
        "<span class='timestamp'>%1</span>"
        "<div class='content'>%2</div>"
        "</div>"
    ).arg(timestamp, escapeHtml(message));
    
    m_HtmlContent += html;
    m_OutputConsole->setHtml(m_HtmlContent + "</body></html>");
    m_OutputConsole->verticalScrollBar()->setValue(m_OutputConsole->verticalScrollBar()->maximum());
}

void AIConsoleWidget::clear()
{
    m_HtmlContent = QString("<html><head><style>%1</style></head><body>").arg(AI_CONSOLE_CSS);
    m_OutputConsole->setHtml(m_HtmlContent + "</body></html>");
    m_ConversationHistory.clear();
}

QString AIConsoleWidget::escapeHtml(const QString &text)
{
    QString result = text;
    result.replace("&", "&amp;");
    result.replace("<", "&lt;");
    result.replace(">", "&gt;");
    result.replace("\n", "<br>");
    return result;
}

QString AIConsoleWidget::markdownToHtml(const QString &markdown)
{
    QString result = escapeHtml(markdown);
    
    // Code blocks: ```code```
    QRegularExpression codeBlockRe("```([a-z]*)\\n([\\s\\S]*?)```", QRegularExpression::MultilineOption);
    result.replace(codeBlockRe, "<pre><code>\\2</code></pre>");
    
    // Inline code: `code`
    result.replace(QRegularExpression("`([^`]+)`"), "<code>\\1</code>");
    
    // Headers
    result.replace(QRegularExpression("^##### (.+)$", QRegularExpression::MultilineOption), "<h5>\\1</h5>");
    result.replace(QRegularExpression("^#### (.+)$", QRegularExpression::MultilineOption), "<h4>\\1</h4>");
    result.replace(QRegularExpression("^### (.+)$", QRegularExpression::MultilineOption), "<h3>\\1</h3>");
    result.replace(QRegularExpression("^## (.+)$", QRegularExpression::MultilineOption), "<h2>\\1</h2>");
    result.replace(QRegularExpression("^# (.+)$", QRegularExpression::MultilineOption), "<h1>\\1</h1>");
    
    // Bold: **text**
    result.replace(QRegularExpression("\\*\\*(.+?)\\*\\*"), "<strong>\\1</strong>");
    
    // Italic: *text*
    result.replace(QRegularExpression("\\*(.+?)\\*"), "<em>\\1</em>");
    
    // Horizontal rule
    result.replace(QRegularExpression("^---$", QRegularExpression::MultilineOption), "<hr>");
    
    // Lists - simple handling
    result.replace(QRegularExpression("^- (.+)$", QRegularExpression::MultilineOption), "• \\1");
    result.replace(QRegularExpression("^\\d+\\. (.+)$", QRegularExpression::MultilineOption), "\\1");
    
    return result;
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
    QString systemPrompt = getSystemPrompt();
    
    // Add project context to message if available
    QString fullMessage = message;
    if (!m_CurrentProjectContext.isEmpty()) {
        fullMessage = m_CurrentProjectContext + "\n\n=== USER REQUEST ===\n" + message;
    } else if (!m_FullProjectContext.isEmpty()) {
        // Use cached full context for follow-up queries
        fullMessage = "Project context is loaded. User request:\n" + message;
    }
    
    if (provider == "gemini") {
        QJsonArray contents;
        
        // System instruction for Gemini
        QJsonObject systemInstruction;
        QJsonArray systemParts;
        QJsonObject systemPart;
        systemPart["text"] = systemPrompt;
        systemParts.append(systemPart);
        systemInstruction["parts"] = systemParts;
        root["systemInstruction"] = systemInstruction;
        
        QJsonObject content;
        QJsonArray parts;
        QJsonObject part;
        part["text"] = fullMessage;
        parts.append(part);
        content["parts"] = parts;
        content["role"] = "user";
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
        systemMsg["content"] = systemPrompt;
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
        root["system"] = systemPrompt;
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
    
    // Process file operations if present in response
    if (responseText.contains("<<<FILE_")) {
        processFileOperations(responseText);
    }
    
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

// ============================================================================
// File Operations - Allow AI to modify project files
// ============================================================================

void AIConsoleWidget::processFileOperations(const QString &response)
{
    // Parse AI response for file operation commands
    // Format: <<<FILE_CREATE:path>>>content<<<END_FILE>>>
    // Format: <<<FILE_MODIFY:path>>>old_content<<<REPLACE_WITH>>>new_content<<<END_FILE>>>
    // Format: <<<FILE_DELETE:path>>>
    // Format: <<<FILE_APPEND:path>>>content<<<END_FILE>>>
    
    QRegularExpression createRe("<<<FILE_CREATE:([^>]+)>>>([\\s\\S]*?)<<<END_FILE>>>");
    QRegularExpression modifyRe("<<<FILE_MODIFY:([^>]+)>>>([\\s\\S]*?)<<<REPLACE_WITH>>>([\\s\\S]*?)<<<END_FILE>>>");
    QRegularExpression deleteRe("<<<FILE_DELETE:([^>]+)>>>");
    QRegularExpression appendRe("<<<FILE_APPEND:([^>]+)>>>([\\s\\S]*?)<<<END_FILE>>>");
    
    int operationsCount = 0;
    
    // Process CREATE operations
    QRegularExpressionMatchIterator createIt = createRe.globalMatch(response);
    while (createIt.hasNext()) {
        QRegularExpressionMatch match = createIt.next();
        QString path = match.captured(1).trimmed();
        QString content = match.captured(2);
        if (createFile(path, content)) {
            operationsCount++;
            appendSystemMessage(tr("✅ Created: %1").arg(path));
        }
    }
    
    // Process MODIFY operations
    QRegularExpressionMatchIterator modifyIt = modifyRe.globalMatch(response);
    while (modifyIt.hasNext()) {
        QRegularExpressionMatch match = modifyIt.next();
        QString path = match.captured(1).trimmed();
        QString oldContent = match.captured(2);
        QString newContent = match.captured(3);
        if (modifyFile(path, oldContent, newContent)) {
            operationsCount++;
            appendSystemMessage(tr("✅ Modified: %1").arg(path));
        }
    }
    
    // Process DELETE operations
    QRegularExpressionMatchIterator deleteIt = deleteRe.globalMatch(response);
    while (deleteIt.hasNext()) {
        QRegularExpressionMatch match = deleteIt.next();
        QString path = match.captured(1).trimmed();
        if (deleteFile(path)) {
            operationsCount++;
            appendSystemMessage(tr("✅ Deleted: %1").arg(path));
        }
    }
    
    // Process APPEND operations
    QRegularExpressionMatchIterator appendIt = appendRe.globalMatch(response);
    while (appendIt.hasNext()) {
        QRegularExpressionMatch match = appendIt.next();
        QString path = match.captured(1).trimmed();
        QString content = match.captured(2);
        if (appendToFile(path, content)) {
            operationsCount++;
            appendSystemMessage(tr("✅ Appended to: %1").arg(path));
        }
    }
    
    if (operationsCount > 0) {
        appendSystemMessage(tr("📝 Completed %1 file operation(s)").arg(operationsCount));
    }
}

bool AIConsoleWidget::createFile(const QString &relativePath, const QString &content)
{
    if (m_ProjectPath.isEmpty()) return false;
    
    QString fullPath = m_ProjectPath + "/" + relativePath;
    QFileInfo info(fullPath);
    
    // Create parent directories if needed
    QDir().mkpath(info.absolutePath());
    
    QFile file(fullPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << content;
        file.close();
        emit fileCreated(fullPath);
        return true;
    }
    
    appendSystemMessage(tr("❌ Failed to create: %1").arg(relativePath));
    return false;
}

bool AIConsoleWidget::modifyFile(const QString &relativePath, const QString &oldContent, const QString &newContent)
{
    if (m_ProjectPath.isEmpty()) return false;
    
    QString fullPath = m_ProjectPath + "/" + relativePath;
    QFile file(fullPath);
    
    if (!file.exists()) {
        appendSystemMessage(tr("❌ File not found: %1").arg(relativePath));
        return false;
    }
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        appendSystemMessage(tr("❌ Cannot read: %1").arg(relativePath));
        return false;
    }
    
    QString content = file.readAll();
    file.close();
    
    // Perform replacement
    if (!content.contains(oldContent)) {
        appendSystemMessage(tr("⚠️ Content not found in: %1").arg(relativePath));
        return false;
    }
    
    content.replace(oldContent, newContent);
    
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        appendSystemMessage(tr("❌ Cannot write: %1").arg(relativePath));
        return false;
    }
    
    QTextStream out(&file);
    out << content;
    file.close();
    
    emit fileModified(fullPath);
    return true;
}

bool AIConsoleWidget::deleteFile(const QString &relativePath)
{
    if (m_ProjectPath.isEmpty()) return false;
    
    QString fullPath = m_ProjectPath + "/" + relativePath;
    QFile file(fullPath);
    
    if (!file.exists()) {
        appendSystemMessage(tr("⚠️ File already doesn't exist: %1").arg(relativePath));
        return true;
    }
    
    if (file.remove()) {
        return true;
    }
    
    appendSystemMessage(tr("❌ Failed to delete: %1").arg(relativePath));
    return false;
}

bool AIConsoleWidget::appendToFile(const QString &relativePath, const QString &content)
{
    if (m_ProjectPath.isEmpty()) return false;
    
    QString fullPath = m_ProjectPath + "/" + relativePath;
    QFile file(fullPath);
    
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
        appendSystemMessage(tr("❌ Cannot append to: %1").arg(relativePath));
        return false;
    }
    
    QTextStream out(&file);
    out << content;
    file.close();
    
    emit fileModified(fullPath);
    return true;
}

QString AIConsoleWidget::readFileContent(const QString &relativePath)
{
    if (m_ProjectPath.isEmpty()) return QString();
    
    QString fullPath = m_ProjectPath + "/" + relativePath;
    QFile file(fullPath);
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    
    QString content = file.readAll();
    file.close();
    return content;
}

// ============================================================================
// Full Project Context - Deep analysis of project structure
// ============================================================================

QString AIConsoleWidget::buildFullProjectContext()
{
    QString context;
    QTextStream stream(&context);
    
    stream << "=== FULL ANDROID PROJECT CONTEXT ===\n\n";
    stream << "Project Path: " << m_ProjectPath << "\n\n";
    
    // Read all key files
    QStringList keyFiles = {
        "AndroidManifest.xml",
        "apktool.yml",
        "res/values/strings.xml",
        "res/values/styles.xml",
        "res/values/colors.xml"
    };
    
    for (const QString &file : keyFiles) {
        QString content = readFileContent(file);
        if (!content.isEmpty()) {
            stream << "=== " << file << " ===\n";
            if (content.length() > 10000) {
                stream << content.left(10000) << "\n... (truncated)\n\n";
            } else {
                stream << content << "\n\n";
            }
        }
    }
    
    // List all smali files with main classes content
    stream << "=== SMALI CODE OVERVIEW ===\n";
    QDir smaliDir(m_ProjectPath + "/smali");
    if (smaliDir.exists()) {
        QDirIterator it(smaliDir.absolutePath(), QStringList() << "*.smali", 
                        QDir::Files, QDirIterator::Subdirectories);
        int count = 0;
        while (it.hasNext() && count < 100) {
            QString filePath = it.next();
            QString relativePath = filePath.mid(m_ProjectPath.length() + 1);
            
            // Include content of important classes
            if (relativePath.contains("MainActivity") || 
                relativePath.contains("Application") ||
                relativePath.contains("Activity") ||
                relativePath.contains("Service")) {
                
                QFile f(filePath);
                if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QString content = f.readAll();
                    f.close();
                    
                    stream << "\n--- " << relativePath << " ---\n";
                    if (content.length() > 5000) {
                        stream << content.left(5000) << "\n... (truncated)\n";
                    } else {
                        stream << content << "\n";
                    }
                }
            } else {
                stream << "  - " << relativePath << "\n";
            }
            count++;
        }
    }
    
    return context;
}

// ============================================================================
// CLI Agent Integration
// ============================================================================

void AIConsoleWidget::detectEnvironment()
{
    m_NodeAvailable = false;
    m_GeminiCliAvailable = false;
    m_CopilotCliAvailable = false;
    
    // Check for Node.js
    QProcess nodeCheck;
    nodeCheck.start("node", QStringList() << "--version");
    if (nodeCheck.waitForFinished(3000)) {
        QString output = nodeCheck.readAllStandardOutput().trimmed();
        if (output.startsWith("v")) {
            m_NodeAvailable = true;
            m_NodeVersion = output;
            m_NodePath = "node";
        }
    }
    
    // Check for Gemini CLI
    QProcess geminiCheck;
    geminiCheck.start("gemini", QStringList() << "--version");
    if (geminiCheck.waitForFinished(3000)) {
        m_GeminiCliAvailable = true;
    }
    
    // Check for GitHub Copilot CLI
    QProcess copilotCheck;
    copilotCheck.start("github-copilot-cli", QStringList() << "--version");
    if (copilotCheck.waitForFinished(3000)) {
        m_CopilotCliAvailable = true;
    }
    
    // Also check npx availability
    if (!m_GeminiCliAvailable && m_NodeAvailable) {
        QProcess npxCheck;
        npxCheck.start("npx", QStringList() << "@anthropic-ai/claude-cli" << "--version");
        // Just check if npx works
    }
}

void AIConsoleWidget::checkDependencies()
{
    detectEnvironment();
    
    QString status = tr("🔍 Environment Check:\n");
    status += QString("  • Node.js: %1\n").arg(m_NodeAvailable ? m_NodeVersion : tr("Not found"));
    status += QString("  • Gemini CLI: %1\n").arg(m_GeminiCliAvailable ? tr("Available") : tr("Not installed"));
    status += QString("  • Copilot CLI: %1\n").arg(m_CopilotCliAvailable ? tr("Available") : tr("Not installed"));
    
    if (!m_NodeAvailable) {
        status += tr("\n⚠️ Node.js is required for CLI agents. Install from nodejs.org");
    }
    
    appendSystemMessage(status);
}

void AIConsoleWidget::installDependencies()
{
    if (!m_NodeAvailable) {
        appendSystemMessage(tr("❌ Node.js is required. Please install from https://nodejs.org/"));
        return;
    }
    
    QSettings settings;
    QString provider = settings.value("ai_provider", "gemini").toString();
    
    QString package;
    if (provider == "gemini") {
        package = "@anthropic-ai/gemini-cli";
    } else if (provider == "copilot") {
        package = "@anthropic-ai/github-copilot-cli";
    }
    
    if (package.isEmpty()) {
        appendSystemMessage(tr("⚠️ No CLI agent available for current provider"));
        return;
    }
    
    appendSystemMessage(tr("📦 Installing %1...").arg(package));
    
    QProcess *installer = new QProcess(this);
    connect(installer, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, installer, package](int exitCode, QProcess::ExitStatus) {
        if (exitCode == 0) {
            appendSystemMessage(tr("✅ Successfully installed %1").arg(package));
            detectEnvironment();
        } else {
            appendSystemMessage(tr("❌ Failed to install %1").arg(package));
        }
        installer->deleteLater();
    });
    
    installer->start("npm", QStringList() << "install" << "-g" << package);
}

void AIConsoleWidget::startCliAgent()
{
    if (m_CliProcess) {
        stopCliAgent();
    }
    
    QString command = getCliAgentCommand();
    if (command.isEmpty()) {
        appendSystemMessage(tr("❌ No CLI agent available for current configuration"));
        return;
    }
    
    m_CliProcess = new QProcess(this);
    m_CliProcess->setWorkingDirectory(m_ProjectPath);
    
    connect(m_CliProcess, &QProcess::readyReadStandardOutput, this, &AIConsoleWidget::handleCliOutput);
    connect(m_CliProcess, &QProcess::readyReadStandardError, this, &AIConsoleWidget::handleCliError);
    connect(m_CliProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &AIConsoleWidget::handleCliFinished);
    
    QStringList args;
    args << "--interactive";
    
    m_CliProcess->start(command, args);
    
    if (m_CliProcess->waitForStarted(5000)) {
        appendSystemMessage(tr("🤖 CLI Agent started: %1").arg(command));
        m_UseCliAgent = true;
    } else {
        appendSystemMessage(tr("❌ Failed to start CLI agent"));
        delete m_CliProcess;
        m_CliProcess = nullptr;
    }
}

void AIConsoleWidget::stopCliAgent()
{
    if (m_CliProcess) {
        m_CliProcess->terminate();
        if (!m_CliProcess->waitForFinished(3000)) {
            m_CliProcess->kill();
        }
        delete m_CliProcess;
        m_CliProcess = nullptr;
        m_UseCliAgent = false;
        appendSystemMessage(tr("🛑 CLI Agent stopped"));
    }
}

void AIConsoleWidget::sendToCliAgent(const QString &message)
{
    if (!m_CliProcess || m_CliProcess->state() != QProcess::Running) {
        appendSystemMessage(tr("⚠️ CLI Agent not running"));
        return;
    }
    
    m_CliProcess->write((message + "\n").toUtf8());
}

QString AIConsoleWidget::getCliAgentCommand()
{
    QSettings settings;
    QString provider = settings.value("ai_provider", "gemini").toString();
    
    detectEnvironment();
    
    if (provider == "gemini" && m_GeminiCliAvailable) {
        return "gemini";
    } else if (provider == "copilot" && m_CopilotCliAvailable) {
        return "github-copilot-cli";
    }
    
    return QString();
}

void AIConsoleWidget::handleCliOutput()
{
    if (!m_CliProcess) return;
    
    QString output = m_CliProcess->readAllStandardOutput();
    if (!output.isEmpty()) {
        appendMessage("assistant", output);
    }
}

void AIConsoleWidget::handleCliError()
{
    if (!m_CliProcess) return;
    
    QString error = m_CliProcess->readAllStandardError();
    if (!error.isEmpty()) {
        appendSystemMessage(tr("CLI Error: %1").arg(error));
    }
}

void AIConsoleWidget::handleCliFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(status)
    appendSystemMessage(tr("CLI Agent exited with code %1").arg(exitCode));
    m_UseCliAgent = false;
    m_CliProcess = nullptr;
}

void AIConsoleWidget::onModeChanged(int index)
{
    Q_UNUSED(index)
    // Handle mode change between API and CLI agent
    if (m_ModeCombo) {
        QString mode = m_ModeCombo->currentData().toString();
        if (mode == "cli") {
            startCliAgent();
        } else {
            stopCliAgent();
        }
    }
}

// ============================================================================
// System Prompt for File Operations
// ============================================================================

QString AIConsoleWidget::getSystemPrompt()
{
    return tr(
        "You are an expert Android reverse engineer and security researcher assistant. "
        "You have full access to modify files in the current APK project.\n\n"
        
        "When the user requests modifications, you can perform file operations using these formats:\n\n"
        
        "To CREATE a new file:\n"
        "<<<FILE_CREATE:relative/path/to/file.ext>>>\n"
        "file content here\n"
        "<<<END_FILE>>>\n\n"
        
        "To MODIFY an existing file (replace content):\n"
        "<<<FILE_MODIFY:relative/path/to/file.ext>>>\n"
        "old content to find\n"
        "<<<REPLACE_WITH>>>\n"
        "new content to replace with\n"
        "<<<END_FILE>>>\n\n"
        
        "To DELETE a file:\n"
        "<<<FILE_DELETE:relative/path/to/file.ext>>>\n\n"
        
        "To APPEND to a file:\n"
        "<<<FILE_APPEND:relative/path/to/file.ext>>>\n"
        "content to append\n"
        "<<<END_FILE>>>\n\n"
        
        "Always explain what you're doing before performing operations. "
        "Be careful with smali code modifications - maintain proper syntax. "
        "When modifying AndroidManifest.xml, ensure XML validity."
    );
}
