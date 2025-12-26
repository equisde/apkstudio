#include "gamemodtools.h"
#include <QApplication>
#include <QBoxLayout>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QSplitter>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTextStream>
#include <QTimer>
#include <QToolBar>

// ============== Game Engine Detector ==============

GameEngineDetector::Engine GameEngineDetector::detectEngine(const QString &projectPath)
{
    QDir dir(projectPath);
    // Búsqueda exhaustiva en carpetas comunes de proyectos decompilados
    if (dir.exists("lib/arm64-v8a/libunity.so") || dir.exists("lib/armeabi-v7a/libunity.so") || dir.exists("assets/bin/Data")) return Unity;
    if (dir.exists("lib/arm64-v8a/libflutter.so") || dir.exists("assets/flutter_assets")) return Flutter;
    if (dir.exists("lib/arm64-v8a/libUE4.so") || dir.exists("assets/UE4Game")) return UnrealEngine;
    if (dir.exists("lib/arm64-v8a/libcocos2dcpp.so") || dir.exists("assets/src")) return Cocos2dx;
    return NativeAndroid;
}

QString GameEngineDetector::engineName(Engine engine)
{
    switch (engine) {
        case Unity: return "Unity (IL2CPP/Mono)";
        case UnrealEngine: return "Unreal Engine";
        case Cocos2dx: return "Cocos2d-x";
        case Flutter: return "Flutter (Dart)";
        case NativeAndroid: return "Native Android";
        default: return "Unknown";
    }
}

// ============== Game Mod Studio Implementation ==============

GameModStudio::GameModStudio(const QString &projectPath, QWidget *parent)
    : QWidget(parent), m_ProjectPath(projectPath)
{
    m_NetworkManager = new QNetworkAccessManager(this);
    setupUI();
    detectEngine();
}

void GameModStudio::setupUI()
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);

    m_EngineBadge = new QLabel(tr("Detecting..."));
    m_EngineBadge->setStyleSheet("background-color: #238636; color: white; padding: 8px 15px; border-radius: 12px; font-weight: bold;");
    layout->addWidget(m_EngineBadge);

    auto heroLayout = new QHBoxLayout();
    m_DownloadToolsBtn = new QPushButton(tr("📥 Setup Tools"));
    m_RunDumperBtn = new QPushButton(tr("🔧 Run Dumper"));
    m_AnalyzeBtn = new QPushButton(tr("🤖 AI Analyze"));
    
    m_DownloadToolsBtn->setStyleSheet("QPushButton { background-color: #21262d; border: 1px solid #30363d; padding: 10px; border-radius: 6px; }");
    m_RunDumperBtn->setStyleSheet("QPushButton { background-color: #21262d; border: 1px solid #388bfd; color: #58a6ff; padding: 10px; border-radius: 6px; }");
    m_AnalyzeBtn->setStyleSheet("QPushButton { background-color: #238636; color: white; padding: 10px; border-radius: 6px; border: none; }");

    heroLayout->addWidget(m_DownloadToolsBtn);
    heroLayout->addWidget(m_RunDumperBtn);
    heroLayout->addWidget(m_AnalyzeBtn);
    layout->addLayout(heroLayout);

    m_Progress = new QProgressBar();
    m_Progress->setVisible(false);
    layout->addWidget(m_Progress);

    auto splitter = new QSplitter(Qt::Vertical);
    m_AIResponseView = new QTextBrowser();
    m_AIResponseView->setStyleSheet("background-color: #0d1117; color: #d1d5da; font-family: 'Consolas', monospace;");
    m_LogView = new QPlainTextEdit();
    m_LogView->setReadOnly(true);
    m_LogView->setStyleSheet("background-color: #010409; color: #7ee787; font-family: 'Consolas', monospace;");
    
    splitter->addWidget(m_AIResponseView);
    splitter->addWidget(m_LogView);
    layout->addWidget(splitter);

    connect(m_DownloadToolsBtn, &QPushButton::clicked, this, &GameModStudio::downloadTools);
    connect(m_RunDumperBtn, &QPushButton::clicked, this, &GameModStudio::runDumper);
    connect(m_AnalyzeBtn, &QPushButton::clicked, this, &GameModStudio::analyzeWithAI);
}

void GameModStudio::detectEngine()
{
    m_DetectedEngine = GameEngineDetector::detectEngine(m_ProjectPath);
    updateEngineUI();
    logMessage("Engine identified: " + GameEngineDetector::engineName(m_DetectedEngine), "success");
}

