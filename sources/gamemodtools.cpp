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
#include <QTabWidget>
#include <QTextStream>
#include <QToolBar>

// ============== Game Engine Detector ==============

GameEngineDetector::Engine GameEngineDetector::detectEngine(const QString &projectPath)
{
    QDir dir(projectPath);
    
    // Check for Unity
    if (dir.exists("lib/armeabi-v7a/libunity.so") || 
        dir.exists("lib/arm64-v8a/libunity.so") ||
        dir.exists("lib/x86/libunity.so") ||
        dir.exists("assets/bin/Data/Managed") ||
        dir.exists("assets/bin/Data/globalgamemanagers")) {
        return Unity;
    }
    
    // Check for Unreal Engine
    if (dir.exists("assets/UE4Game") ||
        dir.exists("assets/UE5Game") ||
        dir.exists("lib/armeabi-v7a/libUE4.so") ||
        dir.exists("lib/arm64-v8a/libUE4.so")) {
        return UnrealEngine;
    }
    
    // Check for Flutter
    if (dir.exists("lib/armeabi-v7a/libflutter.so") ||
        dir.exists("lib/arm64-v8a/libflutter.so") ||
        dir.exists("assets/flutter_assets")) {
        return Flutter;
    }
    
    // Check for React Native
    if (dir.exists("assets/index.android.bundle") ||
        dir.exists("lib/armeabi-v7a/libreactnativejni.so") ||
        dir.exists("lib/arm64-v8a/libreactnativejni.so")) {
        return ReactNative;
    }
    
    // Check for Cocos2d-x
    if (dir.exists("lib/armeabi-v7a/libcocos2dcpp.so") ||
        dir.exists("lib/arm64-v8a/libcocos2dcpp.so") ||
        dir.exists("assets/src") ||
        dir.exists("assets/script")) {
        return Cocos2dx;
    }
    
    // Check for Godot
    if (dir.exists("lib/armeabi-v7a/libgodot_android.so") ||
        dir.exists("lib/arm64-v8a/libgodot_android.so") ||
        dir.exists("assets/project.godot")) {
        return Godot;
    }
    
    // Check for LibGDX
    if (dir.exists("lib/armeabi-v7a/libgdx.so") ||
        dir.exists("lib/arm64-v8a/libgdx.so")) {
        return LibGDX;
    }
    
    // Check for Cordova/PhoneGap
    if (dir.exists("assets/www/cordova.js") ||
        dir.exists("assets/www/index.html")) {
        return Cordova;
    }
    
    // Check for Xamarin
    if (dir.exists("assemblies/Xamarin.Android.dll") ||
        dir.exists("lib/armeabi-v7a/libmonosgen-2.0.so")) {
        return Xamarin;
    }
    
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
        case LibGDX: return "LibGDX";
        case Cordova: return "Cordova/PhoneGap";
        case Xamarin: return "Xamarin";
        case NativeAndroid: return "Native Android";
        default: return "Unknown";
    }
}

QStringList GameEngineDetector::getEngineFiles(Engine engine)
{
    switch (engine) {
        case Unity:
            return {"libunity.so", "libil2cpp.so", "libmono.so", "globalgamemanagers", "level*", "*.assets"};
        case UnrealEngine:
            return {"libUE4.so", "*.pak", "*.uasset", "*.umap"};
        case Flutter:
            return {"libflutter.so", "libapp.so", "kernel_blob.bin", "vm_snapshot_data"};
        case ReactNative:
            return {"index.android.bundle", "libreactnativejni.so"};
        case Cocos2dx:
            return {"libcocos2dcpp.so", "*.lua", "*.luac", "*.js", "*.jsc"};
        case Godot:
            return {"libgodot_android.so", "project.godot", "*.pck"};
        case LibGDX:
            return {"libgdx.so", "*.atlas", "*.fnt"};
        default:
            return {};
    }
}

bool GameEngineDetector::supportsDecompilation(Engine engine)
{
    return engine == Unity || engine == Flutter || engine == ReactNative || 
           engine == Cocos2dx || engine == Xamarin;
}

// ============== Unity Game Dialog ==============

UnityGameDialog::UnityGameDialog(const QString &projectPath, QWidget *parent)
    : QDialog(parent), m_ProjectPath(projectPath), m_IsIl2cpp(false)
{
    setWindowTitle(tr("Unity Game Analyzer"));
    setMinimumSize(900, 700);
    
    m_NetworkManager = new QNetworkAccessManager(this);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Info bar
    QHBoxLayout *infoLayout = new QHBoxLayout();
    m_VersionLabel = new QLabel(tr("Unity Version: Detecting..."));
    m_TypeLabel = new QLabel(tr("Type: Detecting..."));
    infoLayout->addWidget(m_VersionLabel);
    infoLayout->addWidget(m_TypeLabel);
    infoLayout->addStretch();
    mainLayout->addLayout(infoLayout);
    
    // Main content
    QTabWidget *tabs = new QTabWidget();
    
    // Assets tab
    QWidget *assetsTab = new QWidget();
    QVBoxLayout *assetsLayout = new QVBoxLayout(assetsTab);
    m_AssetsTree = new QTreeWidget();
    m_AssetsTree->setHeaderLabels({tr("Asset"), tr("Type"), tr("Size")});
    m_AssetsTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    assetsLayout->addWidget(m_AssetsTree);
    tabs->addTab(assetsTab, tr("📦 Assets"));
    
    // Classes tab
    QWidget *classesTab = new QWidget();
    QVBoxLayout *classesLayout = new QVBoxLayout(classesTab);
    m_ClassesTable = new QTableWidget();
    m_ClassesTable->setColumnCount(4);
    m_ClassesTable->setHorizontalHeaderLabels({tr("Class"), tr("Namespace"), tr("Methods"), tr("Fields")});
    m_ClassesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    classesLayout->addWidget(m_ClassesTable);
    tabs->addTab(classesTab, tr("🏛️ Classes"));
    
    // Details tab
    QWidget *detailsTab = new QWidget();
    QVBoxLayout *detailsLayout = new QVBoxLayout(detailsTab);
    m_DetailsView = new QTextBrowser();
    detailsLayout->addWidget(m_DetailsView);
    tabs->addTab(detailsTab, tr("📋 Details"));
    
    // Log tab
    QWidget *logTab = new QWidget();
    QVBoxLayout *logLayout = new QVBoxLayout(logTab);
    m_LogView = new QPlainTextEdit();
    m_LogView->setReadOnly(true);
    m_LogView->setFont(QFont("Consolas", 10));
    logLayout->addWidget(m_LogView);
    tabs->addTab(logTab, tr("📜 Log"));
    
    mainLayout->addWidget(tabs);
    
    // Progress bar
    m_Progress = new QProgressBar();
    m_Progress->setVisible(false);
    mainLayout->addWidget(m_Progress);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    QPushButton *analyzeBtn = new QPushButton(tr("🔍 Analyze"));
    connect(analyzeBtn, &QPushButton::clicked, this, &UnityGameDialog::analyzeAssets);
    buttonLayout->addWidget(analyzeBtn);
    
    m_DecompileBtn = new QPushButton(tr("🔓 Decompile IL2CPP"));
    connect(m_DecompileBtn, &QPushButton::clicked, this, &UnityGameDialog::decompileIl2cpp);
    buttonLayout->addWidget(m_DecompileBtn);
    
    QPushButton *dumpBtn = new QPushButton(tr("📤 Dump Assembly"));
    connect(dumpBtn, &QPushButton::clicked, this, &UnityGameDialog::dumpAssembly);
    buttonLayout->addWidget(dumpBtn);
    
    m_ModifyBtn = new QPushButton(tr("✏️ Modify Values"));
    connect(m_ModifyBtn, &QPushButton::clicked, this, &UnityGameDialog::modifyGameValues);
    buttonLayout->addWidget(m_ModifyBtn);
    
    m_AiAnalyzeBtn = new QPushButton(tr("🤖 AI Analyze"));
    connect(m_AiAnalyzeBtn, &QPushButton::clicked, this, &UnityGameDialog::aiAnalyzeGame);
    buttonLayout->addWidget(m_AiAnalyzeBtn);
    
    QPushButton *closeBtn = new QPushButton(tr("Close"));
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeBtn);
    
    mainLayout->addLayout(buttonLayout);
    
    // Start detection
    detectUnityVersion();
}

void UnityGameDialog::detectUnityVersion()
{
    m_LogView->appendPlainText(tr("Detecting Unity version..."));
    
    // Check for IL2CPP
    m_IsIl2cpp = hasIl2cpp();
    m_TypeLabel->setText(tr("Type: %1").arg(m_IsIl2cpp ? "IL2CPP" : "Mono"));
    m_DecompileBtn->setEnabled(m_IsIl2cpp);
    
    // Get Unity version from globalgamemanagers
    m_UnityVersion = getUnityVersion();
    m_VersionLabel->setText(tr("Unity Version: %1").arg(m_UnityVersion.isEmpty() ? "Unknown" : m_UnityVersion));
    
    m_LogView->appendPlainText(tr("Unity Version: %1").arg(m_UnityVersion));
    m_LogView->appendPlainText(tr("Build Type: %1").arg(m_IsIl2cpp ? "IL2CPP" : "Mono"));
}

bool UnityGameDialog::isUnityGame()
{
    QDir dir(m_ProjectPath);
    return dir.exists("lib/armeabi-v7a/libunity.so") || 
           dir.exists("lib/arm64-v8a/libunity.so") ||
           dir.exists("assets/bin/Data");
}

bool UnityGameDialog::hasIl2cpp()
{
    QDir dir(m_ProjectPath);
    return dir.exists("lib/armeabi-v7a/libil2cpp.so") || 
           dir.exists("lib/arm64-v8a/libil2cpp.so");
}

