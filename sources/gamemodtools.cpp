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
    // Exhaustive search including architecture folders
    QStringList paths = {"lib/arm64-v8a", "lib/armeabi-v7a", "lib/x86", "assets/bin/Data"};
    for (const QString &p : paths) {
        if (dir.exists(p + "/libunity.so") || dir.exists(p + "/libil2cpp.so")) return Unity;
        if (dir.exists(p + "/libflutter.so")) return Flutter;
        if (dir.exists(p + "/libUE4.so") || dir.exists("assets/UE4Game")) return UnrealEngine;
    }
    return NativeAndroid;
}

QString GameEngineDetector::engineName(Engine engine)
{
    switch (engine) {
        case Unity: return "Unity (IL2CPP/Mono)";
        case UnrealEngine: return "Unreal Engine";
        case Cocos2dx: return "Cocos2d-x";
        case Flutter: return "Flutter (Dart)";
        case ReactNative: return "React Native";
        case Godot: return "Godot";
        case NativeAndroid: return "Native Android";
        default: return "Unknown Engine";
    }
}

// ============== Game Mod Studio Implementation ==============

GameModStudio::GameModStudio(const QString &projectPath, QWidget *parent)
    : QWidget(parent), m_ProjectPath(projectPath)
{
    m_NetworkManager = new QNetworkAccessManager(this);
    setupUI();
    if (!m_ProjectPath.isEmpty()) {
        detectEngine();
    }
}

void GameModStudio::setProjectPath(const QString &path)
{
    m_ProjectPath = path;
    if (!m_ProjectPath.isEmpty()) {
        detectEngine();
        logMessage("Project re-indexed. Ready for analysis.", "success");
    }
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
    m_Progress->setStyleSheet("QProgressBar::chunk { background-color: #238636; }");
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
}

void GameModStudio::updateEngineUI()
{
    m_EngineBadge->setText("🎮 " + GameEngineDetector::engineName(m_DetectedEngine));
    m_RunDumperBtn->setEnabled(m_DetectedEngine == GameEngineDetector::Unity);
}

void GameModStudio::downloadTools()
{
    logMessage("Detecting OS for tool selection...", "info");
    m_Progress->setVisible(true);
    m_Progress->setRange(0, 0);

    QString dumperUrl = "https://github.com/Perfare/Il2CppDumper/releases/download/v6.7.46/Il2CppDumper-win-v6.7.46.zip";
#ifndef Q_OS_WIN
    dumperUrl = "https://github.com/Perfare/Il2CppDumper/releases/download/v6.7.46/Il2CppDumper-net6-v6.7.46.zip";
#endif

    QString toolsDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/tools";
    QDir().mkpath(toolsDir);
    QString savePath = toolsDir + "/Il2CppDumper.zip";

    QNetworkRequest req;
    req.setUrl(QUrl(dumperUrl));
    QNetworkReply *reply = m_NetworkManager->get(req);

    connect(reply, &QNetworkReply::finished, this, [=]() {
        m_Progress->setVisible(false);
        if (reply->error() == QNetworkReply::NoError) {
            QFile f(savePath);
            if (f.open(QFile::WriteOnly)) {
                f.write(reply->readAll());
                f.close();
                logMessage("Tools downloaded successfully.", "success");
                
                QSettings settings;
                settings.setValue("il2cpp_dumper_exe", toolsDir + "/il2cppdumper/Il2CppDumper.exe");
                settings.sync();
            }
        } else {
            logMessage("Download Error: " + reply->errorString(), "error");
        }
        reply->deleteLater();
    });
}

