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
    if (dir.exists("lib/armeabi-v7a/libunity.so") || dir.exists("lib/arm64-v8a/libunity.so") || dir.exists("assets/bin/Data")) return Unity;
    if (dir.exists("assets/UE4Game") || dir.exists("lib/arm64-v8a/libUE4.so")) return UnrealEngine;
    if (dir.exists("lib/arm64-v8a/libflutter.so") || dir.exists("assets/flutter_assets")) return Flutter;
    if (dir.exists("assets/index.android.bundle") || dir.exists("lib/arm64-v8a/libreactnativejni.so")) return ReactNative;
    if (dir.exists("lib/arm64-v8a/libcocos2dcpp.so") || dir.exists("assets/src")) return Cocos2dx;
    if (dir.exists("lib/arm64-v8a/libgodot_android.so") || dir.exists("assets/project.godot")) return Godot;
    return NativeAndroid;
}

QString GameEngineDetector::engineName(Engine engine)
{
    switch (engine) {
        case Unity: return "Unity";
        case UnrealEngine: return "Unreal Engine";
        case Cocos2dx: return "Cocos2d-x";
        case Flutter: return "Flutter";
        case ReactNative: return "React Native";
        case Godot: return "Godot";
        case NativeAndroid: return "Native Android";
        default: return "Unknown";
    }
}

QStringList GameEngineDetector::getEngineFiles(Engine engine)
{
    switch (engine) {
        case Unity: return {"libunity.so", "libil2cpp.so", "global-metadata.dat"};
        case UnrealEngine: return {"libUE4.so", "*.pak"};
        case Flutter: return {"libflutter.so", "libapp.so"};
        default: return {};
    }
}

bool GameEngineDetector::supportsDecompilation(Engine engine)
{
    return engine != Unknown && engine != NativeAndroid;
}

// ============== Tool Downloader ==============

void GameModToolDownloader::downloadAllTools(GameEngineDetector::Engine engine, QWidget *parent, std::function<void(int, int)> progress, std::function<void(bool)> completion)
{
    Q_UNUSED(engine) Q_UNUSED(parent) Q_UNUSED(progress)
    if (completion) completion(true);
}

QString GameModToolDownloader::getToolsDirectory()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/tools";
    QDir().mkpath(path);
    return path;
}

QString GameModToolDownloader::getToolExecutable(const QString &toolName)
{
    QDir dir(getToolsDirectory() + "/" + toolName.toLower());
    return dir.absoluteFilePath(toolName);
}

// ============== AI Game Mod Dialog ==============

AIGameModDialog::AIGameModDialog(const QString &projectPath, QWidget *parent)
    : QDialog(parent), m_ProjectPath(projectPath)
{
    setAttribute(Qt::WA_DeleteOnClose);
    m_NetworkManager = new QNetworkAccessManager(this);
    setupUI();
    detectEngine();
}