bool UnityGameDialog::hasMono()
{
    QDir dir(m_ProjectPath);
    return dir.exists("assets/bin/Data/Managed/Assembly-CSharp.dll");
}

QString UnityGameDialog::getUnityVersion()
{
    QString ggmPath = m_ProjectPath + "/assets/bin/Data/globalgamemanagers";
    if (!QFile::exists(ggmPath)) {
        ggmPath = m_ProjectPath + "/assets/bin/Data/data.unity3d";
    }
    
    if (QFile::exists(ggmPath)) {
        QFile file(ggmPath);
        if (file.open(QFile::ReadOnly)) {
            QByteArray data = file.read(256);
            file.close();
            
            // Unity version is usually in the first bytes
            QRegularExpression versionRe("(\\d+\\.\\d+\\.\\d+[a-zA-Z0-9]*)");
            QRegularExpressionMatch match = versionRe.match(QString::fromLatin1(data));
            if (match.hasMatch()) {
                return match.captured(1);
            }
        }
    }
    
    return QString();
}

void UnityGameDialog::analyzeAssets()
{
    m_Progress->setVisible(true);
    m_Progress->setValue(0);
    m_AssetsTree->clear();
    m_Assets.clear();
    
    m_LogView->appendPlainText(tr("\n=== Analyzing Assets ==="));
    
    QDir assetsDir(m_ProjectPath + "/assets");
    if (!assetsDir.exists()) {
        m_LogView->appendPlainText(tr("❌ Assets directory not found"));
        return;
    }
    
    // Scan for asset files
    QDirIterator it(assetsDir.absolutePath(), QDir::Files, QDirIterator::Subdirectories);
    int count = 0;
    
    while (it.hasNext()) {
        it.next();
        QFileInfo info = it.fileInfo();
        
        UnityAsset asset;
        asset.name = info.fileName();
        asset.path = info.absoluteFilePath();
        asset.size = info.size();
        asset.extractable = true;
        
        // Determine type
        QString ext = info.suffix().toLower();
        if (ext == "assets" || ext == "unity3d") {
            asset.type = "Asset Bundle";
        } else if (ext == "resource" || ext == "resS") {
            asset.type = "Resource";
        } else if (info.fileName() == "globalgamemanagers") {
            asset.type = "Game Config";
        } else if (ext == "dll") {
            asset.type = "Assembly";
        } else if (ext == "so") {
            asset.type = "Native Library";
        } else {
            asset.type = "Other";
        }
        
        m_Assets.append(asset);
        
        // Add to tree
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, asset.name);
        item->setText(1, asset.type);
        item->setText(2, QString::number(asset.size / 1024) + " KB");
        item->setData(0, Qt::UserRole, asset.path);
        m_AssetsTree->addTopLevelItem(item);
        
        count++;
        if (count % 100 == 0) {
            m_Progress->setValue(qMin(count / 10, 100));
            QApplication::processEvents();
        }
    }
    
    m_LogView->appendPlainText(tr("✅ Found %1 assets").arg(m_Assets.count()));
    m_Progress->setVisible(false);
}

void UnityGameDialog::extractIl2cppMetadata()
{
    QString metadataPath = m_ProjectPath + "/assets/bin/Data/Managed/Metadata/global-metadata.dat";
    if (!QFile::exists(metadataPath)) {
        m_LogView->appendPlainText(tr("❌ global-metadata.dat not found"));
        return;
    }
    
    m_LogView->appendPlainText(tr("📋 Extracting IL2CPP metadata..."));
    
    QFile file(metadataPath);
    if (!file.open(QFile::ReadOnly)) {
        m_LogView->appendPlainText(tr("❌ Failed to open metadata file"));
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    // Check magic
    if (data.left(4) != QByteArray("\xAF\x1B\xB1\xFA", 4)) {
        m_LogView->appendPlainText(tr("❌ Invalid metadata file (wrong magic)"));
        return;
    }
    
    m_LogView->appendPlainText(tr("✅ Metadata file is valid"));
    m_LogView->appendPlainText(tr("📊 Size: %1 bytes").arg(data.size()));
}

void UnityGameDialog::decompileIl2cpp()
{
    m_LogView->appendPlainText(tr("\n=== IL2CPP Decompilation ==="));
    m_LogView->appendPlainText(tr("ℹ️ This requires Il2CppDumper tool"));
    
    QString libIl2cppPath;
    if (QFile::exists(m_ProjectPath + "/lib/arm64-v8a/libil2cpp.so")) {
        libIl2cppPath = m_ProjectPath + "/lib/arm64-v8a/libil2cpp.so";
    } else if (QFile::exists(m_ProjectPath + "/lib/armeabi-v7a/libil2cpp.so")) {
        libIl2cppPath = m_ProjectPath + "/lib/armeabi-v7a/libil2cpp.so";
    }
    
    QString metadataPath = m_ProjectPath + "/assets/bin/Data/Managed/Metadata/global-metadata.dat";
    
    if (!QFile::exists(libIl2cppPath)) {
        m_LogView->appendPlainText(tr("❌ libil2cpp.so not found"));
        return;
    }
    
    if (!QFile::exists(metadataPath)) {
        m_LogView->appendPlainText(tr("❌ global-metadata.dat not found"));
        return;
    }
    
    m_LogView->appendPlainText(tr("📁 libil2cpp.so: %1").arg(libIl2cppPath));
    m_LogView->appendPlainText(tr("📁 metadata: %1").arg(metadataPath));
    
    // Show instructions for manual decompilation
    QString msg = tr(
        "To decompile IL2CPP:\n\n"
        "1. Download Il2CppDumper from GitHub\n"
        "2. Run: Il2CppDumper.exe \"%1\" \"%2\"\n"
        "3. Output will be in dump.cs\n\n"
        "Files needed:\n"
        "• libil2cpp.so: %1\n"
        "• global-metadata.dat: %2"
    ).arg(libIl2cppPath, metadataPath);
    
    m_DetailsView->setHtml("<pre>" + msg + "</pre>");
    QMessageBox::information(this, tr("IL2CPP Decompilation"), msg);
}

void UnityGameDialog::modifyGameValues()
{
    GameValueEditorDialog *dialog = new GameValueEditorDialog(m_ProjectPath, this);
    dialog->exec();
}

void UnityGameDialog::dumpAssembly()
{
    m_LogView->appendPlainText(tr("\n=== Dumping Assembly ==="));
    
    if (hasMono()) {
        QString managedPath = m_ProjectPath + "/assets/bin/Data/Managed";
        QDir dir(managedPath);
        QStringList dlls = dir.entryList({"*.dll"}, QDir::Files);
        
        m_LogView->appendPlainText(tr("Found %1 assemblies in Managed folder:").arg(dlls.count()));
        for (const QString &dll : dlls) {
            m_LogView->appendPlainText("  • " + dll);
        }
        
        m_LogView->appendPlainText(tr("\n💡 Use dnSpy or ILSpy to decompile these DLLs"));
    } else {
        m_LogView->appendPlainText(tr("ℹ️ This is an IL2CPP build, use 'Decompile IL2CPP' instead"));
    }
}

void UnityGameDialog::patchAssembly()
{
    // TODO: Implement assembly patching
}

void UnityGameDialog::aiAnalyzeGame()
{
    QString prompt = QString(
        "Analyze this Unity game project structure:\n"
        "- Unity Version: %1\n"
        "- Build Type: %2\n"
        "- Assets found: %3\n\n"
        "Provide analysis on:\n"
        "1. What type of game this might be\n"
        "2. Potential modification points\n"
        "3. Security measures detected\n"
        "4. Suggested tools for further analysis"
    ).arg(m_UnityVersion, m_IsIl2cpp ? "IL2CPP" : "Mono", QString::number(m_Assets.count()));
    
    askAI(prompt, [this](const QString &response) {
        m_DetailsView->setHtml("<h2>AI Analysis</h2><pre>" + response + "</pre>");
    });
}

void UnityGameDialog::aiSuggestMods()
{
    // TODO: Implement AI suggestions
}

void UnityGameDialog::scanAssetBundles()
{
    // TODO: Parse Unity asset bundles
}

void UnityGameDialog::parseGlobalMetadata()
{
    // TODO: Parse global-metadata.dat
}

void UnityGameDialog::findGameValues()
{
    // TODO: Find modifiable game values
}

void UnityGameDialog::askAI(const QString &prompt, std::function<void(const QString&)> callback)
{
    QSettings settings;
    QString provider = settings.value("ai_provider", "gemini").toString();
    QString model = settings.value("ai_model", "gemini-2.0-flash-exp").toString();
    QString apiKey = settings.value("ai_api_key").toString();
    
    if (apiKey.isEmpty()) {
        QMessageBox::warning(this, tr("AI Not Configured"), 
            tr("Configure API key in Settings → AI Assistant"));
        return;
    }
    
    m_LogView->appendPlainText(tr("🤖 Asking AI..."));
    
    QString endpoint;
    QJsonObject root;
    
    if (provider == "gemini") {
        endpoint = QString("https://generativelanguage.googleapis.com/v1beta/models/%1:generateContent?key=%2")
            .arg(model, apiKey);
        QJsonArray contents;
        QJsonObject content;
        QJsonArray parts;
        QJsonObject part;
        part["text"] = prompt;
        parts.append(part);
        content["parts"] = parts;
        contents.append(content);
        root["contents"] = contents;
    } else {
        endpoint = "https://api.openai.com/v1/chat/completions";
        root["model"] = model;
        QJsonArray messages;
        QJsonObject msg;
        msg["role"] = "user";
        msg["content"] = prompt;
        messages.append(msg);
        root["messages"] = messages;
        root["max_tokens"] = 4096;
    }
    
    QNetworkRequest request;
    request.setUrl(QUrl(endpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    if (provider != "gemini") {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());
    }
    
    QNetworkReply *reply = m_NetworkManager->post(request, QJsonDocument(root).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback, provider]() {
        if (reply->error() != QNetworkReply::NoError) {
            m_LogView->appendPlainText(tr("❌ AI Error: %1").arg(reply->errorString()));
            reply->deleteLater();
            return;
        }
        
        QByteArray data = reply->readAll();
        reply->deleteLater();
        
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QString response;
        
        if (provider == "gemini") {
            QJsonArray candidates = doc.object()["candidates"].toArray();
            if (!candidates.isEmpty()) {
                response = candidates[0].toObject()["content"].toObject()["parts"]
                    .toArray()[0].toObject()["text"].toString();
            }
        } else {
            QJsonArray choices = doc.object()["choices"].toArray();
            if (!choices.isEmpty()) {
                response = choices[0].toObject()["message"].toObject()["content"].toString();
            }
        }
        
        m_LogView->appendPlainText(tr("✅ AI response received"));
        callback(response);
    });
}

// ============== Flutter Analyzer Dialog ==============

FlutterAnalyzerDialog::FlutterAnalyzerDialog(const QString &projectPath, QWidget *parent)
    : QDialog(parent), m_ProjectPath(projectPath)
{
    setWindowTitle(tr("Flutter App Analyzer"));
    setMinimumSize(800, 600);
    
    m_NetworkManager = new QNetworkAccessManager(this);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Info bar
    QHBoxLayout *infoLayout = new QHBoxLayout();
    m_FlutterVersionLabel = new QLabel(tr("Flutter: Detecting..."));
    m_DartVersionLabel = new QLabel(tr("Dart: Detecting..."));
    infoLayout->addWidget(m_FlutterVersionLabel);
    infoLayout->addWidget(m_DartVersionLabel);
    infoLayout->addStretch();
    mainLayout->addLayout(infoLayout);
    
    // Tabs
    QTabWidget *tabs = new QTabWidget();
    
    // Assets tab
    QWidget *assetsTab = new QWidget();
    QVBoxLayout *assetsLayout = new QVBoxLayout(assetsTab);
    m_AssetsTree = new QTreeWidget();
    m_AssetsTree->setHeaderLabels({tr("Asset"), tr("Type"), tr("Size")});
    assetsLayout->addWidget(m_AssetsTree);
    tabs->addTab(assetsTab, tr("📦 Assets"));
    
    // Functions tab
    QWidget *functionsTab = new QWidget();
    QVBoxLayout *functionsLayout = new QVBoxLayout(functionsTab);
    m_FunctionsTable = new QTableWidget();
    m_FunctionsTable->setColumnCount(4);
    m_FunctionsTable->setHorizontalHeaderLabels({tr("Function"), tr("Class"), tr("Offset"), tr("Size")});
    m_FunctionsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    functionsLayout->addWidget(m_FunctionsTable);
    tabs->addTab(functionsTab, tr("⚡ Functions"));
    
    // Details tab
    QWidget *detailsTab = new QWidget();
    QVBoxLayout *detailsLayout = new QVBoxLayout(detailsTab);
    m_DetailsView = new QTextBrowser();
    detailsLayout->addWidget(m_DetailsView);
    tabs->addTab(detailsTab, tr("📋 Details"));
    
    // Log tab
    QWidget *logTab = new QWidget();
    QVBoxLayout *logLayout = new QVBoxLayout(logTab);
    m_LogView = new QPlainTextEdit();
    m_LogView->setReadOnly(true);
    logLayout->addWidget(m_LogView);
    tabs->addTab(logTab, tr("📜 Log"));
    
    mainLayout->addWidget(tabs);
    
    // Progress
    m_Progress = new QProgressBar();
    m_Progress->setVisible(false);
    mainLayout->addWidget(m_Progress);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    QPushButton *analyzeBtn = new QPushButton(tr("🔍 Analyze"));
    connect(analyzeBtn, &QPushButton::clicked, this, &FlutterAnalyzerDialog::detectFlutter);
    buttonLayout->addWidget(analyzeBtn);
    
    QPushButton *extractBtn = new QPushButton(tr("📤 Extract Assets"));
    connect(extractBtn, &QPushButton::clicked, this, &FlutterAnalyzerDialog::extractAssets);
    buttonLayout->addWidget(extractBtn);
    
    QPushButton *decompileBtn = new QPushButton(tr("🔓 Decompile Snapshot"));
    connect(decompileBtn, &QPushButton::clicked, this, &FlutterAnalyzerDialog::decompileSnapshot);
    buttonLayout->addWidget(decompileBtn);
    
    QPushButton *aiBtn = new QPushButton(tr("🤖 AI Analyze"));
    connect(aiBtn, &QPushButton::clicked, this, &FlutterAnalyzerDialog::aiAnalyzeFlutter);
    buttonLayout->addWidget(aiBtn);
    
    QPushButton *closeBtn = new QPushButton(tr("Close"));
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeBtn);
    
    mainLayout->addLayout(buttonLayout);
    
    detectFlutter();
}