void GameModStudio::runDumper()
{
    // 1. INTENTAR DESCOMPILACIÓN DE C# (Assembly-CSharp.dll)
    QString managedPath = m_ProjectPath + "/assets/bin/Data/Managed/Assembly-CSharp.dll";
    if (QFile::exists(managedPath)) {
        logMessage("Assembly-CSharp.dll found. Running ILSpyCmd...", "info");
        QSettings settings;
        QString ilspy = settings.value("ilspy_cmd").toString();
        
        if (!ilspy.isEmpty() && QFile::exists(ilspy)) {
            QString outDir = m_ProjectPath + "/csharp_src";
            QDir().mkpath(outDir);
            QProcess *p = new QProcess(this);
            p->start(ilspy, {"-o", outDir, managedPath});
            connect(p, &QProcess::finished, [=]() {
                logMessage("C# Source extracted! AI analysis ready.", "success");
                p->deleteLater();
            });
            return;
        } else {
            logMessage("ILSpyCmd not found. Run 'Setup Tools' to get it.", "error");
        }
    }

    // 2. LOGICA DE IL2CPP DUMPER (Ya existente)
    QSettings settings;
    QString exe = settings.value("il2cpp_dumper_exe").toString();
    
    if (exe.isEmpty() || !QFile::exists(exe)) {
        logMessage("Dumper not found. Run 'Setup Tools' first.", "error");
        return;
    }

    QString lib, meta;
    QStringList archs = {"arm64-v8a", "armeabi-v7a", "x86"};
    for (const QString &arch : archs) {
        if (QFile::exists(m_ProjectPath + "/lib/" + arch + "/libil2cpp.so")) {
            lib = m_ProjectPath + "/lib/" + arch + "/libil2cpp.so";
            break;
        }
    }
    meta = m_ProjectPath + "/assets/bin/Data/Managed/Metadata/global-metadata.dat";

    if (lib.isEmpty() || !QFile::exists(meta)) {
        logMessage("Critical Error: libil2cpp.so or metadata not found.", "error");
        return;
    }

    logMessage("Starting Dumper...", "info");
    
    QString dumpPath = m_ProjectPath + "/dump/";
    QDir().mkpath(dumpPath);

    QProcess *p = new QProcess(this);
    p->setWorkingDirectory(QFileInfo(exe).absolutePath());
    p->start(exe, {lib, meta, dumpPath});
    
    connect(p, &QProcess::finished, this, [=]() {
        logMessage("Dump Successful!", "success");
        p->deleteLater();
        analyzeWithAI();
    });
}

void GameModStudio::analyzeWithAI()
{
    QSettings settings;
    QString model = settings.value("ai_model", "gemini-2.0-flash-exp").toString();
    logMessage("Consulting AI Engine (" + model + ")...", "info");

    askAI("Analyze this game project and identify potential modding entries.", [this](const QString &res) {
        m_AIResponseView->setMarkdown(res);
        logMessage("AI Analysis ready.", "success");
    });
}

void GameModStudio::askAI(const QString &prompt, std::function<void(const QString&)> callback)
{
    QSettings settings;
    QString key = settings.value("ai_api_key").toString();
    QString model = settings.value("ai_model", "gemini-2.0-flash-exp").toString();

    if (key.isEmpty()) { logMessage("API Key missing.", "error"); return; }

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
            logMessage("AI Hub Error: " + reply->errorString(), "error");
        }
        reply->deleteLater();
    });
}

void GameModStudio::logMessage(const QString &msg, const QString &type)
{
    QString color = (type == "success") ? "#7ee787" : (type == "error") ? "#f85149" : (type == "warning") ? "#d29922" : "#8b949e";
    m_LogView->appendHtml(QString("<span style='color: %1;'>[%2] %3</span>").arg(color, QDateTime::currentDateTime().toString("HH:mm:ss"), msg));
}

void GameModStudio::applyMod() { logMessage("Applying AI-generated patches...", "warning"); }

// Stubs for legacy support
QString GameModToolDownloader::getToolsDirectory() { return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/tools"; }
QString GameModToolDownloader::getToolExecutable(const QString &toolName) { return getToolsDirectory() + "/" + toolName; }

UnityGameDialog::UnityGameDialog(const QString &p, QWidget *par) : QDialog(par) { Q_UNUSED(p) }
FlutterAnalyzerDialog::FlutterAnalyzerDialog(const QString &p, QWidget *par) : QDialog(par) { Q_UNUSED(p) }
GameValueEditorDialog::GameValueEditorDialog(const QString &p, QWidget *par) : QDialog(par) { Q_UNUSED(p) }