void GameModStudio::updateEngineUI()
{
    m_EngineBadge->setText("🎮 " + GameEngineDetector::engineName(m_DetectedEngine));
    m_RunDumperBtn->setEnabled(m_DetectedEngine == GameEngineDetector::Unity);
}

void GameModStudio::downloadTools()
{
    logMessage("Searching for latest modding tools...", "info");
    m_Progress->setVisible(true);
    m_Progress->setRange(0, 0);

    // URL real de Il2CppDumper (Ejemplo de release estable)
    QString dumperUrl = "https://github.com/Perfare/Il2CppDumper/releases/download/v6.7.40/Il2CppDumper-v6.7.40.zip";
    QString toolsDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/tools";
    QDir().mkpath(toolsDir);
    QString savePath = toolsDir + "/Il2CppDumper.zip";

    QNetworkRequest req(QUrl(dumperUrl));
    QNetworkReply *reply = m_NetworkManager->get(req);

    connect(reply, &QNetworkReply::finished, this, [=]() {
        m_Progress->setVisible(false);
        if (reply->error() == QNetworkReply::NoError) {
            QFile f(savePath);
            if (f.open(QFile::WriteOnly)) {
                f.write(reply->readAll());
                f.close();
                logMessage("Tools downloaded successfully. Stored in /tools/", "success");
                
                // Configurar automáticamente la ruta en Settings
                QSettings settings;
                settings.setValue("il2cpp_dumper_exe", toolsDir + "/Il2CppDumper.exe");
                settings.sync();
                
                QMessageBox::information(this, "Success", "Modding tools are ready to use.");
            }
        } else {
            logMessage("Download failed: " + reply->errorString(), "error");
        }
        reply->deleteLater();
    });
}

void GameModStudio::runDumper()
{
    QSettings settings;
    QString exe = settings.value("il2cpp_dumper_exe").toString();
    
    if (exe.isEmpty() || !QFile::exists(exe)) {
        logMessage("Dumper not found. Please click 'Setup Tools' first.", "error");
        return;
    }

    logMessage("Starting IL2CPP Dumper...", "info");
    
    // Rutas comunes en proyectos APK Studio
    QString lib = m_ProjectPath + "/lib/arm64-v8a/libil2cpp.so";
    QString meta = m_ProjectPath + "/assets/bin/Data/Managed/Metadata/global-metadata.dat";

    if (!QFile::exists(lib) || !QFile::exists(meta)) {
        logMessage("Missing binary targets (libil2cpp.so or global-metadata.dat)", "error");
        return;
    }

    QProcess *p = new QProcess(this);
    p->start(exe, {lib, meta, m_ProjectPath + "/dump/"});
    connect(p, &QProcess::finished, this, [=]() {
        logMessage("Dump finished! Check /dump/ directory.", "success");
        p->deleteLater();
    });
}

void GameModStudio::analyzeWithAI()
{
    logMessage("AI dissecting project structure...", "info");
    m_AIResponseView->setHtml("<i>Analyzing...</i>");

    QString prompt = QString("You are a master modder. Analyze this %1 project at %2. Suggest modding patches.")
                     .arg(GameEngineDetector::engineName(m_DetectedEngine), m_ProjectPath);

    askAI(prompt, [this](const QString &res) {
        m_AIResponseView->setMarkdown(res);
        logMessage("AI analysis complete.", "success");
    });
}

void GameModStudio::askAI(const QString &prompt, std::function<void(const QString&)> callback)
{
    QSettings settings;
    QString key = settings.value("ai_api_key").toString();
    QString model = settings.value("ai_model", "gemini-2.0-flash-exp").toString();

    if (key.isEmpty()) {
        logMessage("API Key missing in Settings!", "error");
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

    QNetworkRequest req(QUrl(QString("https://generativelanguage.googleapis.com/v1beta/models/%1:generateContent?key=%2").arg(model, key)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = m_NetworkManager->post(req, QJsonDocument(root).toJson());
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QString text = doc.object()["candidates"].toArray()[0].toObject()["content"].toObject()["parts"].toArray()[0].toObject()["text"].toString();
            callback(text);
        } else {
            logMessage("AI Error: " + reply->errorString(), "error");
        }
        reply->deleteLater();
    });
}

void GameModStudio::logMessage(const QString &msg, const QString &type)
{
    QString color = (type == "success") ? "#7ee787" : (type == "error") ? "#f85149" : "#8b949e";
    m_LogView->appendHtml(QString("<span style='color: %1;'>[%2] %3</span>")
        .arg(color, QDateTime::currentDateTime().toString("HH:mm:ss"), msg));
}

void GameModStudio::applyMod() {}