void FlutterAnalyzerDialog::detectFlutter()
{
    m_LogView->appendPlainText(tr("=== Detecting Flutter ==="));
    
    if (!isFlutterApp()) {
        m_LogView->appendPlainText(tr("❌ This is not a Flutter app"));
        return;
    }
    
    m_FlutterVersion = getFlutterVersion();
    m_FlutterVersionLabel->setText(tr("Flutter: %1").arg(m_FlutterVersion.isEmpty() ? "Unknown" : m_FlutterVersion));
    
    findLibflutter();
    
    m_LogView->appendPlainText(tr("✅ Flutter app detected"));
    m_LogView->appendPlainText(tr("📁 libflutter.so: %1").arg(m_LibflutterPath));
}

bool FlutterAnalyzerDialog::isFlutterApp()
{
    QDir dir(m_ProjectPath);
    return dir.exists("lib/armeabi-v7a/libflutter.so") ||
           dir.exists("lib/arm64-v8a/libflutter.so") ||
           dir.exists("assets/flutter_assets");
}

QString FlutterAnalyzerDialog::getFlutterVersion()
{
    // Try to read from flutter_assets
    QString versionFile = m_ProjectPath + "/assets/flutter_assets/version.json";
    if (QFile::exists(versionFile)) {
        QFile file(versionFile);
        if (file.open(QFile::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            file.close();
            return doc.object()["version"].toString();
        }
    }
    return QString();
}

void FlutterAnalyzerDialog::findLibflutter()
{
    if (QFile::exists(m_ProjectPath + "/lib/arm64-v8a/libflutter.so")) {
        m_LibflutterPath = m_ProjectPath + "/lib/arm64-v8a/libflutter.so";
    } else if (QFile::exists(m_ProjectPath + "/lib/armeabi-v7a/libflutter.so")) {
        m_LibflutterPath = m_ProjectPath + "/lib/armeabi-v7a/libflutter.so";
    }
}

void FlutterAnalyzerDialog::analyzeLibflutter()
{
    // TODO: Analyze libflutter.so
}

void FlutterAnalyzerDialog::extractDartSnapshot()
{
    m_LogView->appendPlainText(tr("\n=== Extracting Dart Snapshot ==="));
    
    QString libappPath;
    if (QFile::exists(m_ProjectPath + "/lib/arm64-v8a/libapp.so")) {
        libappPath = m_ProjectPath + "/lib/arm64-v8a/libapp.so";
    } else if (QFile::exists(m_ProjectPath + "/lib/armeabi-v7a/libapp.so")) {
        libappPath = m_ProjectPath + "/lib/armeabi-v7a/libapp.so";
    }
    
    if (libappPath.isEmpty()) {
        m_LogView->appendPlainText(tr("❌ libapp.so not found"));
        return;
    }
    
    m_LogView->appendPlainText(tr("📁 libapp.so: %1").arg(libappPath));
    m_LogView->appendPlainText(tr("💡 Use reFlutter or flutter_tools for extraction"));
}

void FlutterAnalyzerDialog::decompileSnapshot()
{
    m_LogView->appendPlainText(tr("\n=== Decompiling Dart Snapshot ==="));
    
    QString msg = tr(
        "To decompile Flutter/Dart:\n\n"
        "1. Use reFlutter to patch libflutter.so\n"
        "2. Install patched APK on device\n"
        "3. Run app and capture dump.dart\n"
        "4. Use Dart tools to analyze\n\n"
        "Alternative: Use flutter_tools or darter for static analysis"
    );
    
    m_DetailsView->setHtml("<pre>" + msg + "</pre>");
    QMessageBox::information(this, tr("Flutter Decompilation"), msg);
}

void FlutterAnalyzerDialog::findWidgets()
{
    // TODO: Find Flutter widgets
}

void FlutterAnalyzerDialog::extractAssets()
{
    m_LogView->appendPlainText(tr("\n=== Extracting Flutter Assets ==="));
    
    QString assetsPath = m_ProjectPath + "/assets/flutter_assets";
    if (!QDir(assetsPath).exists()) {
        m_LogView->appendPlainText(tr("❌ flutter_assets not found"));
        return;
    }
    
    m_AssetsTree->clear();
    
    QDirIterator it(assetsPath, QDir::Files, QDirIterator::Subdirectories);
    int count = 0;
    
    while (it.hasNext()) {
        it.next();
        QFileInfo info = it.fileInfo();
        
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, info.fileName());
        item->setText(1, info.suffix().isEmpty() ? "Unknown" : info.suffix().toUpper());
        item->setText(2, QString::number(info.size() / 1024) + " KB");
        m_AssetsTree->addTopLevelItem(item);
        
        count++;
    }
    
    m_LogView->appendPlainText(tr("✅ Found %1 assets").arg(count));
}