void AIGameModDialog::setupUI()
{
    setWindowTitle(tr("🎮 AI Game Mod Studio Pro"));
    setMinimumSize(1000, 700);
    setStyleSheet("QDialog { background-color: #0d1117; color: #c9d1d9; }");

    auto mainLayout = new QVBoxLayout(this);
    
    auto header = new QHBoxLayout();
    m_EngineBadge = new QLabel(tr("Detecting..."));
    m_EngineBadge->setStyleSheet("background-color: #238636; color: white; padding: 5px; border-radius: 5px;");
    header->addWidget(m_EngineBadge);
    header->addStretch();
    m_StatusLabel = new QLabel(tr("Ready"));
    header->addWidget(m_StatusLabel);
    mainLayout->addLayout(header);

    auto heroLayout = new QHBoxLayout();
    m_DownloadToolsBtn = new QPushButton(tr("Setup Tools"));
    m_RunDumperBtn = new QPushButton(tr("Run Dumper"));
    m_AnalyzeBtn = new QPushButton(tr("AI Analyze"));
    heroLayout->addWidget(m_DownloadToolsBtn);
    heroLayout->addWidget(m_RunDumperBtn);
    heroLayout->addWidget(m_AnalyzeBtn);
    mainLayout->addLayout(heroLayout);

    m_Progress = new QProgressBar();
    m_Progress->setVisible(false);
    mainLayout->addWidget(m_Progress);

    auto splitter = new QSplitter(Qt::Horizontal);
    
    auto leftWidget = new QWidget();
    auto leftLayout = new QVBoxLayout(leftWidget);
    m_ModTypeCombo = new QComboBox();
    m_ModTypeCombo->addItems({tr("Currency Mod"), tr("SSL Bypass"), tr("Anti-Cheat Bypass")});
    leftLayout->addWidget(m_ModTypeCombo);
    
    m_ModOptionsTree = new QTreeWidget();
    m_ModOptionsTree->setHeaderLabels({tr("Target"), tr("Status")});
    leftLayout->addWidget(m_ModOptionsTree);
    
    m_ApplyBtn = new QPushButton(tr("Apply Patch"));
    leftLayout->addWidget(m_ApplyBtn);
    splitter->addWidget(leftWidget);

    auto rightWidget = new QTabWidget();
    m_AIResponseView = new QTextBrowser();
    m_LogView = new QPlainTextEdit();
    m_LogView->setReadOnly(true);
    rightWidget->addTab(m_AIResponseView, tr("AI Mod Genius"));
    rightWidget->addTab(m_LogView, tr("Log"));
    splitter->addWidget(rightWidget);

    mainLayout->addWidget(splitter);

    connect(m_DownloadToolsBtn, &QPushButton::clicked, this, &AIGameModDialog::downloadTools);
    connect(m_RunDumperBtn, &QPushButton::clicked, this, &AIGameModDialog::runDumper);
    connect(m_AnalyzeBtn, &QPushButton::clicked, this, &AIGameModDialog::analyzeWithAI);
    connect(m_ApplyBtn, &QPushButton::clicked, this, &AIGameModDialog::applyMod);
}

void AIGameModDialog::detectEngine()
{
    m_DetectedEngine = GameEngineDetector::detectEngine(m_ProjectPath);
    updateEngineUI();
}

void AIGameModDialog::updateEngineUI()
{
    m_EngineBadge->setText(GameEngineDetector::engineName(m_DetectedEngine));
}

void AIGameModDialog::logMessage(const QString &message, const QString &type)
{
    QString color = (type == "success") ? "#7ee787" : (type == "error") ? "#f85149" : "#8b949e";
    m_LogView->appendHtml(QString("<span style='color: %1;'>%2</span>").arg(color, message));
}

void AIGameModDialog::downloadTools() { logMessage("Checking tools...", "info"); }
void AIGameModDialog::runDumper() { logMessage("Running dumper...", "info"); }
void AIGameModDialog::analyzeWithAI() { logMessage("Consulting IA...", "info"); }
void AIGameModDialog::applyMod() { logMessage("Applying patches...", "info"); }
void AIGameModDialog::generatePatch() {}
void AIGameModDialog::searchValues() {}
void AIGameModDialog::bypassSSL() {}
void AIGameModDialog::bypassAntiCheat() {}
void AIGameModDialog::bypassIAP() {}
void AIGameModDialog::extractAssets() {}
void AIGameModDialog::saveModProfile() {}
void AIGameModDialog::askAI(const QString &, std::function<void(const QString&)>) {}
void AIGameModDialog::applyPatch(const QString &, const QByteArray &, const QByteArray &) {}

// Stubs for legacy support
UnityGameDialog::UnityGameDialog(const QString &p, QWidget *par) : QDialog(par) { Q_UNUSED(p) }
FlutterAnalyzerDialog::FlutterAnalyzerDialog(const QString &p, QWidget *par) : QDialog(par) { Q_UNUSED(p) }
GameValueEditorDialog::GameValueEditorDialog(const QString &p, QWidget *par) : QDialog(par) { Q_UNUSED(p) }