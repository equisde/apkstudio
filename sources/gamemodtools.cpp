#include "gamemodtools.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QHeaderView>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QDateTime>
#include <QTimer>
#include <QStandardPaths>

GameEngineDetector::Engine GameEngineDetector::detectEngine(const QString &projectPath) {
    QDir dir(projectPath);
    // Búsqueda más profunda en el árbol del proyecto
    if (dir.exists("lib/arm64-v8a/libunity.so") || dir.exists("assets/bin/Data")) return Unity;
    if (dir.exists("lib/arm64-v8a/libflutter.so") || dir.exists("assets/flutter_assets")) return Flutter;
    if (dir.exists("lib/arm64-v8a/libUE4.so") || dir.exists("assets/UE4Game")) return UnrealEngine;
    if (dir.exists("lib/arm64-v8a/libcocos2dcpp.so") || dir.exists("assets/src")) return Cocos2dx;
    return NativeAndroid;
}

QString GameEngineDetector::engineName(Engine engine) {
    switch (engine) {
        case Unity: return "Unity (IL2CPP/Mono)";
        case Flutter: return "Flutter (Dart)";
        case UnrealEngine: return "Unreal Engine";
        case Cocos2dx: return "Cocos2d-x";
        case NativeAndroid: return "Native Android";
        default: return "Unknown Engine";
    }
}

GameModStudio::GameModStudio(const QString &projectPath, QWidget *parent)
    : QWidget(parent), m_ProjectPath(projectPath) {
    m_NetworkManager = new QNetworkAccessManager(this);
    setupUI();
    detectEngine();
}

void GameModStudio::setupUI() {
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);

    m_EngineBadge = new QLabel(tr("Detecting Engine..."));
    m_EngineBadge->setStyleSheet("background-color: #238636; color: white; padding: 5px; border-radius: 4px; font-weight: bold;");
    layout->addWidget(m_EngineBadge);

    auto btnLayout = new QHBoxLayout();
    m_DownloadToolsBtn = new QPushButton(tr("📥 Setup Tools"));
    m_RunDumperBtn = new QPushButton(tr("🔧 Run Dumper"));
    m_AnalyzeBtn = new QPushButton(tr("🤖 AI Analyze"));
    btnLayout->addWidget(m_DownloadToolsBtn);
    btnLayout->addWidget(m_RunDumperBtn);
    btnLayout->addWidget(m_AnalyzeBtn);
    layout->addLayout(btnLayout);

    m_Progress = new QProgressBar();
    m_Progress->setVisible(false);
    layout->addWidget(m_Progress);

    auto splitter = new QSplitter(Qt::Vertical);
    
    m_AIResponseView = new QTextBrowser();
    m_AIResponseView->setPlaceholderText(tr("AI Analysis will appear here..."));
    m_AIResponseView->setStyleSheet("background-color: #0d1117; color: #d1d5da; font-family: monospace;");
    
    m_LogView = new QPlainTextEdit();
    m_LogView->setReadOnly(true);
    m_LogView->setStyleSheet("background-color: #010409; color: #7ee787; font-family: monospace;");
    
    splitter->addWidget(m_AIResponseView);
    splitter->addWidget(m_LogView);
    layout->addWidget(splitter);

    connect(m_DownloadToolsBtn, &QPushButton::clicked, this, &GameModStudio::downloadTools);
    connect(m_RunDumperBtn, &QPushButton::clicked, this, &GameModStudio::runDumper);
    connect(m_AnalyzeBtn, &QPushButton::clicked, this, &GameModStudio::analyzeWithAI);
}

void GameModStudio::detectEngine() {
    m_DetectedEngine = GameEngineDetector::detectEngine(m_ProjectPath);
    updateEngineUI();
}

void GameModStudio::updateEngineUI() {
    m_EngineBadge->setText("🎮 " + GameEngineDetector::engineName(m_DetectedEngine));
    m_RunDumperBtn->setEnabled(m_DetectedEngine == GameEngineDetector::Unity);
}

void GameModStudio::runDumper() {
    logMessage("Locating Il2CppDumper binary...", "info");
    QSettings settings;
    QString exe = settings.value("il2cpp_dumper_exe").toString();
    
    if (exe.isEmpty() || !QFile::exists(exe)) {
        logMessage("Error: Il2CppDumper not configured in Settings.", "error");
        return;
    }

    QString libPath = m_ProjectPath + "/lib/arm64-v8a/libil2cpp.so";
    QString metaPath = m_ProjectPath + "/assets/bin/Data/Managed/Metadata/global-metadata.dat";

    logMessage("Running Dumper on: " + libPath, "info");
    
    QProcess *proc = new QProcess(this);
    proc->start(exe, {libPath, metaPath, m_ProjectPath + "/dump/"});
    connect(proc, &QProcess::finished, this, [=]() {
        logMessage("Dump completed. Files saved to /dump/", "success");
        proc->deleteLater();
    });
}

void GameModStudio::askAI(const QString &prompt, std::function<void(const QString&)> callback) {
    QSettings settings;
    QString key = settings.value("ai_api_key").toString();
    QString model = settings.value("ai_model", "gemini-2.0-flash-exp").toString(); // Valor por defecto si no hay uno
    
    if (key.isEmpty()) {
        logMessage("Error: API Key is missing.", "error");
        return;
    }

    logMessage("Calling AI using model: " + model, "info");

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

    QNetworkRequest req(QUrl(QString("https://generativelanguage.googleapis.com/v1beta/models/%1:generateContent?key=%2").arg(model, key)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = m_NetworkManager->post(req, QJsonDocument(root).toJson());
    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QString text = doc.object()["candidates"].toArray()[0].toObject()["content"].toObject()["parts"].toArray()[0].toObject()["text"].toString();
            callback(text);
        } else {
            logMessage("AI Error: " + reply->errorString(), "error");
            if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 429) {
                logMessage("Tip: Rate limit reached. Try again in a minute or check your quota.", "warning");
            }
        }
        reply->deleteLater();
    });
}

void GameModStudio::logMessage(const QString &msg, const QString &type) {
    QString color = (type == "success") ? "#7ee787" : (type == "error") ? "#f85149" : "#8b949e";
    m_LogView->appendHtml(QString("<span style='color: %1;'>[%2] %3</span>")
        .arg(color, QDateTime::currentDateTime().toString("HH:mm:ss"), msg));
}

void GameModStudio::downloadTools() { logMessage("Scanning for missing binaries...", "info"); }
void GameModStudio::analyzeWithAI() { 
    askAI("Analyze this game project context and suggest modding points.", [this](const QString& res) {
        m_AIResponseView->setMarkdown(res);
    });
}
void GameModStudio::applyMod() {}

// Stubs
QString GameModToolDownloader::getToolsDirectory() { return ""; }
QString GameModToolDownloader::getToolExecutable(const QString &toolName) { return ""; }