void FlutterAnalyzerDialog::aiAnalyzeFlutter()
{
    QString prompt = QString(
        "Analyze this Flutter application:\n"
        "- Flutter Version: %1\n"
        "- libflutter.so path: %2\n\n"
        "Provide analysis on:\n"
        "1. Security measures in Flutter apps\n"
        "2. Common modification techniques\n"
        "3. How to extract/modify Dart code\n"
        "4. Suggested tools for Flutter reverse engineering"
    ).arg(m_FlutterVersion, m_LibflutterPath);
    
    askAI(prompt, [this](const QString &response) {
        m_DetailsView->setHtml("<h2>AI Analysis</h2><pre>" + response + "</pre>");
    });
}

void FlutterAnalyzerDialog::aiSuggestPatches()
{
    // TODO: Implement
}

void FlutterAnalyzerDialog::parseDartSnapshot()
{
    // TODO: Parse Dart snapshot
}

void FlutterAnalyzerDialog::extractFlutterAssets()
{
    // TODO: Extract Flutter assets
}

void FlutterAnalyzerDialog::askAI(const QString &prompt, std::function<void(const QString&)> callback)
{
    QSettings settings;
    QString provider = settings.value("ai_provider", "gemini").toString();
    QString model = settings.value("ai_model", "gemini-2.0-flash-exp").toString();
    QString apiKey = settings.value("ai_api_key").toString();
    
    if (apiKey.isEmpty()) {
        QMessageBox::warning(this, tr("AI Not Configured"), 
            tr("Configure API key in Settings → AI Assistant"));
        return;
    }
    
    m_LogView->appendPlainText(tr("🤖 Asking AI..."));
    
    QString endpoint;
    QJsonObject root;
    
    if (provider == "gemini") {
        endpoint = QString("https://generativelanguage.googleapis.com/v1beta/models/%1:generateContent?key=%2")
            .arg(model, apiKey);
        QJsonArray contents;
        QJsonObject content;
        QJsonArray parts;
        QJsonObject part;
        part["text"] = prompt;
        parts.append(part);
        content["parts"] = parts;
        contents.append(content);
        root["contents"] = contents;
    } else {
        endpoint = "https://api.openai.com/v1/chat/completions";
        root["model"] = model;
        QJsonArray messages;
        QJsonObject msg;
        msg["role"] = "user";
        msg["content"] = prompt;
        messages.append(msg);
        root["messages"] = messages;
    }
    
    QNetworkRequest request;
    request.setUrl(QUrl(endpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    if (provider != "gemini") {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());
    }
    
    QNetworkReply *reply = m_NetworkManager->post(request, QJsonDocument(root).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback, provider]() {
        if (reply->error() != QNetworkReply::NoError) {
            m_LogView->appendPlainText(tr("❌ AI Error: %1").arg(reply->errorString()));
            reply->deleteLater();
            return;
        }
        
        QByteArray data = reply->readAll();
        reply->deleteLater();
        
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QString response;
        
        if (provider == "gemini") {
            QJsonArray candidates = doc.object()["candidates"].toArray();
            if (!candidates.isEmpty()) {
                response = candidates[0].toObject()["content"].toObject()["parts"]
                    .toArray()[0].toObject()["text"].toString();
            }
        } else {
            QJsonArray choices = doc.object()["choices"].toArray();
            if (!choices.isEmpty()) {
                response = choices[0].toObject()["message"].toObject()["content"].toString();
            }
        }
        
        callback(response);
    });
}

// ============== Game Value Editor Dialog ==============

GameValueEditorDialog::GameValueEditorDialog(const QString &projectPath, QWidget *parent)
    : QDialog(parent), m_ProjectPath(projectPath)
{
    setWindowTitle(tr("Game Value Editor"));
    setMinimumSize(800, 600);
    
    m_NetworkManager = new QNetworkAccessManager(this);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Search bar
    QHBoxLayout *searchLayout = new QHBoxLayout();
    m_SearchInput = new QLineEdit();
    m_SearchInput->setPlaceholderText(tr("Search values..."));
    searchLayout->addWidget(m_SearchInput);
    
    m_TypeFilter = new QComboBox();
    m_TypeFilter->addItems({tr("All Types"), "int", "float", "string", "boolean"});
    searchLayout->addWidget(m_TypeFilter);
    
    QPushButton *searchBtn = new QPushButton(tr("🔍 Search"));
    connect(searchBtn, &QPushButton::clicked, this, &GameValueEditorDialog::scanForValues);
    searchLayout->addWidget(searchBtn);
    
    mainLayout->addLayout(searchLayout);
    
    // Splitter
    QSplitter *splitter = new QSplitter(Qt::Vertical);
    
    // Values table
    m_ValuesTable = new QTableWidget();
    m_ValuesTable->setColumnCount(5);
    m_ValuesTable->setHorizontalHeaderLabels({tr("Name"), tr("Type"), tr("Value"), tr("File"), tr("Line")});
    m_ValuesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_ValuesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    splitter->addWidget(m_ValuesTable);
    
    // Preview
    m_PreviewView = new QTextBrowser();
    splitter->addWidget(m_PreviewView);
    
    mainLayout->addWidget(splitter);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    m_ModifyBtn = new QPushButton(tr("✏️ Modify Selected"));
    connect(m_ModifyBtn, &QPushButton::clicked, this, &GameValueEditorDialog::modifyValue);
    buttonLayout->addWidget(m_ModifyBtn);
    
    m_SaveBtn = new QPushButton(tr("💾 Save Changes"));
    connect(m_SaveBtn, &QPushButton::clicked, this, &GameValueEditorDialog::saveChanges);
    buttonLayout->addWidget(m_SaveBtn);
    
    QPushButton *revertBtn = new QPushButton(tr("↩️ Revert"));
    connect(revertBtn, &QPushButton::clicked, this, &GameValueEditorDialog::revertChanges);
    buttonLayout->addWidget(revertBtn);
    
    QPushButton *aiBtn = new QPushButton(tr("🤖 AI Suggest"));
    connect(aiBtn, &QPushButton::clicked, this, &GameValueEditorDialog::aiSuggestValues);
    buttonLayout->addWidget(aiBtn);
    
    QPushButton *closeBtn = new QPushButton(tr("Close"));
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeBtn);
    
    mainLayout->addLayout(buttonLayout);
    
    // Initial scan
    scanForValues();
}

void GameValueEditorDialog::scanForValues()
{
    m_Values.clear();
    m_ValuesTable->setRowCount(0);
    
    scanSmaliForValues();
    scanXmlForValues();
    scanJsonForValues();
    
    // Populate table
    m_ValuesTable->setRowCount(m_Values.count());
    for (int i = 0; i < m_Values.count(); ++i) {
        const GameValue &v = m_Values[i];
        m_ValuesTable->setItem(i, 0, new QTableWidgetItem(v.name));
        m_ValuesTable->setItem(i, 1, new QTableWidgetItem(v.type));
        m_ValuesTable->setItem(i, 2, new QTableWidgetItem(v.currentValue));
        m_ValuesTable->setItem(i, 3, new QTableWidgetItem(QFileInfo(v.file).fileName()));
        m_ValuesTable->setItem(i, 4, new QTableWidgetItem(QString::number(v.line)));
    }
}

void GameValueEditorDialog::scanSmaliForValues()
{
    QDir smaliDir(m_ProjectPath + "/smali");
    if (!smaliDir.exists()) return;
    
    // Common game value patterns
    QRegularExpression constRe("const(?:/4|/16|/high16)?\\s+([pv]\\d+),\\s*(0x[0-9a-fA-F]+|\\d+)");
    QRegularExpression fieldRe("\\.field\\s+[^:]+:(I|F|J|D)\\s*=\\s*([^\\s]+)");
    
    QDirIterator it(smaliDir.absolutePath(), {"*.smali"}, QDir::Files, QDirIterator::Subdirectories);
    
    while (it.hasNext()) {
        it.next();
        QFile file(it.filePath());
        if (!file.open(QFile::ReadOnly | QFile::Text)) continue;
        
        QTextStream in(&file);
        int lineNum = 0;
        
        while (!in.atEnd()) {
            QString line = in.readLine();
            lineNum++;
            
            // Look for const values
            QRegularExpressionMatch match = constRe.match(line);
            if (match.hasMatch()) {
                GameValue v;
                v.name = match.captured(1);
                v.file = it.filePath();
                v.line = lineNum;
                v.originalValue = match.captured(2);
                v.currentValue = v.originalValue;
                v.type = "int";
                v.context = line.trimmed();
                m_Values.append(v);
            }
            
            // Look for field values
            match = fieldRe.match(line);
            if (match.hasMatch()) {
                GameValue v;
                v.name = "field";
                v.file = it.filePath();
                v.line = lineNum;
                v.originalValue = match.captured(2);
                v.currentValue = v.originalValue;
                
                QString type = match.captured(1);
                if (type == "I" || type == "J") v.type = "int";
                else if (type == "F" || type == "D") v.type = "float";
                
                v.context = line.trimmed();
                m_Values.append(v);
            }
        }
        
        file.close();
    }
}

void GameValueEditorDialog::scanXmlForValues()
{
    QDir resDir(m_ProjectPath + "/res/values");
    if (!resDir.exists()) return;
    
    QRegularExpression intRe("<integer[^>]+name=\"([^\"]+)\"[^>]*>([^<]+)</integer>");
    QRegularExpression strRe("<string[^>]+name=\"([^\"]+)\"[^>]*>([^<]+)</string>");
    
    for (const QString &xmlFile : resDir.entryList({"*.xml"}, QDir::Files)) {
        QFile file(resDir.absoluteFilePath(xmlFile));
        if (!file.open(QFile::ReadOnly | QFile::Text)) continue;
        
        QString content = QString::fromUtf8(file.readAll());
        file.close();
        
        // Find integers
        auto it = intRe.globalMatch(content);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            GameValue v;
            v.name = match.captured(1);
            v.file = resDir.absoluteFilePath(xmlFile);
            v.line = 0;
            v.originalValue = match.captured(2);
            v.currentValue = v.originalValue;
            v.type = "int";
            m_Values.append(v);
        }
    }
}

void GameValueEditorDialog::scanJsonForValues()
{
    QDir assetsDir(m_ProjectPath + "/assets");
    if (!assetsDir.exists()) return;
    
    QDirIterator it(assetsDir.absolutePath(), {"*.json"}, QDir::Files, QDirIterator::Subdirectories);
    
    while (it.hasNext()) {
        it.next();
        QFile file(it.filePath());
        if (!file.open(QFile::ReadOnly)) continue;
        
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
        file.close();
        
        if (error.error != QJsonParseError::NoError) continue;
        
        // Extract numeric values from JSON
        std::function<void(const QJsonObject&, const QString&)> extractValues;
        extractValues = [this, &it, &extractValues](const QJsonObject &obj, const QString &prefix) {
            for (auto key = obj.begin(); key != obj.end(); ++key) {
                QString fullKey = prefix.isEmpty() ? key.key() : prefix + "." + key.key();
                
                if (key.value().isDouble()) {
                    GameValue v;
                    v.name = fullKey;
                    v.file = it.filePath();
                    v.originalValue = QString::number(key.value().toDouble());
                    v.currentValue = v.originalValue;
                    v.type = "float";
                    m_Values.append(v);
                } else if (key.value().isObject()) {
                    extractValues(key.value().toObject(), fullKey);
                }
            }
        };
        
        if (doc.isObject()) {
            extractValues(doc.object(), "");
        }
    }
}

void GameValueEditorDialog::scanAssetsForValues()
{
    // TODO: Scan binary assets for values
}

void GameValueEditorDialog::modifyValue()
{
    int row = m_ValuesTable->currentRow();
    if (row < 0 || row >= m_Values.count()) return;
    
    GameValue &v = m_Values[row];
    
    QString newValue = QInputDialog::getText(this, tr("Modify Value"),
        tr("Enter new value for %1:").arg(v.name), QLineEdit::Normal, v.currentValue);
    
    if (!newValue.isEmpty() && newValue != v.currentValue) {
        v.currentValue = newValue;
        m_ValuesTable->item(row, 2)->setText(newValue);
        m_ValuesTable->item(row, 2)->setBackground(QColor(255, 255, 0, 100));
    }
}

void GameValueEditorDialog::saveChanges()
{
    int modified = 0;
    
    for (const GameValue &v : m_Values) {
        if (v.currentValue != v.originalValue) {
            // Read file
            QFile file(v.file);
            if (!file.open(QFile::ReadOnly | QFile::Text)) continue;
            
            QStringList lines;
            QTextStream in(&file);
            while (!in.atEnd()) {
                lines.append(in.readLine());
            }
            file.close();
            
            // Modify line
            if (v.line > 0 && v.line <= lines.count()) {
                lines[v.line - 1].replace(v.originalValue, v.currentValue);
            }
            
            // Write back
            if (file.open(QFile::WriteOnly | QFile::Text)) {
                QTextStream out(&file);
                for (const QString &line : lines) {
                    out << line << "\n";
                }
                file.close();
                modified++;
            }
        }
    }
    
    QMessageBox::information(this, tr("Save Complete"),
        tr("Modified %1 values").arg(modified));
}

void GameValueEditorDialog::revertChanges()
{
    for (int i = 0; i < m_Values.count(); ++i) {
        m_Values[i].currentValue = m_Values[i].originalValue;
        m_ValuesTable->item(i, 2)->setText(m_Values[i].originalValue);
        m_ValuesTable->item(i, 2)->setBackground(Qt::transparent);
    }
}

void GameValueEditorDialog::aiSuggestValues()
{
    QString prompt = QString(
        "I'm modifying an Android game. Here are some values I found:\n\n");
    
    for (int i = 0; i < qMin(20, m_Values.count()); ++i) {
        const GameValue &v = m_Values[i];
        prompt += QString("- %1 (%2) = %3\n").arg(v.name, v.type, v.currentValue);
    }
    
    prompt += "\nSuggest which values might be worth modifying for game cheats (coins, health, damage, etc).";
    
    askAI(prompt, [this](const QString &response) {
        m_PreviewView->setHtml("<h3>AI Suggestions</h3><pre>" + response + "</pre>");
    });
}

void GameValueEditorDialog::askAI(const QString &prompt, std::function<void(const QString&)> callback)
{
    QSettings settings;
    QString provider = settings.value("ai_provider", "gemini").toString();
    QString model = settings.value("ai_model", "gemini-2.0-flash-exp").toString();
    QString apiKey = settings.value("ai_api_key").toString();
    
    if (apiKey.isEmpty()) {
        QMessageBox::warning(this, tr("AI Not Configured"), 
            tr("Configure API key in Settings → AI Assistant"));
        return;
    }
    
    QString endpoint;
    QJsonObject root;
    
    if (provider == "gemini") {
        endpoint = QString("https://generativelanguage.googleapis.com/v1beta/models/%1:generateContent?key=%2")
            .arg(model, apiKey);
        QJsonArray contents;
        QJsonObject content;
        QJsonArray parts;
        QJsonObject part;
        part["text"] = prompt;
        parts.append(part);
        content["parts"] = parts;
        contents.append(content);
        root["contents"] = contents;
    } else {
        endpoint = "https://api.openai.com/v1/chat/completions";
        root["model"] = model;
        QJsonArray messages;
        QJsonObject msg;
        msg["role"] = "user";
        msg["content"] = prompt;
        messages.append(msg);
        root["messages"] = messages;
    }
    
    QNetworkRequest request;
    request.setUrl(QUrl(endpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    if (provider != "gemini") {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());
    }
    
    QNetworkReply *reply = m_NetworkManager->post(request, QJsonDocument(root).toJson());
    connect(reply, &QNetworkReply::finished, this, [reply, callback, provider]() {
        if (reply->error() != QNetworkReply::NoError) {
            reply->deleteLater();
            return;
        }
        
        QByteArray data = reply->readAll();
        reply->deleteLater();
        
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QString response;
        
        if (provider == "gemini") {
            QJsonArray candidates = doc.object()["candidates"].toArray();
            if (!candidates.isEmpty()) {
                response = candidates[0].toObject()["content"].toObject()["parts"]
                    .toArray()[0].toObject()["text"].toString();
            }
        } else {
            QJsonArray choices = doc.object()["choices"].toArray();
            if (!choices.isEmpty()) {
                response = choices[0].toObject()["message"].toObject()["content"].toString();
            }
        }
        
        callback(response);
    });
}

// Stub implementations for other dialogs

NativeLibAnalyzerDialog::NativeLibAnalyzerDialog(const QString &libPath, QWidget *parent)
    : QDialog(parent), m_LibPath(libPath)
{
    setWindowTitle(tr("Native Library Analyzer - %1").arg(QFileInfo(libPath).fileName()));
    setMinimumSize(800, 600);
    m_NetworkManager = new QNetworkAccessManager(this);
    // TODO: Implement full UI
}

void NativeLibAnalyzerDialog::analyzeLib() {}
void NativeLibAnalyzerDialog::extractSymbols() {}
void NativeLibAnalyzerDialog::findStrings() {}
void NativeLibAnalyzerDialog::hexDump() {}
void NativeLibAnalyzerDialog::disassemble() {}
void NativeLibAnalyzerDialog::patchBytes() {}
void NativeLibAnalyzerDialog::aiAnalyzeLib() {}

// ============== AI-Powered Tool Downloader ==============

class GameModToolDownloader : public QObject
{
    Q_OBJECT
public:
    struct Tool {
        QString name;
        QString description;
        QString downloadUrl;
        QString extractPath;
        bool required;
    };
    
    static QList<Tool> getRequiredTools(GameEngineDetector::Engine engine) {
        QList<Tool> tools;
        
        switch (engine) {
        case GameEngineDetector::Unity:
            tools << Tool{"Il2CppDumper", "Dumps IL2CPP metadata", 
                "https://github.com/Perfare/Il2CppDumper/releases/latest/download/Il2CppDumper-net6-win.zip",
                "tools/il2cppdumper", true};
            tools << Tool{"AssetStudio", "Unity asset extractor",
                "https://github.com/Perfare/AssetStudio/releases/latest/download/AssetStudio.net6.v0.16.0.zip",
                "tools/assetstudio", false};
            tools << Tool{"dnSpy", "Mono assembly decompiler",
                "https://github.com/dnSpy/dnSpy/releases/latest/download/dnSpy-net-win64.zip",
                "tools/dnspy", false};
            break;
            
        case GameEngineDetector::Flutter:
            tools << Tool{"reFlutter", "Flutter reverse engineering",
                "https://github.com/nicro950/reFlutter/archive/refs/heads/main.zip",
                "tools/reflutter", true};
            tools << Tool{"Blutter", "Dart AOT snapshot parser",
                "https://github.com/pwnintended/blutter/archive/refs/heads/main.zip",
                "tools/blutter", true};
            break;
            
        case GameEngineDetector::Cocos2dx:
            tools << Tool{"Cocos2dxLuaDec", "Lua script decryptor",
                "https://github.com/nicro950/cocos2dx-luadec/archive/refs/heads/main.zip",
                "tools/luadec", true};
            break;
            
        case GameEngineDetector::UnrealEngine:
            tools << Tool{"UE4Pak", "Unreal pak file extractor",
                "https://github.com/nicro950/ue4pak/releases/latest/download/ue4pak-win.zip",
                "tools/ue4pak", true};
            break;
            
        default:
            break;
        }
        
        return tools;
    }
    
    static bool isToolInstalled(const QString &toolPath) {
        return QDir(toolPath).exists();
    }
};

// ============== AI Game Mod Assistant ==============

class AIGameModAssistant
{
public:
    static QString generateModPrompt(GameEngineDetector::Engine engine, const QString &projectPath) {
        QString prompt = "Analyze this Android game for modding opportunities:\n\n";
        prompt += QString("Engine: %1\n").arg(GameEngineDetector::engineName(engine));
        prompt += QString("Project: %1\n\n").arg(projectPath);
        
        switch (engine) {
        case GameEngineDetector::Unity:
            prompt += "Focus on:\n";
            prompt += "1. IL2CPP metadata extraction for game values\n";
            prompt += "2. PlayerPrefs and saved game data locations\n";
            prompt += "3. In-app purchase validation bypass\n";
            prompt += "4. Anti-cheat detection and bypass\n";
            prompt += "5. Game currency and stats modification\n";
            prompt += "6. Asset extraction and modification\n";
            break;
            
        case GameEngineDetector::Flutter:
            prompt += "Focus on:\n";
            prompt += "1. libflutter.so analysis for SSL pinning\n";
            prompt += "2. libapp.so Dart snapshot analysis\n";
            prompt += "3. API endpoint extraction\n";
            prompt += "4. Authentication bypass\n";
            prompt += "5. Premium feature unlock\n";
            break;
            
        default:
            prompt += "Analyze for common game modifications like:\n";
            prompt += "1. Game values (currency, lives, stats)\n";
            prompt += "2. Anti-cheat bypasses\n";
            prompt += "3. Premium unlocks\n";
            prompt += "4. SSL pinning bypass\n";
            break;
        }
        
        prompt += "\nProvide specific file paths and modification instructions.";
        return prompt;
    }
    
    static QString generatePatchPrompt(const QString &targetFile, const QString &modType) {
        QString prompt = QString("Generate a patch for %1\n\n").arg(targetFile);
        prompt += QString("Modification type: %1\n\n").arg(modType);
        prompt += "Provide:\n";
        prompt += "1. Exact bytes to find (hex)\n";
        prompt += "2. Replacement bytes (hex)\n";
        prompt += "3. Explanation of the patch\n";
        prompt += "4. Risk assessment\n";
        return prompt;
    }
    
    static QString generateValueSearchPrompt(const QString &projectPath) {
        return QString(
            "Search for modifiable game values in %1:\n\n"
            "Look for:\n"
            "- Numeric constants (health, damage, speed, currency)\n"
            "- Boolean flags (isPremium, isUnlocked, hasAds)\n"
            "- String values (API endpoints, version checks)\n"
            "- Configuration files (JSON, XML, binary)\n\n"
            "Provide file paths and line numbers."
        ).arg(projectPath);
    }
};
void NativeLibAnalyzerDialog::parseElfHeader() {}
void NativeLibAnalyzerDialog::parseSymbolTable() {}
void NativeLibAnalyzerDialog::findInterestingPatterns() {}
void NativeLibAnalyzerDialog::askAI(const QString &, std::function<void(const QString&)>) {}

Cocos2dxAnalyzerDialog::Cocos2dxAnalyzerDialog(const QString &projectPath, QWidget *parent)
    : QDialog(parent), m_ProjectPath(projectPath)
{
    setWindowTitle(tr("Cocos2d-x Analyzer"));
    m_NetworkManager = new QNetworkAccessManager(this);
}

void Cocos2dxAnalyzerDialog::detectCocos() {}
void Cocos2dxAnalyzerDialog::analyzeScripts() {}
void Cocos2dxAnalyzerDialog::extractResources() {}
void Cocos2dxAnalyzerDialog::decryptLua() {}
void Cocos2dxAnalyzerDialog::modifyLua() {}
void Cocos2dxAnalyzerDialog::aiAnalyzeCocos() {}
bool Cocos2dxAnalyzerDialog::isCocosGame() { return false; }
QString Cocos2dxAnalyzerDialog::getCocosVersion() { return QString(); }
void Cocos2dxAnalyzerDialog::findLuaScripts() {}
void Cocos2dxAnalyzerDialog::findJsScripts() {}
void Cocos2dxAnalyzerDialog::decryptLuaScript(const QString &) {}
void Cocos2dxAnalyzerDialog::askAI(const QString &, std::function<void(const QString&)>) {}

UnrealAnalyzerDialog::UnrealAnalyzerDialog(const QString &projectPath, QWidget *parent)
    : QDialog(parent), m_ProjectPath(projectPath)
{
    setWindowTitle(tr("Unreal Engine Analyzer"));
    m_NetworkManager = new QNetworkAccessManager(this);
}

void UnrealAnalyzerDialog::detectUnreal() {}
void UnrealAnalyzerDialog::analyzePakFiles() {}
void UnrealAnalyzerDialog::extractAssets() {}
void UnrealAnalyzerDialog::findBlueprints() {}
void UnrealAnalyzerDialog::modifyConfig() {}
void UnrealAnalyzerDialog::aiAnalyzeUnreal() {}
bool UnrealAnalyzerDialog::isUnrealGame() { return false; }
QString UnrealAnalyzerDialog::getUnrealVersion() { return QString(); }
void UnrealAnalyzerDialog::parsePakFile(const QString &) {}
void UnrealAnalyzerDialog::findConfigFiles() {}
void UnrealAnalyzerDialog::askAI(const QString &, std::function<void(const QString&)>) {}

ReactNativeAnalyzerDialog::ReactNativeAnalyzerDialog(const QString &projectPath, QWidget *parent)
    : QDialog(parent), m_ProjectPath(projectPath)
{
    setWindowTitle(tr("React Native Analyzer"));
    m_NetworkManager = new QNetworkAccessManager(this);
}

void ReactNativeAnalyzerDialog::detectReactNative() {}
void ReactNativeAnalyzerDialog::analyzeBundle() {}
void ReactNativeAnalyzerDialog::extractComponents() {}
void ReactNativeAnalyzerDialog::modifyBundle() {}
void ReactNativeAnalyzerDialog::aiAnalyzeRN() {}
bool ReactNativeAnalyzerDialog::isReactNativeApp() { return false; }
QString ReactNativeAnalyzerDialog::getRNVersion() { return QString(); }
void ReactNativeAnalyzerDialog::parseJsBundle() {}
void ReactNativeAnalyzerDialog::findComponents() {}
void ReactNativeAnalyzerDialog::askAI(const QString &, std::function<void(const QString&)>) {}

// ============== AI Game Mod Dialog Implementation ==============

AIGameModDialog::AIGameModDialog(const QString &projectPath, QWidget *parent)
    : QDialog(parent), m_ProjectPath(projectPath), m_DetectedEngine(GameEngineDetector::Unknown)
{
    setWindowTitle(tr("🎮 AI Game Mod Studio"));
    setMinimumSize(1000, 700);
    
    m_NetworkManager = new QNetworkAccessManager(this);
    
    setupUI();
    detectEngine();
}

void AIGameModDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    
    // Header with engine info
    QHBoxLayout *headerLayout = new QHBoxLayout();
    m_EngineLabel = new QLabel(tr("🔍 Detecting game engine..."));
    m_EngineLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    m_StatusLabel = new QLabel(tr("Ready"));
    m_StatusLabel->setStyleSheet("color: #888;");
    headerLayout->addWidget(m_EngineLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_StatusLabel);
    mainLayout->addLayout(headerLayout);
    
    // Progress bar
    m_Progress = new QProgressBar();
    m_Progress->setVisible(false);
    mainLayout->addWidget(m_Progress);
    
    // Main content splitter
    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal);
    
    // Left panel - Mod options
    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    
    // Mod type selector
    QGroupBox *modGroup = new QGroupBox(tr("🛠️ Mod Options"));
    QVBoxLayout *modLayout = new QVBoxLayout(modGroup);
    
    m_ModTypeCombo = new QComboBox();
    m_ModTypeCombo->addItems({
        tr("💰 Currency/Resources Mod"),
        tr("❤️ Health/Lives Mod"),
        tr("⚡ Speed/Damage Mod"),
        tr("🔓 Premium Unlock"),
        tr("🚫 Remove Ads"),
        tr("🔐 SSL Pinning Bypass"),
        tr("🛡️ Anti-Cheat Bypass"),
        tr("💳 IAP Bypass"),
        tr("📦 Extract Assets"),
        tr("🔧 Custom Patch")
    });
    modLayout->addWidget(m_ModTypeCombo);
    
    m_ModOptionsTree = new QTreeWidget();
    m_ModOptionsTree->setHeaderLabels({tr("Option"), tr("Value"), tr("Status")});
    m_ModOptionsTree->setColumnCount(3);
    modLayout->addWidget(m_ModOptionsTree);
    
    leftLayout->addWidget(modGroup);
    
    // Search section
    QGroupBox *searchGroup = new QGroupBox(tr("🔎 Value Search"));
    QVBoxLayout *searchLayout = new QVBoxLayout(searchGroup);
    m_SearchInput = new QLineEdit();
    m_SearchInput->setPlaceholderText(tr("Search for values (e.g., coins, health, gems)..."));
    searchLayout->addWidget(m_SearchInput);
    
    m_ValuesTable = new QTableWidget();
    m_ValuesTable->setColumnCount(5);
    m_ValuesTable->setHorizontalHeaderLabels({tr("Name"), tr("Value"), tr("Type"), tr("File"), tr("Line")});
    m_ValuesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    searchLayout->addWidget(m_ValuesTable);
    leftLayout->addWidget(searchGroup);
    
    mainSplitter->addWidget(leftPanel);
    
    // Right panel - AI Response and Log
    QWidget *rightPanel = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    
    QTabWidget *rightTabs = new QTabWidget();
    
    // AI Response tab
    QWidget *aiTab = new QWidget();
    QVBoxLayout *aiLayout = new QVBoxLayout(aiTab);
    m_AIResponseView = new QTextBrowser();
    m_AIResponseView->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4; font-family: monospace;");
    m_AIResponseView->setPlaceholderText(tr("AI analysis will appear here..."));
    aiLayout->addWidget(m_AIResponseView);
    rightTabs->addTab(aiTab, tr("🤖 AI Analysis"));
    
    // Log tab
    QWidget *logTab = new QWidget();
    QVBoxLayout *logLayout = new QVBoxLayout(logTab);
    m_LogView = new QPlainTextEdit();
    m_LogView->setReadOnly(true);
    m_LogView->setStyleSheet("background-color: #0d1117; color: #c9d1d9; font-family: monospace;");
    logLayout->addWidget(m_LogView);
    rightTabs->addTab(logTab, tr("📜 Log"));
    
    rightLayout->addWidget(rightTabs);
    mainSplitter->addWidget(rightPanel);
    
    mainSplitter->setSizes({400, 600});
    mainLayout->addWidget(mainSplitter);
    
    // Action buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    QPushButton *downloadToolsBtn = new QPushButton(tr("📥 Download Tools"));
    connect(downloadToolsBtn, &QPushButton::clicked, this, &AIGameModDialog::downloadTools);
    buttonLayout->addWidget(downloadToolsBtn);
    
    m_AnalyzeBtn = new QPushButton(tr("🔍 AI Analyze"));
    m_AnalyzeBtn->setStyleSheet("background-color: #238636; color: white; font-weight: bold;");
    connect(m_AnalyzeBtn, &QPushButton::clicked, this, &AIGameModDialog::analyzeWithAI);
    buttonLayout->addWidget(m_AnalyzeBtn);
    
    QPushButton *searchBtn = new QPushButton(tr("🔎 Search Values"));
    connect(searchBtn, &QPushButton::clicked, this, &AIGameModDialog::searchValues);
    buttonLayout->addWidget(searchBtn);
    
    m_ApplyBtn = new QPushButton(tr("✅ Apply Mod"));
    m_ApplyBtn->setStyleSheet("background-color: #1f6feb; color: white; font-weight: bold;");
    connect(m_ApplyBtn, &QPushButton::clicked, this, &AIGameModDialog::applyMod);
    buttonLayout->addWidget(m_ApplyBtn);
    
    QPushButton *saveProfileBtn = new QPushButton(tr("💾 Save Profile"));
    connect(saveProfileBtn, &QPushButton::clicked, this, &AIGameModDialog::saveModProfile);
    buttonLayout->addWidget(saveProfileBtn);
    
    mainLayout->addLayout(buttonLayout);
}

void AIGameModDialog::detectEngine()
{
    m_DetectedEngine = GameEngineDetector::detectEngine(m_ProjectPath);
    QString engineName = GameEngineDetector::engineName(m_DetectedEngine);
    
    m_EngineLabel->setText(tr("🎮 Engine: %1").arg(engineName));
    logMessage(tr("Detected game engine: %1").arg(engineName), "success");
    
    // Populate mod options based on engine
    m_ModOptionsTree->clear();
    
    if (m_DetectedEngine == GameEngineDetector::Unity) {
        QTreeWidgetItem *il2cpp = new QTreeWidgetItem({tr("IL2CPP Dump"), "", tr("Available")});
        QTreeWidgetItem *mono = new QTreeWidgetItem({tr("Mono Assembly"), "", tr("Available")});
        QTreeWidgetItem *assets = new QTreeWidgetItem({tr("Asset Bundles"), "", tr("Available")});
        m_ModOptionsTree->addTopLevelItem(il2cpp);
        m_ModOptionsTree->addTopLevelItem(mono);
        m_ModOptionsTree->addTopLevelItem(assets);
    } else if (m_DetectedEngine == GameEngineDetector::Flutter) {
        QTreeWidgetItem *ssl = new QTreeWidgetItem({tr("SSL Bypass"), "", tr("Available")});
        QTreeWidgetItem *snapshot = new QTreeWidgetItem({tr("Dart Snapshot"), "", tr("Available")});
        m_ModOptionsTree->addTopLevelItem(ssl);
        m_ModOptionsTree->addTopLevelItem(snapshot);
    }
}

void AIGameModDialog::downloadTools()
{
    QList<GameModToolDownloader::Tool> tools = GameModToolDownloader::getRequiredTools(m_DetectedEngine);
    
    if (tools.isEmpty()) {
        logMessage(tr("No specialized tools required for this engine"), "info");
        return;
    }
    
    m_Progress->setVisible(true);
    m_Progress->setMaximum(tools.size());
    m_Progress->setValue(0);
    
    for (const auto &tool : tools) {
        if (GameModToolDownloader::isToolInstalled(tool.extractPath)) {
            logMessage(tr("✅ %1 already installed").arg(tool.name), "success");
        } else {
            logMessage(tr("📥 Downloading %1...").arg(tool.name), "info");
            // TODO: Implement actual download
        }
        m_Progress->setValue(m_Progress->value() + 1);
    }
    
    m_Progress->setVisible(false);
}

void AIGameModDialog::analyzeWithAI()
{
    m_StatusLabel->setText(tr("Analyzing..."));
    m_AnalyzeBtn->setEnabled(false);
    logMessage(tr("🤖 Starting AI analysis..."), "info");
    
    QString prompt = AIGameModAssistant::generateModPrompt(m_DetectedEngine, m_ProjectPath);
    
    askAI(prompt, [this](const QString &response) {
        m_AIResponseView->setHtml(QString("<pre style='white-space: pre-wrap;'>%1</pre>").arg(response));
        m_StatusLabel->setText(tr("Analysis complete"));
        m_AnalyzeBtn->setEnabled(true);
        logMessage(tr("✅ AI analysis complete"), "success");
    });
}

void AIGameModDialog::searchValues()
{
    QString searchTerm = m_SearchInput->text();
    logMessage(tr("🔎 Searching for: %1").arg(searchTerm.isEmpty() ? "all values" : searchTerm), "info");
    
    m_ValuesTable->setRowCount(0);
    
    // Search in smali files
    QDirIterator smaliIt(m_ProjectPath + "/smali", {"*.smali"}, QDir::Files, QDirIterator::Subdirectories);
    while (smaliIt.hasNext()) {
        QString filePath = smaliIt.next();
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            int lineNum = 0;
            while (!in.atEnd()) {
                QString line = in.readLine();
                lineNum++;
                
                // Look for const declarations
                QRegularExpression constRe("const(?:/\\d+)?\\s+\\w+,\\s*(0x[\\da-fA-F]+|\\d+)");
                QRegularExpressionMatch match = constRe.match(line);
                if (match.hasMatch()) {
                    QString value = match.captured(1);
                    if (searchTerm.isEmpty() || line.contains(searchTerm, Qt::CaseInsensitive)) {
                        int row = m_ValuesTable->rowCount();
                        m_ValuesTable->insertRow(row);
                        m_ValuesTable->setItem(row, 0, new QTableWidgetItem(tr("Constant")));
                        m_ValuesTable->setItem(row, 1, new QTableWidgetItem(value));
                        m_ValuesTable->setItem(row, 2, new QTableWidgetItem("int"));
                        m_ValuesTable->setItem(row, 3, new QTableWidgetItem(QFileInfo(filePath).fileName()));
                        m_ValuesTable->setItem(row, 4, new QTableWidgetItem(QString::number(lineNum)));
                    }
                }
            }
        }
    }
    
    logMessage(tr("Found %1 values").arg(m_ValuesTable->rowCount()), "success");
}

void AIGameModDialog::applyMod()
{
    int modType = m_ModTypeCombo->currentIndex();
    logMessage(tr("Applying mod: %1").arg(m_ModTypeCombo->currentText()), "info");
    
    switch (modType) {
    case 5: // SSL Pinning Bypass
        bypassSSL();
        break;
    case 6: // Anti-Cheat Bypass
        bypassAntiCheat();
        break;
    case 7: // IAP Bypass
        bypassIAP();
        break;
    case 8: // Extract Assets
        extractAssets();
        break;
    default:
        generatePatch();
        break;
    }
}

void AIGameModDialog::generatePatch()
{
    QString prompt = AIGameModAssistant::generatePatchPrompt(m_ProjectPath, m_ModTypeCombo->currentText());
    
    askAI(prompt, [this](const QString &response) {
        m_AIResponseView->setHtml(QString("<pre style='white-space: pre-wrap;'>%1</pre>").arg(response));
        logMessage(tr("Patch instructions generated"), "success");
    });
}

void AIGameModDialog::bypassSSL()
{
    logMessage(tr("🔐 Applying SSL Pinning Bypass..."), "info");
    
    // Common SSL bypass patterns
    QStringList patterns = {
        "checkServerTrusted",
        "X509TrustManager",
        "SSLSocketFactory",
        "CertificatePinner",
        "OkHostnameVerifier"
    };
    
    int patchCount = 0;
    QDirIterator it(m_ProjectPath + "/smali", {"*.smali"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString filePath = it.next();
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = file.readAll();
            file.close();
            
            for (const QString &pattern : patterns) {
                if (content.contains(pattern)) {
                    logMessage(tr("Found SSL pattern in: %1").arg(QFileInfo(filePath).fileName()), "info");
                    patchCount++;
                }
            }
        }
    }
    
    logMessage(tr("✅ Found %1 SSL-related files to patch").arg(patchCount), "success");
}

void AIGameModDialog::bypassAntiCheat()
{
    logMessage(tr("🛡️ Analyzing Anti-Cheat mechanisms..."), "info");
    
    QStringList antiCheatPatterns = {
        "SafetyNet", "Play Integrity", "root", "su ", 
        "Magisk", "Xposed", "Frida", "debugger"
    };
    
    // Search for anti-cheat patterns
    QDirIterator it(m_ProjectPath, {"*.smali", "*.xml", "*.json"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString filePath = it.next();
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = file.readAll();
            file.close();
            
            for (const QString &pattern : antiCheatPatterns) {
                if (content.contains(pattern, Qt::CaseInsensitive)) {
                    logMessage(tr("Anti-cheat pattern '%1' found in: %2").arg(pattern, QFileInfo(filePath).fileName()), "warning");
                }
            }
        }
    }
}

void AIGameModDialog::bypassIAP()
{
    logMessage(tr("💳 Analyzing In-App Purchase validation..."), "info");
    
    QStringList iapPatterns = {
        "BillingClient", "Purchase", "verifyPurchase",
        "IabHelper", "consumePurchase", "acknowledgePurchase"
    };
    
    QDirIterator it(m_ProjectPath + "/smali", {"*.smali"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString filePath = it.next();
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = file.readAll();
            file.close();
            
            for (const QString &pattern : iapPatterns) {
                if (content.contains(pattern)) {
                    logMessage(tr("IAP pattern '%1' found in: %2").arg(pattern, QFileInfo(filePath).fileName()), "info");
                }
            }
        }
    }
}

void AIGameModDialog::extractAssets()
{
    logMessage(tr("📦 Extracting game assets..."), "info");
    
    QString assetsPath = m_ProjectPath + "/assets";
    if (!QDir(assetsPath).exists()) {
        logMessage(tr("No assets folder found"), "warning");
        return;
    }
    
    QDirIterator it(assetsPath, QDir::Files, QDirIterator::Subdirectories);
    int count = 0;
    while (it.hasNext()) {
        it.next();
        count++;
    }
    
    logMessage(tr("Found %1 asset files").arg(count), "success");
}

void AIGameModDialog::decompileCode()
{
    logMessage(tr("🔧 Decompiling native code..."), "info");
    // Implementation depends on engine
}

void AIGameModDialog::saveModProfile()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Mod Profile"), 
        m_ProjectPath + "/mod_profile.json", tr("JSON Files (*.json)"));
    
    if (fileName.isEmpty()) return;
    
    QJsonObject profile;
    profile["engine"] = GameEngineDetector::engineName(m_DetectedEngine);
    profile["project"] = m_ProjectPath;
    profile["modType"] = m_ModTypeCombo->currentIndex();
    
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(profile).toJson());
        file.close();
        logMessage(tr("✅ Mod profile saved"), "success");
    }
}

void AIGameModDialog::loadModProfile()
{
    // Implementation
}

void AIGameModDialog::askAI(const QString &prompt, std::function<void(const QString&)> callback)
{
    QSettings settings;
    QString provider = settings.value("ai/provider", "gemini").toString();
    QString apiKey = settings.value("ai/api_key").toString();
    
    if (apiKey.isEmpty()) {
        logMessage(tr("❌ No AI API key configured"), "error");
        return;
    }
    
    QString endpoint;
    QJsonObject root;
    
    if (provider == "gemini") {
        endpoint = QString("https://generativelanguage.googleapis.com/v1beta/models/gemini-pro:generateContent?key=%1").arg(apiKey);
        QJsonArray contents;
        QJsonObject content;
        QJsonArray parts;
        QJsonObject part;
        part["text"] = prompt;
        parts.append(part);
        content["parts"] = parts;
        contents.append(content);
        root["contents"] = contents;
    } else {
        endpoint = "https://api.openai.com/v1/chat/completions";
        root["model"] = settings.value("ai/model", "gpt-4").toString();
        QJsonArray messages;
        QJsonObject msg;
        msg["role"] = "user";
        msg["content"] = prompt;
        messages.append(msg);
        root["messages"] = messages;
    }
    
    QNetworkRequest request;
    request.setUrl(QUrl(endpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    if (provider != "gemini") {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());
    }
    
    QNetworkReply *reply = m_NetworkManager->post(request, QJsonDocument(root).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback, provider]() {
        if (reply->error() != QNetworkReply::NoError) {
            logMessage(tr("❌ AI Error: %1").arg(reply->errorString()), "error");
            reply->deleteLater();
            return;
        }
        
        QByteArray data = reply->readAll();
        reply->deleteLater();
        
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QString response;
        
        if (provider == "gemini") {
            QJsonArray candidates = doc.object()["candidates"].toArray();
            if (!candidates.isEmpty()) {
                response = candidates[0].toObject()["content"].toObject()["parts"]
                    .toArray()[0].toObject()["text"].toString();
            }
        } else {
            QJsonArray choices = doc.object()["choices"].toArray();
            if (!choices.isEmpty()) {
                response = choices[0].toObject()["message"].toObject()["content"].toString();
            }
        }
        
        callback(response);
    });
}

void AIGameModDialog::applyPatch(const QString &file, const QByteArray &find, const QByteArray &replace)
{
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly)) {
        logMessage(tr("Cannot open file: %1").arg(file), "error");
        return;
    }
    
    QByteArray content = f.readAll();
    f.close();
    
    int pos = content.indexOf(find);
    if (pos == -1) {
        logMessage(tr("Pattern not found in: %1").arg(file), "warning");
        return;
    }
    
    content.replace(pos, find.size(), replace);
    
    if (f.open(QIODevice::WriteOnly)) {
        f.write(content);
        f.close();
        logMessage(tr("✅ Patched: %1").arg(file), "success");
    }
}

void AIGameModDialog::logMessage(const QString &message, const QString &type)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString color = "#c9d1d9";
    QString icon = "ℹ️";
    
    if (type == "success") {
        color = "#3fb950";
        icon = "✅";
    } else if (type == "warning") {
        color = "#d29922";
        icon = "⚠️";
    } else if (type == "error") {
        color = "#f85149";
        icon = "❌";
    }
    
    m_LogView->appendHtml(QString("<span style='color: #888;'>[%1]</span> <span style='color: %2;'>%3 %4</span>")
        .arg(timestamp, color, icon, message));
}

// Additional AI prompt generators
QString AIGameModAssistant::generateSSLBypassPrompt(const QString &projectPath)
{
    return QString(
        "Analyze SSL pinning in Android app at %1:\n\n"
        "1. Identify all SSL/TLS certificate pinning implementations\n"
        "2. Find OkHttp CertificatePinner usage\n"
        "3. Locate custom TrustManager implementations\n"
        "4. Find network_security_config.xml settings\n"
        "5. Provide specific smali patches for each method\n\n"
        "Format response with file paths and exact code changes."
    ).arg(projectPath);
}

QString AIGameModAssistant::generateAntiCheatBypassPrompt(const QString &projectPath)
{
    return QString(
        "Analyze anti-cheat/anti-tampering in Android app at %1:\n\n"
        "1. Find SafetyNet/Play Integrity API calls\n"
        "2. Locate root detection methods\n"
        "3. Identify signature verification checks\n"
        "4. Find debugger detection code\n"
        "5. Locate Frida/Xposed detection\n\n"
        "Provide bypass patches for each detection method."
    ).arg(projectPath);
}

QString AIGameModAssistant::generateIAPBypassPrompt(const QString &projectPath)
{
    return QString(
        "Analyze In-App Purchase validation in Android app at %1:\n\n"
        "1. Find Google Play Billing Library usage\n"
        "2. Locate purchase verification methods\n"
        "3. Identify server-side validation calls\n"
        "4. Find premium/pro feature checks\n"
        "5. Locate subscription status checks\n\n"
        "Provide patches to bypass purchase validation."
    ).arg(projectPath);
}

void AIGameModDialog::applyPatch(const QString &file, const QByteArray &find, const QByteArray &replace)
{
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly)) {
        logMessage(tr("Cannot open file: %1").arg(file), "error");
        return;
    }
    
    QByteArray content = f.readAll();
    f.close();
    
    int pos = content.indexOf(find);
    if (pos == -1) {
        logMessage(tr("Pattern not found in: %1").arg(file), "warning");
        return;
    }
    
    content.replace(pos, find.size(), replace);
    
    if (f.open(QIODevice::WriteOnly)) {
        f.write(content);
        f.close();
        logMessage(tr("Patched: %1").arg(file), "success");
    }
}

void AIGameModDialog::logMessage(const QString &message, const QString &type)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString color = "#c9d1d9";
    
    if (type == "success") {
        color = "#3fb950";
    } else if (type == "warning") {
        color = "#d29922";
    } else if (type == "error") {
        color = "#f85149";
    }
    
    m_LogView->appendHtml(QString("<span style='color: #888;'>[%1]</span> <span style='color: %2;'>%3</span>")
        .arg(timestamp, color, message));
}

QString AIGameModAssistant::generateSSLBypassPrompt(const QString &projectPath)
{
    return QString("Analyze SSL pinning in Android app at %1 and provide bypass patches.").arg(projectPath);
}

QString AIGameModAssistant::generateAntiCheatBypassPrompt(const QString &projectPath)
{
    return QString("Analyze anti-cheat in Android app at %1 and provide bypass methods.").arg(projectPath);
}

QString AIGameModAssistant::generateIAPBypassPrompt(const QString &projectPath)
{
    return QString("Analyze IAP validation in Android app at %1 and provide bypass patches.").arg(projectPath);
}
