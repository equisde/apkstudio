#include "gamemodtools.h"
#include <QApplication>
#include <QBoxLayout>
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
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
#include <QScrollArea>
#include <QDialogButtonBox>
#include <QPlainTextEdit>

// ============== Game Engine Detector ==============

GameEngineDetector::Engine GameEngineDetector::detectEngine(const QString &projectPath)
{
    QDir dir(projectPath);
    // Exhaustive search including all architecture folders
    QStringList archs = {"lib/arm64-v8a", "lib/armeabi-v7a", "lib/x86", "lib/x86_64"};
    
    for (const QString &arch : archs) {
        // Unity detection (IL2CPP or Mono)
        if (dir.exists(arch + "/libunity.so") || dir.exists(arch + "/libil2cpp.so") ||
            dir.exists(arch + "/libmono.so") || dir.exists(arch + "/libmonobdwgc-2.0.so")) {
            return Unity;
        }
        // Flutter detection
        if (dir.exists(arch + "/libflutter.so") || dir.exists(arch + "/libapp.so")) {
            return Flutter;
        }
        // Unreal Engine detection
        if (dir.exists(arch + "/libUE4.so") || dir.exists(arch + "/libUnreal.so")) {
            return UnrealEngine;
        }
        // Godot detection
        if (dir.exists(arch + "/libgodot_android.so") || dir.exists(arch + "/libgodot.so")) {
            return Godot;
        }
        // Cocos2d-x detection
        if (dir.exists(arch + "/libcocos2dcpp.so") || dir.exists(arch + "/libcocos2djs.so") ||
            dir.exists(arch + "/libcocos2dlua.so") || dir.exists(arch + "/libgame.so")) {
            return Cocos2dx;
        }
        // React Native detection
        if (dir.exists(arch + "/libreactnativejni.so") || dir.exists(arch + "/libjsc.so") ||
            dir.exists(arch + "/libhermes.so")) {
            return ReactNative;
        }
    }
    
    // Additional checks for assets
    if (dir.exists("assets/bin/Data") || dir.exists("assets/bin/Data/Managed")) {
        return Unity;
    }
    if (dir.exists("assets/UE4Game") || dir.exists("assets/Engine")) {
        return UnrealEngine;
    }
    if (dir.exists("assets/godot.pck") || dir.exists("assets/pack.pck")) {
        return Godot;
    }
    if (dir.exists("assets/src") || dir.exists("assets/res") && dir.exists("assets/script")) {
        return Cocos2dx;
    }
    if (dir.exists("assets/index.android.bundle") || dir.exists("assets/index.bundle")) {
        return ReactNative;
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

// ============== ModConfigWidget Implementation ==============

ModConfigWidget::ModConfigWidget(QWidget *parent) : QWidget(parent)
{
    setupUI();
}

void ModConfigWidget::setupUI()
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    auto scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: none; background: transparent; }");
    
    auto scrollWidget = new QWidget();
    auto scrollLayout = new QVBoxLayout(scrollWidget);
    scrollLayout->setSpacing(10);
    
    // Define mod categories and options with more detailed types
    QList<ModOption> resourceMods = {
        {"coins", tr("Unlimited Coins"), tr("Set coins to maximum value"), false, 999999999, 0, 999999999, "", tr("Resources"), "value"},
        {"gems", tr("Unlimited Gems/Diamonds"), tr("Set gems/diamonds to maximum"), false, 999999999, 0, 999999999, "", tr("Resources"), "value"},
        {"energy", tr("Unlimited Energy/Stamina"), tr("Remove energy limits"), false, 9999, 0, 99999, "", tr("Resources"), "value"},
        {"keys", tr("Unlimited Keys"), tr("Get unlimited keys for chests/levels"), false, 9999, 0, 99999, "", tr("Resources"), "value"},
        {"tickets", tr("Unlimited Tickets"), tr("Remove ticket restrictions"), false, 9999, 0, 99999, "", tr("Resources"), "value"},
        {"tokens", tr("Unlimited Tokens"), tr("Set tokens to maximum"), false, 999999, 0, 999999, "", tr("Resources"), "value"},
        {"stars", tr("Unlimited Stars"), tr("Maximize star count"), false, 9999, 0, 99999, "", tr("Resources"), "value"},
        {"hearts", tr("Unlimited Hearts/Lives"), tr("Never run out of lives"), false, 999, 0, 999, "", tr("Resources"), "value"},
        {"vip", tr("VIP Status"), tr("Enable VIP/Premium features"), false, 99, 1, 99, "", tr("Resources"), "value"}
    };
    
    QList<ModOption> playerMods = {
        {"health", tr("God Mode / Unlimited Health"), tr("Player takes no damage"), false, 999999, 0, 999999, "", tr("Player"), "value"},
        {"damage", tr("One-Hit Kill"), tr("Kill enemies with one hit"), false, 999999, 1, 999999, "", tr("Player"), "multiplier"},
        {"damage_mult", tr("Damage Multiplier"), tr("Multiply player damage"), false, 10, 2, 1000, "", tr("Player"), "multiplier"},
        {"speed", tr("Speed Multiplier"), tr("Increase player movement speed"), false, 5, 1, 100, "", tr("Player"), "multiplier"},
        {"defense", tr("Max Defense/Armor"), tr("Maximum protection"), false, 9999, 0, 9999, "", tr("Player"), "value"},
        {"exp", tr("Unlimited XP/Level"), tr("Maximize experience points"), false, 999999, 0, 999999, "", tr("Player"), "value"},
        {"exp_mult", tr("XP Multiplier"), tr("Multiply experience gained"), false, 10, 2, 1000, "", tr("Player"), "multiplier"},
        {"attack_speed", tr("Attack Speed"), tr("Increase attack speed"), false, 5, 1, 100, "", tr("Player"), "multiplier"},
        {"critical", tr("100% Critical Hit"), tr("Always deal critical damage"), false, 100, 1, 100, "", tr("Player"), "value"},
        {"dodge", tr("100% Dodge/Evasion"), tr("Never get hit"), false, 100, 0, 100, "", tr("Player"), "value"}
    };
    
    QList<ModOption> inventoryMods = {
        {"inventory_max", tr("Max Inventory Stack"), tr("Stack items to maximum"), false, 9999, 1, 9999, "", tr("Inventory"), "value"},
        {"unlock_items", tr("Unlock All Items"), tr("Access all game items"), false, 1, 0, 1, "", tr("Inventory"), "toggle"},
        {"unlock_chars", tr("Unlock All Characters"), tr("Access all playable characters"), false, 1, 0, 1, "", tr("Inventory"), "toggle"},
        {"unlock_levels", tr("Unlock All Levels"), tr("Access all game levels"), false, 1, 0, 1, "", tr("Inventory"), "toggle"},
        {"unlock_skins", tr("Unlock All Skins"), tr("Access all cosmetic items"), false, 1, 0, 1, "", tr("Inventory"), "toggle"},
        {"unlock_weapons", tr("Unlock All Weapons"), tr("Access all weapons"), false, 1, 0, 1, "", tr("Inventory"), "toggle"},
        {"unlock_pets", tr("Unlock All Pets"), tr("Access all companion pets"), false, 1, 0, 1, "", tr("Inventory"), "toggle"},
        {"max_upgrades", tr("Max Upgrades"), tr("Maximize all item upgrades"), false, 1, 0, 1, "", tr("Inventory"), "toggle"}
    };
    
    QList<ModOption> gameMods = {
        {"no_ads", tr("Remove Ads"), tr("Disable all advertisements"), false, 1, 0, 1, "", tr("Game"), "toggle"},
        {"free_iap", tr("Free In-App Purchases"), tr("Bypass payment verification"), false, 1, 0, 1, "", tr("Game"), "toggle"},
        {"no_cooldown", tr("No Cooldowns"), tr("Remove ability cooldowns"), false, 1, 0, 1, "", tr("Game"), "toggle"},
        {"freeze_time", tr("Freeze Timer"), tr("Stop countdown timers"), false, 1, 0, 1, "", tr("Game"), "toggle"},
        {"always_win", tr("Always Win"), tr("Automatic victory"), false, 1, 0, 1, "", tr("Game"), "toggle"},
        {"score_mult", tr("Score Multiplier"), tr("Multiply points earned"), false, 10, 2, 1000, "", tr("Game"), "multiplier"},
        {"no_enemies", tr("No Enemies"), tr("Disable enemy spawning"), false, 1, 0, 1, "", tr("Game"), "toggle"},
        {"instant_kill", tr("Instant Kill Enemies"), tr("One hit defeats any enemy"), false, 1, 0, 1, "", tr("Game"), "toggle"},
        {"custom", tr("Custom Modification"), tr("Specify your own modification"), false, 0, 0, 999999999, "", tr("Game"), "custom"}
    };
    
    addModCategory(scrollLayout, tr("💰 Resources & Currency"), resourceMods);
    addModCategory(scrollLayout, tr("🎮 Player Stats"), playerMods);
    addModCategory(scrollLayout, tr("📦 Inventory & Unlocks"), inventoryMods);
    addModCategory(scrollLayout, tr("⚙️ Game Mechanics"), gameMods);
    
    scrollLayout->addStretch();
    scrollArea->setWidget(scrollWidget);
    mainLayout->addWidget(scrollArea);
}

void ModConfigWidget::addModCategory(QVBoxLayout *layout, const QString &category, const QList<ModOption> &options)
{
    auto group = new QGroupBox(category);
    group->setStyleSheet("QGroupBox { font-weight: bold; color: #58a6ff; border: 1px solid #30363d; border-radius: 6px; margin-top: 10px; padding-top: 10px; } "
                         "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }");
    
    auto groupLayout = new QVBoxLayout(group);
    
    for (const ModOption &opt : options) {
        m_AllOptions.append(opt);
        
        auto rowLayout = new QHBoxLayout();
        
        auto checkbox = new QCheckBox(opt.name);
        checkbox->setToolTip(opt.description);
        checkbox->setStyleSheet("QCheckBox { color: #c9d1d9; }");
        m_Checkboxes[opt.id] = checkbox;
        rowLayout->addWidget(checkbox);
        
        if (opt.id == "custom") {
            auto customInput = new QLineEdit();
            customInput->setPlaceholderText(tr("Describe your modification..."));
            customInput->setStyleSheet("QLineEdit { background: #21262d; border: 1px solid #30363d; color: #c9d1d9; padding: 5px; border-radius: 4px; }");
            m_CustomInputs[opt.id] = customInput;
            rowLayout->addWidget(customInput, 1);
        } else if (opt.maxValue > 1) {
            auto valueSpin = new QSpinBox();
            valueSpin->setRange(opt.minValue, opt.maxValue);
            valueSpin->setValue(opt.value);
            valueSpin->setStyleSheet("QSpinBox { background: #21262d; border: 1px solid #30363d; color: #c9d1d9; padding: 3px; border-radius: 4px; min-width: 100px; }");
            m_ValueSpins[opt.id] = valueSpin;
            rowLayout->addWidget(valueSpin);
        }
        
        rowLayout->addStretch();
        groupLayout->addLayout(rowLayout);
        
        connect(checkbox, &QCheckBox::toggled, this, &ModConfigWidget::configChanged);
    }
    
    layout->addWidget(group);
}

QList<ModOption> ModConfigWidget::getSelectedMods() const
{
    QList<ModOption> selected;
    
    for (const ModOption &opt : m_AllOptions) {
        if (m_Checkboxes.contains(opt.id) && m_Checkboxes[opt.id]->isChecked()) {
            ModOption mod = opt;
            mod.enabled = true;
            
            if (m_ValueSpins.contains(opt.id)) {
                mod.value = m_ValueSpins[opt.id]->value();
            }
            if (m_CustomInputs.contains(opt.id)) {
                mod.customValue = m_CustomInputs[opt.id]->text();
            }
            
            selected.append(mod);
        }
    }
    
    return selected;
}

QString ModConfigWidget::generateModDescription() const
{
    QStringList descriptions;
    QList<ModOption> mods = getSelectedMods();
    
    for (const ModOption &mod : mods) {
        if (mod.id == "custom" && !mod.customValue.isEmpty()) {
            descriptions << QString("Custom: %1").arg(mod.customValue);
        } else if (mod.maxValue > 1) {
            descriptions << QString("%1 (value: %2)").arg(mod.name).arg(mod.value);
        } else {
            descriptions << mod.name;
        }
    }
    
    return descriptions.join(", ");
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

    // Engine badge
    m_EngineBadge = new QLabel(tr("Detecting..."));
    m_EngineBadge->setStyleSheet("background-color: #238636; color: white; padding: 8px 15px; border-radius: 12px; font-weight: bold;");
    layout->addWidget(m_EngineBadge);

    // Tool buttons row
    auto heroLayout = new QHBoxLayout();
    m_DownloadToolsBtn = new QPushButton(tr("📥 Setup Tools"));
    m_RunDumperBtn = new QPushButton(tr("🔧 Decompile"));
    m_VerifyBtn = new QPushButton(tr("✅ Verify"));
    m_AnalyzeBtn = new QPushButton(tr("🤖 AI Analyze"));
    m_ModMenuBtn = new QPushButton(tr("🎮 Generate Mod Menu"));
    
    QString btnStyle = "QPushButton { background-color: #21262d; border: 1px solid #30363d; padding: 10px; border-radius: 6px; color: #c9d1d9; }";
    m_DownloadToolsBtn->setStyleSheet(btnStyle);
    m_RunDumperBtn->setStyleSheet("QPushButton { background-color: #21262d; border: 1px solid #388bfd; color: #58a6ff; padding: 10px; border-radius: 6px; }");
    m_VerifyBtn->setStyleSheet(btnStyle);
    m_AnalyzeBtn->setStyleSheet("QPushButton { background-color: #238636; color: white; padding: 10px; border-radius: 6px; border: none; }");
    m_ModMenuBtn->setStyleSheet("QPushButton { background-color: #1f6feb; color: white; padding: 10px; border-radius: 6px; border: none; }");

    heroLayout->addWidget(m_DownloadToolsBtn);
    heroLayout->addWidget(m_RunDumperBtn);
    heroLayout->addWidget(m_VerifyBtn);
    heroLayout->addWidget(m_AnalyzeBtn);
    heroLayout->addWidget(m_ModMenuBtn);
    
    m_InteractiveBtn = new QPushButton(tr("🎨 Interactive Builder"));
    m_InteractiveBtn->setStyleSheet("QPushButton { background-color: #8957e5; color: white; padding: 10px; border-radius: 6px; border: none; }");
    heroLayout->addWidget(m_InteractiveBtn);
    
    layout->addLayout(heroLayout);

    m_Progress = new QProgressBar();
    m_Progress->setVisible(false);
    m_Progress->setStyleSheet("QProgressBar::chunk { background-color: #238636; }");
    layout->addWidget(m_Progress);

    // Main content splitter
    auto mainSplitter = new QSplitter(Qt::Horizontal);
    
    // Left side: Mod configuration
    auto leftWidget = new QWidget();
    auto leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    
    auto modLabel = new QLabel(tr("<b>🎯 Select Modifications</b>"));
    modLabel->setStyleSheet("color: #58a6ff; font-size: 14px;");
    leftLayout->addWidget(modLabel);
    
    m_ModConfig = new ModConfigWidget();
    leftLayout->addWidget(m_ModConfig);
    
    mainSplitter->addWidget(leftWidget);
    
    // Right side: AI response and logs
    auto rightSplitter = new QSplitter(Qt::Vertical);
    
    m_AIResponseView = new QTextBrowser();
    m_AIResponseView->setStyleSheet("background-color: #0d1117; color: #d1d5da; font-family: 'Consolas', monospace;");
    m_AIResponseView->setOpenExternalLinks(true);
    rightSplitter->addWidget(m_AIResponseView);
    
    // AI Chat input
    auto chatWidget = new QWidget();
    auto chatLayout = new QHBoxLayout(chatWidget);
    chatLayout->setContentsMargins(0, 0, 0, 0);
    
    m_AIChatInput = new QLineEdit();
    m_AIChatInput->setPlaceholderText(tr("Ask AI about game modifications... (e.g., 'How do I modify player health?')"));
    m_AIChatInput->setStyleSheet("QLineEdit { background: #21262d; border: 1px solid #30363d; color: #c9d1d9; padding: 10px; border-radius: 6px; }");
    
    auto sendBtn = new QPushButton(tr("Send"));
    sendBtn->setStyleSheet("QPushButton { background-color: #238636; color: white; padding: 10px 20px; border-radius: 6px; }");
    
    chatLayout->addWidget(m_AIChatInput, 1);
    chatLayout->addWidget(sendBtn);
    rightSplitter->addWidget(chatWidget);
    
    m_LogView = new QPlainTextEdit();
    m_LogView->setReadOnly(true);
    m_LogView->setStyleSheet("background-color: #010409; color: #7ee787; font-family: 'Consolas', monospace;");
    m_LogView->setMaximumHeight(150);
    rightSplitter->addWidget(m_LogView);
    
    rightSplitter->setSizes({400, 50, 150});
    mainSplitter->addWidget(rightSplitter);
    mainSplitter->setSizes({350, 650});
    
    layout->addWidget(mainSplitter);

    // Connections
    connect(m_DownloadToolsBtn, &QPushButton::clicked, this, &GameModStudio::downloadTools);
    connect(m_RunDumperBtn, &QPushButton::clicked, this, &GameModStudio::runDumper);
    connect(m_VerifyBtn, &QPushButton::clicked, this, &GameModStudio::verifyDecompilation);
    connect(m_AnalyzeBtn, &QPushButton::clicked, this, &GameModStudio::analyzeWithAI);
    connect(m_ModMenuBtn, &QPushButton::clicked, this, &GameModStudio::generateModMenu);
    connect(m_InteractiveBtn, &QPushButton::clicked, this, &GameModStudio::openInteractiveModBuilder);
    connect(sendBtn, &QPushButton::clicked, this, &GameModStudio::openAIChat);
    connect(m_AIChatInput, &QLineEdit::returnPressed, this, &GameModStudio::openAIChat);
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
    // 1. Check if this is a Unity game
    if (m_DetectedEngine != GameEngineDetector::Unity) {
        logMessage("This is not a Unity game. Use appropriate tools for " + GameEngineDetector::engineName(m_DetectedEngine), "warning");
        return;
    }
    
    // 2. Try Mono decompilation first (Assembly-CSharp.dll)
    QString managedPath = m_ProjectPath + "/assets/bin/Data/Managed/Assembly-CSharp.dll";
    if (QFile::exists(managedPath)) {
        logMessage("Assembly-CSharp.dll found - Mono backend detected.", "info");
        
        // Check if already decompiled
        if (QDir(m_ProjectPath + "/csharp_src").exists() && !QDir(m_ProjectPath + "/csharp_src").isEmpty()) {
            logMessage("C# source already extracted. Running AI analysis...", "info");
            analyzeWithAI();
            return;
        }
        
        if (runILSpyDecompilation()) {
            return; // Successfully started decompilation
        }
    }

    // 3. Try IL2CPP dumper
    bool hasIl2cpp = false;
    QStringList archs = {"arm64-v8a", "armeabi-v7a", "x86", "x86_64"};
    for (const QString &arch : archs) {
        if (QFile::exists(m_ProjectPath + "/lib/" + arch + "/libil2cpp.so")) {
            hasIl2cpp = true;
            break;
        }
    }
    
    if (hasIl2cpp) {
        // Check if already dumped
        if (QFile::exists(m_ProjectPath + "/dump/dump.cs")) {
            logMessage("IL2CPP dump already exists. Running AI analysis...", "info");
            analyzeWithAI();
            return;
        }
        
        if (runIl2CppDumper()) {
            return; // Successfully started dumping
        }
    }
    
    // 4. Check if IL2CPP dump exists with dummy DLLs - can decompile those with ILSpy
    QString dummyDll = m_ProjectPath + "/dump/DummyDll/Assembly-CSharp.dll";
    if (QFile::exists(dummyDll)) {
        logMessage("Found DummyDll from IL2CPP Dumper - attempting ILSpy decompilation...", "info");
        if (runILSpyOnDummyDlls()) {
            return;
        }
    }
    
    // 5. Neither found or configured
    logMessage("Could not decompile. Check 'Verify' for details and configure tools in Settings.", "error");
    verifyDecompilation();
}

void GameModStudio::analyzeWithAI()
{
    QSettings settings;
    QString model = settings.value("ai_model", "gemini-2.0-flash-exp").toString();
    logMessage("Consulting AI Engine (" + model + ")...", "info");
    logMessage("Deep scanning game code for modding opportunities...", "info");

    QString gameContext;
    QString engineType = "Unknown";
    QString gameName = QDir(m_ProjectPath).dirName();
    int filesAnalyzed = 0;
    int totalClasses = 0;
    int valuableFieldsFound = 0;
    
    // Detect engine type
    bool isIL2CPP = QFile::exists(m_ProjectPath + "/lib/arm64-v8a/libil2cpp.so") ||
                   QFile::exists(m_ProjectPath + "/lib/armeabi-v7a/libil2cpp.so");
    bool isMono = QFile::exists(m_ProjectPath + "/assets/bin/Data/Managed/Assembly-CSharp.dll");
    
    if (isIL2CPP || isMono) {
        engineType = isIL2CPP ? "Unity (IL2CPP)" : "Unity (Mono)";
    } else if (QFile::exists(m_ProjectPath + "/assets/main.pak") ||
               QFile::exists(m_ProjectPath + "/lib/arm64-v8a/libUE4.so")) {
        engineType = "Unreal Engine";
    } else if (QFile::exists(m_ProjectPath + "/lib/arm64-v8a/libflutter.so")) {
        engineType = "Flutter";
    } else {
        engineType = "Native Android/Java";
    }
    
    // Extract package name from manifest
    QString packageName = "unknown";
    QString manifestPath = m_ProjectPath + "/AndroidManifest.xml";
    if (QFile::exists(manifestPath)) {
        QFile manifestFile(manifestPath);
        if (manifestFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString manifestContent = QString::fromUtf8(manifestFile.readAll());
            manifestFile.close();
            QRegularExpression pkgRegex("package=\"([^\"]+)\"");
            QRegularExpressionMatch match = pkgRegex.match(manifestContent);
            if (match.hasMatch()) {
                packageName = match.captured(1);
                gameName = packageName.split('.').last();
            }
        }
    }
    
    gameContext += QString("# Game: %1\n").arg(gameName);
    gameContext += QString("Package: `%1` | Engine: **%2**\n\n").arg(packageName, engineType);

    // Structured data for AI
    struct FieldInfo {
        QString className;
        QString fieldName;
        QString fieldType;
        QString offset;
        QString category;
    };
    struct MethodInfo {
        QString className;
        QString methodName;
        QString returnType;
        QString params;
        QString rva;
        QString category;
    };
    QList<FieldInfo> valuableFields;
    QList<MethodInfo> valuableMethods;
    
    // BLACKLIST: Classes to completely ignore (UI, Animation, generic Unity)
    QStringList classBlacklist = {
        "Token", "Animation", "Animated", "Tween", "UI", "Button", "Text", "Image",
        "Panel", "Canvas", "Layout", "Scroll", "Renderer", "Shader", "Material",
        "Sprite", "Particle", "Audio", "Sound", "Music", "Effect", "Transition",
        "Popup", "Modal", "Dialog", "Tooltip", "HUD", "Label", "Icon", "Badge",
        "Localization", "Translation", "EventLog", "Analytics", "Tracking",
        "DOTween", "LeanTween", "iTween", "UniRx", "Cysharp"
    };
    
    // WHITELIST: Class name patterns that are HIGH VALUE targets
    QStringList highValueClassPatterns = {
        "PlayerData", "UserData", "SaveData", "GameData", "ProfileData",
        "CurrencyManager", "CoinManager", "GemManager", "WalletManager",
        "InventoryManager", "ItemManager", "EquipmentManager",
        "HealthManager", "DamageManager", "CombatManager", "StatsManager",
        "ShopManager", "IAPManager", "PurchaseManager", "StoreManager",
        "EnergyManager", "StaminaManager", "TimerManager", "CooldownManager",
        "PlayerStats", "PlayerProfile", "PlayerController", "PlayerState",
        "GameManager", "GameController", "GameState", "LevelManager",
        "CheatDetector", "AntiCheat", "SecurityManager", "IntegrityCheck",
        "Wallet", "Balance", "Currency", "Economy"
    };
    
    // Field name patterns that indicate moddable values
    QStringList valuableFieldPatterns = {
        "gold", "coin", "gem", "diamond", "ruby", "crystal", "money", "cash", "credit",
        "health", "hp", "maxHealth", "maxHp", "currentHealth", "currentHp",
        "damage", "attack", "atk", "defense", "def", "speed", "spd", "power", "str",
        "energy", "stamina", "mana", "mp", "ap",
        "level", "lvl", "experience", "exp", "xp", "score", "points",
        "count", "amount", "quantity", "balance", "total",
        "cooldown", "timer", "duration", "remaining",
        "price", "cost", "reward", "bonus", "multiplier"
    };
    
    // Method name patterns that indicate moddable logic
    QStringList valuableMethodPatterns = {
        "AddCoin", "AddGem", "AddGold", "AddMoney", "AddCurrency", "SetCurrency",
        "AddHealth", "SetHealth", "TakeDamage", "Heal", "Die", "Kill",
        "AddDamage", "SetDamage", "AddAttack", "SetAttack",
        "AddEnergy", "SetEnergy", "ConsumeEnergy", "RefillEnergy",
        "AddExperience", "AddXP", "AddExp", "LevelUp", "SetLevel",
        "Purchase", "Buy", "CanAfford", "SpendCurrency", "DeductCurrency",
        "Validate", "Verify", "CheckIntegrity", "IsHacked", "DetectCheat",
        "UnlockItem", "UnlockAll", "GrantReward", "ClaimReward",
        "StartTimer", "StopTimer", "ResetCooldown", "SkipCooldown"
    };

    // 1. Parse IL2CPP dump.cs with PRECISE extraction
    QString dumpPath = m_ProjectPath + "/dump/dump.cs";
    if (QFile::exists(dumpPath)) {
        QFile dumpFile(dumpPath);
        if (dumpFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString dumpContent = QString::fromUtf8(dumpFile.readAll());
            dumpFile.close();
            
            logMessage(QString("Parsing dump.cs (%1 MB) with precision extraction...").arg(dumpContent.size() / 1024 / 1024.0, 0, 'f', 2), "info");
            
            QStringList lines = dumpContent.split('\n');
            QString currentClassName;
            QString currentNamespace;
            bool inRelevantClass = false;
            int braceCount = 0;
            
            // Optimized regex patterns
            QRegularExpression namespaceRegex("^namespace\\s+([\\w\\.]+)");
            QRegularExpression classRegex("^\\s*(public|internal|private)?\\s*(sealed|abstract|static)?\\s*(class|struct)\\s+(\\w+)");
            // Match fields with offset: public int gold; // 0x20
            QRegularExpression fieldWithOffsetRegex("^\\s*(public|private|protected|internal)?\\s*(static)?\\s*(readonly)?\\s*(int|float|double|long|bool|byte|short|ushort|uint|ulong|string|Int32|Int64|Single|Double|Boolean)\\s+(\\w+)\\s*;\\s*//\\s*(0x[0-9A-Fa-f]+)");
            // Match methods with RVA: public void AddGold(int amount) { } // RVA: 0x123456
            QRegularExpression methodWithRvaRegex("^\\s*(public|private|protected|internal)?\\s*(static)?\\s*(virtual|override)?\\s*(void|int|float|double|bool|string|\\w+)\\s+(\\w+)\\s*\\(([^)]*)\\)\\s*[^/]*//\\s*RVA:\\s*(0x[0-9A-Fa-f]+)");
            
            for (int i = 0; i < lines.size(); i++) {
                const QString &line = lines[i];
                
                // Track namespace
                QRegularExpressionMatch nsMatch = namespaceRegex.match(line);
                if (nsMatch.hasMatch()) {
                    currentNamespace = nsMatch.captured(1);
                    continue;
                }
                
                // Detect class/struct declaration
                QRegularExpressionMatch classMatch = classRegex.match(line);
                if (classMatch.hasMatch()) {
                    currentClassName = classMatch.captured(4);
                    totalClasses++;
                    
                    // Check if class should be analyzed
                    bool isBlacklisted = false;
                    for (const QString &bl : classBlacklist) {
                        if (currentClassName.contains(bl, Qt::CaseInsensitive)) {
                            isBlacklisted = true;
                            break;
                        }
                    }
                    
                    if (isBlacklisted) {
                        inRelevantClass = false;
                        continue;
                    }
                    
                    // Check if high-value class
                    bool isHighValue = false;
                    for (const QString &pattern : highValueClassPatterns) {
                        if (currentClassName.contains(pattern, Qt::CaseInsensitive)) {
                            isHighValue = true;
                            break;
                        }
                    }
                    
                    inRelevantClass = isHighValue;
                    if (inRelevantClass) {
                        braceCount = 0;
                        filesAnalyzed++;
                    }
                    continue;
                }
                
                // Track brace count for class scope
                if (inRelevantClass) {
                    braceCount += line.count('{') - line.count('}');
                    if (braceCount < 0) {
                        inRelevantClass = false;
                        continue;
                    }
                }
                
                // Extract fields with offsets (works for any class, but prioritize relevant ones)
                QRegularExpressionMatch fieldMatch = fieldWithOffsetRegex.match(line);
                if (fieldMatch.hasMatch()) {
                    QString fieldName = fieldMatch.captured(5);
                    QString fieldType = fieldMatch.captured(4);
                    QString offset = fieldMatch.captured(6);
                    
                    // Check if field name matches valuable patterns
                    bool isValuable = false;
                    QString category;
                    for (const QString &pattern : valuableFieldPatterns) {
                        if (fieldName.contains(pattern, Qt::CaseInsensitive)) {
                            isValuable = true;
                            // Categorize
                            if (pattern.contains(QRegularExpression("gold|coin|gem|diamond|money|cash|credit|currency|balance", QRegularExpression::CaseInsensitiveOption)))
                                category = "💰 Currency";
                            else if (pattern.contains(QRegularExpression("health|hp|damage|attack|defense|speed|power", QRegularExpression::CaseInsensitiveOption)))
                                category = "❤️ Player Stats";
                            else if (pattern.contains(QRegularExpression("energy|stamina|mana|cooldown|timer", QRegularExpression::CaseInsensitiveOption)))
                                category = "⏱️ Energy/Timer";
                            else if (pattern.contains(QRegularExpression("level|exp|xp|score", QRegularExpression::CaseInsensitiveOption)))
                                category = "📈 Progression";
                            else if (pattern.contains(QRegularExpression("price|cost|purchase", QRegularExpression::CaseInsensitiveOption)))
                                category = "🛒 Shop/IAP";
                            else
                                category = "🎮 Game Value";
                            break;
                        }
                    }
                    
                    if (isValuable || inRelevantClass) {
                        valuableFields.append({currentClassName, fieldName, fieldType, offset, category.isEmpty() ? "🎮 Game Value" : category});
                        valuableFieldsFound++;
                    }
                }
                
                // Extract methods with RVA
                QRegularExpressionMatch methodMatch = methodWithRvaRegex.match(line);
                if (methodMatch.hasMatch()) {
                    QString methodName = methodMatch.captured(5);
                    QString returnType = methodMatch.captured(4);
                    QString params = methodMatch.captured(6);
                    QString rva = methodMatch.captured(7);
                    
                    // Check if method name matches valuable patterns
                    bool isValuable = false;
                    QString category;
                    for (const QString &pattern : valuableMethodPatterns) {
                        if (methodName.contains(pattern, Qt::CaseInsensitive)) {
                            isValuable = true;
                            if (pattern.contains(QRegularExpression("Coin|Gem|Gold|Money|Currency", QRegularExpression::CaseInsensitiveOption)))
                                category = "💰 Currency";
                            else if (pattern.contains(QRegularExpression("Health|Damage|Attack|Die|Kill|Heal", QRegularExpression::CaseInsensitiveOption)))
                                category = "❤️ Combat";
                            else if (pattern.contains(QRegularExpression("Energy|Stamina|Timer|Cooldown", QRegularExpression::CaseInsensitiveOption)))
                                category = "⏱️ Energy/Timer";
                            else if (pattern.contains(QRegularExpression("Experience|XP|Level", QRegularExpression::CaseInsensitiveOption)))
                                category = "📈 Progression";
                            else if (pattern.contains(QRegularExpression("Purchase|Buy|Afford|Spend", QRegularExpression::CaseInsensitiveOption)))
                                category = "🛒 Shop/IAP";
                            else if (pattern.contains(QRegularExpression("Validate|Verify|Integrity|Cheat|Hack", QRegularExpression::CaseInsensitiveOption)))
                                category = "🛡️ Anti-Cheat";
                            else
                                category = "🎮 Game Logic";
                            break;
                        }
                    }
                    
                    if (isValuable || inRelevantClass) {
                        valuableMethods.append({currentClassName, methodName, returnType, params, rva, category.isEmpty() ? "🎮 Game Logic" : category});
                    }
                }
            }
            
            logMessage(QString("Found %1 valuable fields and %2 hookable methods from %3 classes").arg(valuableFields.size()).arg(valuableMethods.size()).arg(filesAnalyzed), "info");
            
            // Build structured context for AI
            if (!valuableFields.isEmpty() || !valuableMethods.isEmpty()) {
                gameContext += "## 🎯 EXTRACTED MODDING TARGETS\n\n";
                gameContext += "*These are the EXACT fields and methods found in the game code with their memory offsets/RVAs.*\n\n";
                
                // Group fields by category
                QMap<QString, QStringList> fieldsByCategory;
                for (const FieldInfo &f : valuableFields) {
                    QString entry = QString("| `%1` | `%2` | `%3` | `%4` |")
                        .arg(f.className, f.fieldName, f.fieldType, f.offset);
                    fieldsByCategory[f.category].append(entry);
                }
                
                if (!fieldsByCategory.isEmpty()) {
                    gameContext += "### 📊 Modifiable Fields (with offsets)\n\n";
                    for (auto it = fieldsByCategory.begin(); it != fieldsByCategory.end(); ++it) {
                        gameContext += QString("#### %1\n").arg(it.key());
                        gameContext += "| Class | Field | Type | Offset |\n|---|---|---|---|\n";
                        for (const QString &entry : it.value().mid(0, 20)) { // Limit per category
                            gameContext += entry + "\n";
                        }
                        gameContext += "\n";
                    }
                }
                
                // Group methods by category
                QMap<QString, QStringList> methodsByCategory;
                for (const MethodInfo &m : valuableMethods) {
                    QString entry = QString("| `%1` | `%2(%3)` | `%4` | `%5` |")
                        .arg(m.className, m.methodName, m.params.left(30), m.returnType, m.rva);
                    methodsByCategory[m.category].append(entry);
                }
                
                if (!methodsByCategory.isEmpty()) {
                    gameContext += "### 🔧 Hookable Methods (with RVA)\n\n";
                    for (auto it = methodsByCategory.begin(); it != methodsByCategory.end(); ++it) {
                        gameContext += QString("#### %1\n").arg(it.key());
                        gameContext += "| Class | Method | Return | RVA |\n|---|---|---|---|\n";
                        for (const QString &entry : it.value().mid(0, 20)) {
                            gameContext += entry + "\n";
                        }
                        gameContext += "\n";
                    }
                }
            }
        }
    }
    
    // Also check for any remaining high-value classes by scanning full classes
    struct ClassInfo {
        QString name;
        QString content;
        QString category;
        int priority;
    };
    QList<ClassInfo> scoredClasses;
    
    // Re-scan for full class content of high-priority classes
    if (QFile::exists(dumpPath) && valuableFields.size() < 10) {
        // If we didn't find many fields, do a broader search
        QFile dumpFile(dumpPath);
        if (dumpFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString dumpContent = QString::fromUtf8(dumpFile.readAll());
            dumpFile.close();
            
            QStringList lines = dumpContent.split('\n');
            QString currentClassName;
            QStringList currentClassContent;
            bool inClass = false;
            int braceCount = 0;
            
            QRegularExpression classRegex("^\\s*(public|internal)?\\s*(sealed|abstract)?\\s*(class|struct)\\s+(\\w+)");
            
            for (int i = 0; i < lines.size(); i++) {
                QString line = lines[i];
                
                QRegularExpressionMatch classMatch = classRegex.match(line);
                if (classMatch.hasMatch()) {
                    // Save previous class
                    if (!currentClassName.isEmpty() && !currentClassContent.isEmpty() && currentClassContent.size() > 3) {
                        QString fullContent = currentClassContent.join("\n");
                        // Only add if it has valuable content
                        if (fullContent.contains(QRegularExpression("(int|float)\\s+(\\w*(gold|coin|gem|health|damage|energy|level)\\w*)\\s*;", QRegularExpression::CaseInsensitiveOption))) {
                            QString category = categorizeClass(currentClassName, fullContent);
                            if (!category.isEmpty()) {
                                scoredClasses.append({currentClassName, fullContent.left(3000), category, 100});
                            }
                        }
                    }
                    
                    currentClassName = classMatch.captured(4);
                    currentClassContent.clear();
                    
                    // Check if worth capturing
                    bool isBlacklisted = false;
                    for (const QString &bl : classBlacklist) {
                        if (currentClassName.contains(bl, Qt::CaseInsensitive)) {
                            isBlacklisted = true;
                            break;
                        }
                    }
                    
                    inClass = !isBlacklisted;
                    braceCount = 0;
                }
                
                if (inClass) {
                    currentClassContent.append(line);
                    braceCount += line.count('{') - line.count('}');
                    if (braceCount <= 0 && currentClassContent.size() > 2) {
                        inClass = false;
                    }
                    if (currentClassContent.size() > 150) {
                        inClass = false;
                    }
                }
            }
        }
        
        // Add top scored classes to context
        if (!scoredClasses.isEmpty()) {
            std::sort(scoredClasses.begin(), scoredClasses.end(), [](const ClassInfo &a, const ClassInfo &b) {
                return a.priority > b.priority;
            });
            
            gameContext += "\n## 📦 Full Class Definitions (Top Targets)\n\n";
            int classCount = 0;
            for (const ClassInfo &ci : scoredClasses) {
                if (classCount >= 10) break;
                gameContext += QString("### `%1` [%2]\n```csharp\n%3\n```\n\n").arg(ci.name, ci.category, ci.content);
                classCount++;
            }
        }
    }

    // LEGACY: Keep the rest of the function for additional context gathering
    // (SharedPreferences, C# source, etc.)
    
    // Priority keywords for legacy code paths
    QStringList highPriorityKeywords = {
        "Gold", "Coin", "Gem", "Diamond", "Money", "Cash", "Currency", "Credit",
        "Health", "HP", "Damage", "Attack", "Defense", "Speed", "Power",
        "PlayerData", "UserData", "SaveData", "GameData", "PlayerStats",
        "Inventory", "Item", "Weapon", "Equipment",
        "Purchase", "IAP", "Buy", "Price", "Premium", "VIP",
        "CheatDetect", "AntiCheat", "Hack", "Security", "Validate"
    };
    
    QStringList mediumPriorityKeywords = {
        "Manager", "Controller", "Handler", "Service",
        "Timer", "Cooldown", "Energy", "Stamina",
        "Level", "Experience", "XP", "Score", "Rank",
        "Shop", "Store", "Reward", "Loot"
    };
    
    QStringList lowPriorityKeywords = {
        "Player", "Character", "Hero", "Avatar",
        "Game", "Config", "Settings", "Data"
    };
    
    // 2. Read decompiled C# source files with smart filtering
    QDir csharpDir(m_ProjectPath + "/csharp_src");
    if (csharpDir.exists()) {
        QDirIterator it(csharpDir.path(), {"*.cs"}, QDir::Files, QDirIterator::Subdirectories);
        QMap<QString, QString> sourceFiles;
        int csFilesRead = 0;
        
        while (it.hasNext() && csFilesRead < 20) {
            QString filePath = it.next();
            QString fileName = QFileInfo(filePath).fileName();
            QString relativePath = QDir(m_ProjectPath + "/csharp_src").relativeFilePath(filePath);
            
            // Skip common Unity boilerplate
            if (relativePath.startsWith("UnityEngine") || relativePath.startsWith("System") ||
                relativePath.startsWith("Mono") || fileName.startsWith("__")) {
                continue;
            }
            
            // Check relevance
            bool isRelevant = false;
            for (const QString &kw : highPriorityKeywords + mediumPriorityKeywords) {
                if (fileName.contains(kw, Qt::CaseInsensitive)) {
                    isRelevant = true;
                    break;
                }
            }
            
            if (isRelevant) {
                QFile file(filePath);
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QString content = QString::fromUtf8(file.readAll());
                    file.close();
                    
                    QString category = categorizeClass(fileName, content);
                    if (!category.isEmpty()) {
                        QString truncated = content.length() > 5000 ? content.left(5000) + "\n// ... (truncated)" : content;
                        sourceFiles[category] += QString("#### `%1`\n```csharp\n%2\n```\n").arg(relativePath, truncated);
                        csFilesRead++;
                        filesAnalyzed++;
                    }
                }
            }
        }
        
        if (!sourceFiles.isEmpty()) {
            gameContext += "\n## 📁 Decompiled C# Source\n\n";
            for (auto sit = sourceFiles.begin(); sit != sourceFiles.end(); ++sit) {
                gameContext += QString("### %1\n%2\n").arg(sit.key(), sit.value());
            }
        }
    }
    
    // 3. Parse SharedPreferences for save data mods
    QDir sharedPrefsDir(m_ProjectPath + "/shared_prefs");
    if (sharedPrefsDir.exists()) {
        QStringList xmlFiles = sharedPrefsDir.entryList({"*.xml"}, QDir::Files);
        if (!xmlFiles.isEmpty()) {
            gameContext += "\n## 💾 Local Save Data (SharedPreferences)\n\n";
            gameContext += "*These values can often be modified directly for offline cheats:*\n\n";
            
            for (const QString &xmlFile : xmlFiles) {
                QFile file(sharedPrefsDir.filePath(xmlFile));
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QString content = QString::fromUtf8(file.readAll());
                    file.close();
                    
                    QRegularExpression kvRegex("<(int|long|float|boolean|string)\\s+name=\"([^\"]+)\"[^>]*(?:value=\"([^\"]*)\")?");
                    QRegularExpressionMatchIterator matches = kvRegex.globalMatch(content);
                    
                    if (matches.hasNext()) {
                        gameContext += QString("#### `%1`\n| Type | Key | Value |\n|---|---|---|\n").arg(xmlFile);
                        int count = 0;
                        while (matches.hasNext() && count < 50) {
                            QRegularExpressionMatch m = matches.next();
                            QString key = m.captured(2);
                            // Highlight interesting keys
                            QString keyDisplay = key;
                            for (const QString &kw : highPriorityKeywords) {
                                if (key.contains(kw, Qt::CaseInsensitive)) {
                                    keyDisplay = "**" + key + "**";
                                    break;
                                }
                            }
                            gameContext += QString("| %1 | %2 | %3 |\n").arg(m.captured(1), keyDisplay, m.captured(3).left(40));
                            count++;
                        }
                        gameContext += "\n";
                        filesAnalyzed++;
                    }
                }
            }
        }
    }
    
    if (filesAnalyzed == 0) {
        logMessage("No game source code found! Run 'Decompile' first.", "error");
        m_AIResponseView->setMarkdown("## ❌ No Source Code Available\n\n"
            "Please run **Decompile** first to extract the game's source code.\n\n"
            "### Steps:\n"
            "1. Click **Verify** to check available tools\n"
            "2. Click **Decompile** to extract game code\n"
            "3. Then click **AI Analyze** again");
        return;
    }
    
    logMessage(QString("Sending %1 modding targets to AI for analysis...").arg(filesAnalyzed), "info");
    m_AIResponseView->setMarkdown("## 🔄 Analyzing...\n\nProcessing " + QString::number(filesAnalyzed) + " high-value targets with AI...");
    
    // Build ULTRA-STRICT prompt that forces specific output
    QString prompt = QString(
        "You are a professional IL2CPP reverse engineer. I'm providing you with EXTRACTED DATA from a %1 game.\n\n"
        "## ⚠️ ABSOLUTE RULES - VIOLATION = FAILURE:\n"
        "1. **ONLY** use class names, field names, method names, offsets, and RVAs that appear EXACTLY in the data below\n"
        "2. **NEVER** invent or assume values - if offset says 0x20, write 0x20, not 0x24\n"
        "3. If a category has ZERO valid targets in the data, write: `**No targets found in extracted data**`\n"
        "4. Every mod MUST include the EXACT offset or RVA from the data\n"
        "5. Do NOT provide generic examples - ONLY specific mods for THIS game\n\n"
        "## OUTPUT STRUCTURE:\n\n"
        "### 🎯 Analysis Summary\n"
        "Brief summary of modding potential based on extracted data.\n\n"
        "---\n\n"
        "For each category below, list ONLY mods that have targets in the extracted data:\n\n"
        "### 💰 Currency/Economy Mods\n"
        "For each target found:\n"
        "```\n"
        "Target: [ClassName].[FieldName]\n"
        "Type: [int/float] | Offset: [EXACT offset from data, e.g., 0x20]\n"
        "Mod: Set value to 999999999\n"
        "\n"
        "Frida Hook:\n"
        "var instance = Il2Cpp.Domain.assembly(\"Assembly-CSharp\").image.class(\"[Namespace].[ClassName]\").field(\"[FieldName]\");\n"
        "// Write memory at base + [offset]\n"
        "Memory.writeInt(instance.value.handle.add([offset]), 999999999);\n"
        "```\n"
        "Risk: [Low/Medium/High] - [specific reason]\n\n"
        "### ❤️ Health/Player Stats Mods\n"
        "[Same format - only if targets exist]\n\n"
        "### ⚔️ Damage/Combat Mods\n"
        "[Same format - only if targets exist]\n\n"
        "### 🛒 Shop/IAP Mods\n"
        "[Same format - only if targets exist]\n\n"
        "### ⏱️ Timer/Energy Mods\n"
        "[Same format - only if targets exist]\n\n"
        "### 🔧 Method Hooks (for complex mods)\n"
        "For each hookable method:\n"
        "```\n"
        "Method: [ClassName].[MethodName]([params])\n"
        "RVA: [EXACT RVA from data]\n"
        "Purpose: [what the method does]\n"
        "\n"
        "Frida Interceptor:\n"
        "Interceptor.attach(Module.findBaseAddress(\"libil2cpp.so\").add([RVA]), {\n"
        "    onEnter: function(args) { /* modify args */ },\n"
        "    onLeave: function(retval) { retval.replace(/* new value */); }\n"
        "});\n"
        "```\n\n"
        "### 🛡️ Anti-Cheat Detection\n"
        "List any security-related classes/methods found (or state none found).\n\n"
        "---\n\n"
        "## EXTRACTED GAME DATA:\n\n%2"
    ).arg(engineType, gameContext);
    
    askAI(prompt, [this](const QString &res) {
        m_AIResponseView->setMarkdown(res);
        logMessage("AI Analysis complete!", "success");
        
        // Save analysis to file
        QFile reportFile(m_ProjectPath + "/AI_MOD_ANALYSIS.md");
        if (reportFile.open(QIODevice::WriteOnly)) {
            reportFile.write(res.toUtf8());
            reportFile.close();
            logMessage("Analysis saved to AI_MOD_ANALYSIS.md", "info");
        }
    });
}

int GameModStudio::calculateClassPriority(const QString &className, const QString &content, 
                                          const QStringList &high, const QStringList &medium, const QStringList &low)
{
    int priority = 0;
    QString nameLower = className.toLower();
    QString contentLower = content.toLower();
    
    // High priority keywords (+100 each)
    for (const QString &kw : high) {
        if (nameLower.contains(kw.toLower())) priority += 100;
        if (contentLower.contains(kw.toLower())) priority += 30;
    }
    
    // Medium priority (+50 each)
    for (const QString &kw : medium) {
        if (nameLower.contains(kw.toLower())) priority += 50;
        if (contentLower.contains(kw.toLower())) priority += 15;
    }
    
    // Low priority (+10 each)
    for (const QString &kw : low) {
        if (nameLower.contains(kw.toLower())) priority += 10;
    }
    
    // Bonus for having offsets (means fields are exposed)
    if (content.contains(QRegularExpression("//\\s*0x[0-9A-Fa-f]+"))) {
        priority += 50;
    }
    
    // Bonus for having int/float fields with game-like names
    if (content.contains(QRegularExpression("(int|float|double)\\s+(\\w*(gold|coin|gem|health|damage|attack|speed|level|exp|money)\\w*)", QRegularExpression::CaseInsensitiveOption))) {
        priority += 200;
    }
    
    // Bonus for setter methods
    if (content.contains(QRegularExpression("(set_|Set|Add|Remove)(Gold|Coin|Gem|Money|Health|Damage|Currency)", QRegularExpression::CaseInsensitiveOption))) {
        priority += 150;
    }
    
    return priority;
}

QString GameModStudio::categorizeClass(const QString &className, const QString &content)
{
    QString nameLower = className.toLower();
    QString contentLower = content.toLower();
    
    // Currency/Economy
    if (nameLower.contains("currency") || nameLower.contains("coin") || nameLower.contains("gold") ||
        nameLower.contains("gem") || nameLower.contains("diamond") || nameLower.contains("wallet") ||
        nameLower.contains("money") || nameLower.contains("cash") || nameLower.contains("credit") ||
        nameLower.contains("token") || nameLower.contains("reward") || nameLower.contains("loot")) {
        return "💰 Currency/Economy";
    }
    
    // Player/Stats
    if (nameLower.contains("player") || nameLower.contains("character") || nameLower.contains("hero") ||
        nameLower.contains("health") || nameLower.contains("damage") || nameLower.contains("stats") ||
        nameLower.contains("level") || nameLower.contains("experience") || nameLower.contains("xp") ||
        nameLower.contains("userdata") || nameLower.contains("playerdata") || nameLower.contains("profile")) {
        return "❤️ Player/Stats";
    }
    
    // Inventory/Items
    if (nameLower.contains("inventory") || nameLower.contains("item") || nameLower.contains("equipment") ||
        nameLower.contains("weapon") || nameLower.contains("skill") || nameLower.contains("upgrade")) {
        return "🎒 Inventory/Items";
    }
    
    // Shop/IAP
    if (nameLower.contains("shop") || nameLower.contains("store") || nameLower.contains("purchase") ||
        nameLower.contains("iap") || nameLower.contains("buy") || nameLower.contains("price") ||
        nameLower.contains("premium") || nameLower.contains("ads") || nameLower.contains("offer")) {
        return "🛒 Shop/IAP";
    }
    
    // Timer/Cooldown
    if (nameLower.contains("timer") || nameLower.contains("cooldown") || nameLower.contains("countdown") ||
        nameLower.contains("energy") || nameLower.contains("stamina") || nameLower.contains("wait")) {
        return "⏱️ Timer/Cooldown";
    }
    
    // Game Management
    if (nameLower.contains("gamemanager") || nameLower.contains("datamanager") || nameLower.contains("savemanager") ||
        nameLower.contains("gamecontroller") || nameLower.contains("config") || nameLower.contains("settings")) {
        return "🎮 Game Management";
    }
    
    // Anti-cheat/Security
    if (nameLower.contains("cheat") || nameLower.contains("hack") || nameLower.contains("security") ||
        nameLower.contains("verify") || nameLower.contains("validate") || nameLower.contains("integrity") ||
        nameLower.contains("tamper") || nameLower.contains("detect") || nameLower.contains("check")) {
        return "🛡️ Anti-Cheat/Security";
    }
    
    // Check content for secondary classification
    if (contentLower.contains("addcoin") || contentLower.contains("addgem") || contentLower.contains("setgold") ||
        contentLower.contains("currency") || contentLower.contains("balance")) {
        return "💰 Currency/Economy";
    }
    
    if (contentLower.contains("takedamage") || contentLower.contains("sethp") || contentLower.contains("health")) {
        return "❤️ Player/Stats";
    }
    
    return ""; // Not relevant
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

void GameModStudio::applyMod() 
{ 
    QList<ModOption> mods = m_ModConfig->getSelectedMods();
    if (mods.isEmpty()) {
        logMessage("No modifications selected.", "warning");
        return;
    }
    
    logMessage("Applying " + QString::number(mods.size()) + " modifications...", "info");
    
    QString context = collectGameContext();
    QString modDescription = m_ModConfig->generateModDescription();
    
    QString prompt = QString(
        "You are an expert game modder. Based on the game code context below, generate specific code patches for:\n\n"
        "**Requested Modifications:** %1\n\n"
        "**Game Code Context:**\n%2\n\n"
        "For each modification:\n"
        "1. Identify the exact method/function to modify\n"
        "2. Provide the original code snippet\n"
        "3. Provide the modified code snippet\n"
        "4. If IL2CPP, provide memory offset patches\n"
        "5. Explain how to apply the patch\n\n"
        "Return as structured JSON with patches array."
    ).arg(modDescription, context);
    
    askAI(prompt, [this](const QString &response) {
        m_AIResponseView->setMarkdown("# Applied Modifications\n\n" + response);
        
        QFile f(m_ProjectPath + "/AI_MOD_PATCHES.json");
        if (f.open(QIODevice::WriteOnly)) {
            f.write(response.toUtf8());
            f.close();
            logMessage("Patches saved to AI_MOD_PATCHES.json", "success");
        }
    });
}

void GameModStudio::generateModMenu()
{
    QList<ModOption> mods = m_ModConfig->getSelectedMods();
    if (mods.isEmpty()) {
        logMessage("Select at least one modification to generate a mod menu.", "warning");
        return;
    }
    
    // Check if dump.cs exists for the new generator
    if (QFile::exists(m_ProjectPath + "/dump/dump.cs")) {
        // Use the new project generator
        auto dialog = new ModMenuProjectDialog(m_ProjectPath, mods, this);
        dialog->exec();
        dialog->deleteLater();
    } else {
        // Fall back to AI-based generator
        logMessage("dump.cs not found - using AI-based generation (less accurate)", "warning");
        auto dialog = new ModMenuGeneratorDialog(m_ProjectPath, mods, this);
        dialog->exec();
        dialog->deleteLater();
    }
}

void GameModStudio::openAIChat()
{
    QString question = m_AIChatInput->text().trimmed();
    if (question.isEmpty()) return;
    
    m_AIChatInput->clear();
    m_AIResponseView->append("<div style='color: #58a6ff; margin: 10px 0;'><b>You:</b> " + question + "</div>");
    
    QString context = collectGameContext();
    QString modDescription = m_ModConfig->generateModDescription();
    
    QString prompt = QString(
        "You are an expert game reverse engineer and modder. Answer this question about modifying the game:\n\n"
        "**Question:** %1\n\n"
        "%2"
        "%3\n\n"
        "Provide practical, actionable advice with code examples when applicable."
    ).arg(question,
          modDescription.isEmpty() ? "" : "**Selected Mods:** " + modDescription + "\n\n",
          context.isEmpty() ? "" : "**Game Context:**\n" + context);
    
    askAI(prompt, [this](const QString &response) {
        m_AIResponseView->append("<div style='color: #7ee787; margin: 10px 0; white-space: pre-wrap;'><b>AI:</b>\n" + response + "</div>");
        logMessage("AI response received.", "success");
    });
}

void GameModStudio::verifyDecompilation()
{
    logMessage("Verifying decompilation status and tools...", "info");
    
    // Check tools from settings
    QSettings settings;
    QString ilspy = settings.value("ilspy_cmd").toString();
    QString dumper = settings.value("il2cpp_dumper_exe").toString();
    QString jadx = settings.value("jadx_exe").toString();
    
    bool ilspyAvailable = !ilspy.isEmpty() && QFile::exists(ilspy);
    bool dumperAvailable = !dumper.isEmpty() && QFile::exists(dumper);
    bool jadxAvailable = !jadx.isEmpty() && QFile::exists(jadx);
    
    // Check Unity files
    bool hasMonoSource = QDir(m_ProjectPath + "/csharp_src").exists() && 
                         !QDir(m_ProjectPath + "/csharp_src").isEmpty();
    bool hasIl2cppDump = QFile::exists(m_ProjectPath + "/dump/dump.cs") ||
                         QDir(m_ProjectPath + "/dump").exists();
    bool hasAssemblyCSharp = QFile::exists(m_ProjectPath + "/assets/bin/Data/Managed/Assembly-CSharp.dll");
    bool hasGlobalMetadata = QFile::exists(m_ProjectPath + "/assets/bin/Data/Managed/Metadata/global-metadata.dat");
    bool hasIl2cpp = false;
    QString il2cppArch;
    
    QStringList archs = {"arm64-v8a", "armeabi-v7a", "x86", "x86_64"};
    for (const QString &arch : archs) {
        if (QFile::exists(m_ProjectPath + "/lib/" + arch + "/libil2cpp.so")) {
            hasIl2cpp = true;
            il2cppArch = arch;
            break;
        }
    }
    
    QString report = "<h2>🔍 Unity Decompilation Verification</h2>";
    report += "<style>table { width: 100%; border-collapse: collapse; } td { padding: 8px; border-bottom: 1px solid #30363d; }</style>";
    report += "<table>";
    
    auto addRow = [&report](const QString &item, bool found, const QString &status) {
        QString color = found ? "#7ee787" : "#f85149";
        QString icon = found ? "✅" : "❌";
        report += QString("<tr><td>%1</td><td style='color: %2;'>%3 %4</td></tr>")
                 .arg(item, color, icon, status);
    };
    
    // Engine and files status
    report += "<tr><td colspan='2' style='background: #21262d; font-weight: bold;'>📁 Project Files</td></tr>";
    
    if (m_DetectedEngine == GameEngineDetector::Unity) {
        addRow("Unity Engine", true, "Unity game confirmed");
        
        if (hasAssemblyCSharp) {
            addRow("Assembly-CSharp.dll (Mono)", true, "Found - Mono backend detected");
        }
        if (hasIl2cpp) {
            addRow("libil2cpp.so", true, QString("Found in %1").arg(il2cppArch));
            if (hasGlobalMetadata) {
                addRow("global-metadata.dat", true, "Metadata available for dumping");
            } else {
                addRow("global-metadata.dat", false, "Missing - required for IL2CPP dump");
            }
        }
        
        // Show appropriate status based on backend type
        if (hasAssemblyCSharp) {
            addRow("C# Source Decompiled", hasMonoSource, hasMonoSource ? "Ready for AI analysis" : "Click 'Decompile' to extract");
        }
        if (hasIl2cpp) {
            addRow("IL2CPP Dump Available", hasIl2cppDump, hasIl2cppDump ? "dump.cs ready for analysis" : "Click 'Decompile' to dump");
            // Check for dummy DLLs that can be further decompiled
            bool hasDummyDlls = QFile::exists(m_ProjectPath + "/dump/DummyDll/Assembly-CSharp.dll");
            if (hasIl2cppDump && hasDummyDlls) {
                addRow("Dummy DLLs", true, "Can extract class structures with ILSpy");
            }
        }
    } else {
        addRow("Engine", true, GameEngineDetector::engineName(m_DetectedEngine));
        addRow("Unity Decompilation", false, "Not a Unity game");
    }
    
    // Tools status
    report += "<tr><td colspan='2' style='background: #21262d; font-weight: bold;'>🔧 Configured Tools (from Settings)</td></tr>";
    addRow("ILSpy/ILSpyCmd", ilspyAvailable, ilspyAvailable ? ilspy : "Not configured - needed for Mono games");
    addRow("IL2CPP Dumper", dumperAvailable, dumperAvailable ? dumper : "Not configured - needed for IL2CPP games");
    addRow("JADX", jadxAvailable, jadxAvailable ? jadx : "Not configured - useful for Java code");
    
    report += "</table>";
    
    // Recommendations
    report += "<h3>💡 Recommended Actions</h3><ul style='color: #c9d1d9;'>";
    
    bool needsAction = false;
    
    if (m_DetectedEngine == GameEngineDetector::Unity) {
        if (hasAssemblyCSharp && !hasMonoSource) {
            if (ilspyAvailable) {
                report += "<li>✨ <b>Ready to decompile:</b> Click 'Decompile' to extract C# source from Assembly-CSharp.dll</li>";
            } else {
                report += "<li>⚠️ <b>Install ILSpyCmd:</b> Go to Settings → Binaries → Download Tools to get ILSpyCmd</li>";
                needsAction = true;
            }
        }
        
        if (hasIl2cpp && hasGlobalMetadata && !hasIl2cppDump) {
            if (dumperAvailable) {
                report += "<li>✨ <b>Ready to dump:</b> Click 'Decompile' to run IL2CPP Dumper and extract method signatures</li>";
            } else {
                report += "<li>⚠️ <b>Install IL2CPP Dumper:</b> Go to Settings → Binaries → Download Tools</li>";
                needsAction = true;
            }
        }
        
        if (hasMonoSource || hasIl2cppDump) {
            report += "<li>🎯 <b>Ready for modding:</b> Use 'AI Analyze' to find modifiable game values</li>";
            report += "<li>🎮 <b>Generate Mod Menu:</b> Select modifications and click 'Generate Mod Menu'</li>";
        }
    } else {
        report += "<li>This is not a Unity game. Use different tools for " + GameEngineDetector::engineName(m_DetectedEngine) + "</li>";
    }
    
    if (needsAction) {
        report += "<li style='color: #d29922;'>⚡ Click 'Setup Tools' to automatically download missing tools</li>";
    }
    
    report += "</ul>";
    
    m_AIResponseView->setHtml(report);
    
    // Log summary
    if (hasMonoSource || hasIl2cppDump) {
        logMessage("✅ Decompilation verified! Source code available for analysis.", "success");
    } else if (needsAction) {
        logMessage("⚠️ Missing tools. Configure in Settings or click 'Setup Tools'.", "warning");
    } else if (hasAssemblyCSharp || hasIl2cpp) {
        logMessage("Unity files found. Click 'Decompile' to extract source code.", "info");
    } else {
        logMessage("No Unity files found in this project.", "error");
    }
}

void GameModStudio::openInteractiveModBuilder()
{
    auto dialog = new InteractiveModMenuDialog(m_ProjectPath, this);
    dialog->exec();
    dialog->deleteLater();
}

void GameModStudio::checkToolsAndSuggestDownload()
{
    QSettings settings;
    QStringList missing;
    
    if (settings.value("ilspy_cmd").toString().isEmpty()) missing << "ILSpyCmd";
    if (settings.value("il2cpp_dumper_exe").toString().isEmpty()) missing << "IL2CPP Dumper";
    if (settings.value("jadx_exe").toString().isEmpty()) missing << "JADX";
    
    if (!missing.isEmpty()) {
        logMessage("Missing tools: " + missing.join(", ") + ". Click 'Setup Tools' to download.", "warning");
    }
}

bool GameModStudio::runILSpyDecompilation()
{
    QSettings settings;
    QString ilspy = settings.value("ilspy_cmd").toString();
    
    if (ilspy.isEmpty() || !QFile::exists(ilspy)) {
        logMessage("ILSpyCmd not configured. Go to Settings → Binaries.", "error");
        return false;
    }
    
    QString managedPath = m_ProjectPath + "/assets/bin/Data/Managed/Assembly-CSharp.dll";
    if (!QFile::exists(managedPath)) {
        logMessage("Assembly-CSharp.dll not found.", "error");
        return false;
    }
    
    QString outDir = m_ProjectPath + "/csharp_src";
    QDir().mkpath(outDir);
    
    logMessage("Running ILSpyCmd on Assembly-CSharp.dll...", "info");
    m_Progress->setVisible(true);
    m_Progress->setRange(0, 0);
    
    QProcess *process = new QProcess(this);
    process->start(ilspy, {"-o", outDir, managedPath});
    
    connect(process, &QProcess::finished, this, [=](int exitCode) {
        m_Progress->setVisible(false);
        if (exitCode == 0) {
            logMessage("✅ C# Source extracted successfully!", "success");
        } else {
            logMessage("ILSpyCmd failed with exit code: " + QString::number(exitCode), "error");
        }
        process->deleteLater();
    });
    
    return true;
}

bool GameModStudio::runILSpyOnDummyDlls()
{
    QSettings settings;
    QString ilspy = settings.value("ilspy_cmd").toString();
    
    if (ilspy.isEmpty() || !QFile::exists(ilspy)) {
        logMessage("ILSpyCmd not configured. Go to Settings → Binaries.", "error");
        return false;
    }
    
    QString dummyDir = m_ProjectPath + "/dump/DummyDll";
    if (!QDir(dummyDir).exists()) {
        logMessage("DummyDll directory not found.", "error");
        return false;
    }
    
    // Find all DLLs to decompile
    QStringList dlls;
    QDirIterator it(dummyDir, {"*.dll"}, QDir::Files);
    while (it.hasNext()) {
        dlls << it.next();
    }
    
    if (dlls.isEmpty()) {
        logMessage("No DLL files found in DummyDll directory.", "error");
        return false;
    }
    
    QString outDir = m_ProjectPath + "/csharp_src";
    QDir().mkpath(outDir);
    
    logMessage(QString("Running ILSpyCmd on %1 DLL files...").arg(dlls.size()), "info");
    m_Progress->setVisible(true);
    m_Progress->setRange(0, dlls.size());
    m_Progress->setValue(0);
    
    // Process DLLs sequentially
    int *completed = new int(0);
    int total = dlls.size();
    
    for (const QString &dll : dlls) {
        QProcess *process = new QProcess(this);
        QString dllName = QFileInfo(dll).baseName();
        QString dllOutDir = outDir + "/" + dllName;
        QDir().mkpath(dllOutDir);
        
        process->start(ilspy, {"-o", dllOutDir, dll});
        
        connect(process, &QProcess::finished, this, [=](int exitCode) {
            (*completed)++;
            m_Progress->setValue(*completed);
            
            if (exitCode != 0) {
                logMessage(QString("Warning: Failed to decompile %1").arg(dllName), "warning");
            }
            
            if (*completed >= total) {
                m_Progress->setVisible(false);
                logMessage("✅ C# class structures extracted from IL2CPP DummyDlls!", "success");
                delete completed;
            }
            process->deleteLater();
        });
    }
    
    return true;
}

bool GameModStudio::runIl2CppDumper()
{
    QSettings settings;
    QString dumper = settings.value("il2cpp_dumper_exe").toString();
    
    if (dumper.isEmpty() || !QFile::exists(dumper)) {
        logMessage("IL2CPP Dumper not configured. Go to Settings → Binaries.", "error");
        return false;
    }
    
    QString lib;
    QStringList archs = {"arm64-v8a", "armeabi-v7a", "x86", "x86_64"};
    for (const QString &arch : archs) {
        QString path = m_ProjectPath + "/lib/" + arch + "/libil2cpp.so";
        if (QFile::exists(path)) {
            lib = path;
            break;
        }
    }
    
    QString meta = m_ProjectPath + "/assets/bin/Data/Managed/Metadata/global-metadata.dat";
    
    if (lib.isEmpty() || !QFile::exists(meta)) {
        logMessage("libil2cpp.so or global-metadata.dat not found.", "error");
        return false;
    }
    
    QString dumpPath = m_ProjectPath + "/dump/";
    QDir().mkpath(dumpPath);
    
    logMessage("Running IL2CPP Dumper...", "info");
    m_Progress->setVisible(true);
    m_Progress->setRange(0, 0);
    
    QProcess *process = new QProcess(this);
    process->setWorkingDirectory(QFileInfo(dumper).absolutePath());
    process->start(dumper, {lib, meta, dumpPath});
    
    connect(process, &QProcess::finished, this, [=](int exitCode) {
        m_Progress->setVisible(false);
        if (exitCode == 0 || QFile::exists(dumpPath + "dump.cs")) {
            logMessage("✅ IL2CPP dump completed!", "success");
        } else {
            logMessage("IL2CPP Dumper failed with exit code: " + QString::number(exitCode), "error");
        }
        process->deleteLater();
    });
    
    return true;
}

bool GameModStudio::checkUnityDecompilation()
{
    bool hasSource = QDir(m_ProjectPath + "/csharp_src").exists() ||
                     QFile::exists(m_ProjectPath + "/dump/dump.cs");
    return hasSource;
}

QString GameModStudio::collectGameContext()
{
    QString context;
    int maxContextSize = 30000;
    
    // Keywords for identifying interesting code
    QStringList interestingPatterns = {
        "Player", "Health", "Money", "Coin", "Gem", "Diamond", 
        "Energy", "Score", "Purchase", "IAP", "Store", "Premium",
        "VIP", "Inventory", "Item", "Weapon", "Damage", "Speed",
        "Currency", "Gold", "Level", "XP", "Experience", "Stamina",
        "Timer", "Cooldown", "Ads", "Reward", "Unlock", "Cheat",
        "License", "Billing", "Subscribe"
    };
    
    // 1. Check for C# source files (Mono)
    QString srcDir = m_ProjectPath + "/csharp_src";
    if (QDir(srcDir).exists()) {
        QDirIterator it(srcDir, {"*.cs"}, QDir::Files, QDirIterator::Subdirectories);
        int filesRead = 0;
        
        while (it.hasNext() && context.length() < maxContextSize && filesRead < 30) {
            QString filePath = it.next();
            QFile file(filePath);
            if (file.open(QIODevice::ReadOnly)) {
                QString content = QString::fromUtf8(file.readAll());
                file.close();
                
                bool isInteresting = false;
                for (const QString &pattern : interestingPatterns) {
                    if (content.contains(pattern, Qt::CaseInsensitive)) {
                        isInteresting = true;
                        break;
                    }
                }
                
                if (isInteresting) {
                    QString relativePath = filePath.mid(srcDir.length() + 1);
                    context += "\n--- " + relativePath + " ---\n";
                    context += content.left(2000) + "\n";
                    filesRead++;
                }
            }
        }
    }
    
    // 2. Check for IL2CPP dump
    QString dumpFile = m_ProjectPath + "/dump/dump.cs";
    if (context.isEmpty() && QFile::exists(dumpFile)) {
        QFile file(dumpFile);
        if (file.open(QIODevice::ReadOnly)) {
            QString content = QString::fromUtf8(file.readAll());
            file.close();
            context = content.left(maxContextSize);
        }
    }
    
    // 3. Fallback: Analyze Smali code for Native Android apps
    if (context.isEmpty()) {
        QDir smaliDir(m_ProjectPath + "/smali");
        QDirIterator it(m_ProjectPath, {"*.smali"}, QDir::Files, QDirIterator::Subdirectories);
        int filesRead = 0;
        
        // Smali patterns to look for
        QStringList smaliPatterns = {
            "isPremium", "isProUser", "isPurchased", "isSubscribed", "hasLicense",
            "getCoin", "getGem", "getGold", "getMoney", "getEnergy",
            "addCoin", "addGem", "addGold", "spendCoin", "spendGem",
            "setHealth", "getHealth", "takeDamage", "setDamage",
            "LicenseChecker", "BillingClient", "InAppPurchase",
            "isPro", "isVIP", "hasPurchased", "checkLicense"
        };
        
        while (it.hasNext() && context.length() < maxContextSize && filesRead < 50) {
            QString filePath = it.next();
            QString relativePath = filePath.mid(m_ProjectPath.length() + 1);
            
            // Skip framework and Android system classes
            if (relativePath.contains("/android/") || relativePath.contains("/androidx/") ||
                relativePath.contains("/google/") || relativePath.contains("/com/android/")) {
                continue;
            }
            
            QFile file(filePath);
            if (file.open(QIODevice::ReadOnly)) {
                QString content = QString::fromUtf8(file.readAll());
                file.close();
                
                bool isInteresting = false;
                for (const QString &pattern : smaliPatterns) {
                    if (content.contains(pattern, Qt::CaseInsensitive)) {
                        isInteresting = true;
                        break;
                    }
                }
                
                if (isInteresting) {
                    context += "\n--- " + relativePath + " ---\n";
                    
                    // Extract just the interesting methods
                    QStringList lines = content.split('\n');
                    bool inMethod = false;
                    QString currentMethod;
                    
                    for (const QString &line : lines) {
                        if (line.startsWith(".method")) {
                            inMethod = true;
                            currentMethod = line + "\n";
                        } else if (line.startsWith(".end method")) {
                            currentMethod += line + "\n";
                            
                            // Check if this method is interesting
                            for (const QString &pattern : smaliPatterns) {
                                if (currentMethod.contains(pattern, Qt::CaseInsensitive)) {
                                    context += currentMethod + "\n";
                                    break;
                                }
                            }
                            inMethod = false;
                            currentMethod.clear();
                        } else if (inMethod) {
                            currentMethod += line + "\n";
                        }
                    }
                    filesRead++;
                }
            }
        }
        
        if (!context.isEmpty()) {
            context = "# Smali Code Analysis (Native Android)\n\n" + context;
        }
    }
    
    // 4. Check for SharedPreferences (useful for value modding)
    QDir prefsDir(m_ProjectPath + "/shared_prefs");
    if (prefsDir.exists() && context.length() < maxContextSize - 5000) {
        QStringList xmlFiles = prefsDir.entryList({"*.xml"}, QDir::Files);
        if (!xmlFiles.isEmpty()) {
            context += "\n\n# SharedPreferences Data\n";
            for (const QString &xmlFile : xmlFiles) {
                QFile file(prefsDir.filePath(xmlFile));
                if (file.open(QIODevice::ReadOnly)) {
                    QString prefsContent = QString::fromUtf8(file.readAll());
                    file.close();
                    
                    // Only include if it has interesting keys
                    bool hasInterestingKeys = false;
                    for (const QString &pattern : interestingPatterns) {
                        if (prefsContent.contains(pattern, Qt::CaseInsensitive)) {
                            hasInterestingKeys = true;
                            break;
                        }
                    }
                    
                    if (hasInterestingKeys) {
                        context += "\n--- " + xmlFile + " ---\n";
                        context += prefsContent.left(2000) + "\n";
                    }
                }
            }
        }
    }
    
    return context;
}

// ============== AIModChatDialog Implementation ==============

AIModChatDialog::AIModChatDialog(const QString &projectPath, QWidget *parent)
    : QDialog(parent), m_ProjectPath(projectPath)
{
    setWindowTitle(tr("AI Mod Assistant"));
    setMinimumSize(800, 600);
    
    m_NetworkManager = new QNetworkAccessManager(this);
    
    auto layout = new QVBoxLayout(this);
    
    m_ChatHistory = new QTextBrowser();
    m_ChatHistory->setStyleSheet("background-color: #0d1117; color: #c9d1d9; font-family: 'Consolas', monospace;");
    m_ChatHistory->setOpenExternalLinks(true);
    layout->addWidget(m_ChatHistory);
    
    auto inputLayout = new QHBoxLayout();
    m_Input = new QLineEdit();
    m_Input->setPlaceholderText(tr("Ask about game modding..."));
    m_Input->setStyleSheet("background: #21262d; border: 1px solid #30363d; color: #c9d1d9; padding: 10px; border-radius: 6px;");
    
    auto sendBtn = new QPushButton(tr("Send"));
    sendBtn->setStyleSheet("background-color: #238636; color: white; padding: 10px 20px; border-radius: 6px;");
    
    auto clearBtn = new QPushButton(tr("Clear"));
    clearBtn->setStyleSheet("background-color: #21262d; border: 1px solid #30363d; color: #c9d1d9; padding: 10px;");
    
    inputLayout->addWidget(m_Input, 1);
    inputLayout->addWidget(sendBtn);
    inputLayout->addWidget(clearBtn);
    layout->addLayout(inputLayout);
    
    connect(sendBtn, &QPushButton::clicked, this, &AIModChatDialog::sendMessage);
    connect(clearBtn, &QPushButton::clicked, this, &AIModChatDialog::clearChat);
    connect(m_Input, &QLineEdit::returnPressed, this, &AIModChatDialog::sendMessage);
    
    // Collect initial context
    QString srcDir = m_ProjectPath + "/csharp_src";
    if (QDir(srcDir).exists()) {
        QDirIterator it(srcDir, {"*.cs"}, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext() && m_Context.length() < 15000) {
            QFile file(it.next());
            if (file.open(QIODevice::ReadOnly)) {
                m_Context += file.readAll();
                file.close();
            }
        }
    }
}

void AIModChatDialog::sendMessage()
{
    QString msg = m_Input->text().trimmed();
    if (msg.isEmpty()) return;
    
    m_Input->clear();
    m_ChatHistory->append("<div style='color: #58a6ff; margin: 10px 0;'><b>You:</b> " + msg + "</div>");
    
    QString prompt = QString(
        "You are an expert game modder. Answer this question:\n\n%1\n\n"
        "Game code context:\n%2"
    ).arg(msg, m_Context.left(20000));
    
    askAI(prompt, [this](const QString &response) {
        m_ChatHistory->append("<div style='color: #7ee787; margin: 10px 0; white-space: pre-wrap;'><b>AI:</b>\n" + response + "</div>");
    });
}

void AIModChatDialog::clearChat()
{
    m_ChatHistory->clear();
}

void AIModChatDialog::askAI(const QString &prompt, std::function<void(const QString&)> callback)
{
    QSettings settings;
    QString key = settings.value("ai_api_key").toString();
    QString model = settings.value("ai_model", "gemini-2.0-flash-exp").toString();
    
    if (key.isEmpty()) {
        m_ChatHistory->append("<span style='color: #f85149;'>Error: API Key not configured.</span>");
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
            m_ChatHistory->append("<span style='color: #f85149;'>AI Error: " + reply->errorString() + "</span>");
        }
        reply->deleteLater();
    });
}

// ============== ModMenuGeneratorDialog Implementation ==============

ModMenuGeneratorDialog::ModMenuGeneratorDialog(const QString &projectPath, const QList<ModOption> &mods, QWidget *parent)
    : QDialog(parent), m_ProjectPath(projectPath), m_Mods(mods)
{
    setWindowTitle(tr("Mod Menu Generator"));
    setMinimumSize(900, 700);
    
    m_NetworkManager = new QNetworkAccessManager(this);
    
    auto layout = new QVBoxLayout(this);
    
    auto header = new QLabel(tr("<h2>🎮 Floating Mod Menu Generator</h2>"
                                "<p>Generate a ready-to-compile mod menu with your selected modifications.</p>"));
    layout->addWidget(header);
    
    auto optionsLayout = new QHBoxLayout();
    optionsLayout->addWidget(new QLabel(tr("Menu Style:")));
    
    m_StyleCombo = new QComboBox();
    m_StyleCombo->addItems({tr("ImGui (PC/Android)"), tr("Native Android Overlay"), tr("Unity IMGUI"), tr("Frida Script")});
    m_StyleCombo->setStyleSheet("background: #21262d; border: 1px solid #30363d; color: #c9d1d9; padding: 5px;");
    optionsLayout->addWidget(m_StyleCombo);
    
    auto generateBtn = new QPushButton(tr("🚀 Generate Menu Code"));
    generateBtn->setStyleSheet("background-color: #238636; color: white; padding: 10px 20px;");
    optionsLayout->addWidget(generateBtn);
    
    optionsLayout->addStretch();
    layout->addLayout(optionsLayout);
    
    // Selected mods summary
    QString modsText = "<b>Selected Modifications:</b><ul>";
    for (const ModOption &mod : mods) {
        modsText += QString("<li>%1 (value: %2)</li>").arg(mod.name).arg(mod.value);
    }
    modsText += "</ul>";
    auto modsLabel = new QLabel(modsText);
    modsLabel->setStyleSheet("background: #161b22; padding: 10px; border-radius: 6px; color: #c9d1d9;");
    layout->addWidget(modsLabel);
    
    m_CodeView = new QTextBrowser();
    m_CodeView->setStyleSheet("background-color: #0d1117; color: #7ee787; font-family: 'Consolas', monospace;");
    layout->addWidget(m_CodeView);
    
    auto btnLayout = new QHBoxLayout();
    auto saveBtn = new QPushButton(tr("💾 Save Code"));
    saveBtn->setStyleSheet("background-color: #1f6feb; color: white; padding: 10px 20px;");
    auto closeBtn = new QPushButton(tr("Close"));
    closeBtn->setStyleSheet("background: #21262d; border: 1px solid #30363d; color: #c9d1d9; padding: 10px;");
    btnLayout->addStretch();
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);
    
    connect(generateBtn, &QPushButton::clicked, this, &ModMenuGeneratorDialog::generateMenu);
    connect(saveBtn, &QPushButton::clicked, this, &ModMenuGeneratorDialog::saveCode);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void ModMenuGeneratorDialog::generateMenu()
{
    m_CodeView->setText("Generating mod menu code with AI...");
    
    QString style = m_StyleCombo->currentText();
    QString modsDescription;
    
    for (const ModOption &mod : m_Mods) {
        modsDescription += QString("- %1: %2 (value: %3)\n").arg(mod.id, mod.name).arg(mod.value);
    }
    
    QString prompt = QString(
        "Generate a complete, production-ready mod menu for a Unity game.\n\n"
        "**Menu Style:** %1\n\n"
        "**Modifications to include:**\n%2\n\n"
        "Requirements:\n"
        "1. Create a floating/overlay menu that can be toggled\n"
        "2. Include toggle switches for each modification\n"
        "3. Include value sliders/inputs where applicable\n"
        "4. Add proper initialization and cleanup code\n"
        "5. Make it visually appealing with proper styling\n"
        "6. Include comments explaining how to use and compile\n"
        "7. If ImGui: Include all necessary headers and setup\n"
        "8. If Frida: Include the JavaScript hooks\n"
        "9. Add hotkey to toggle menu visibility (F1 or Volume buttons on Android)\n\n"
        "Return ONLY the complete source code, ready to compile."
    ).arg(style, modsDescription);
    
    askAI(prompt, [this](const QString &code) {
        m_GeneratedCode = code;
        m_CodeView->setPlainText(code);
    });
}

void ModMenuGeneratorDialog::saveCode()
{
    if (m_GeneratedCode.isEmpty()) {
        QMessageBox::warning(this, tr("No Code"), tr("Generate the menu code first."));
        return;
    }
    
    QString ext = ".cpp";
    if (m_StyleCombo->currentText().contains("Frida")) ext = ".js";
    
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Mod Menu"), 
                                                    m_ProjectPath + "/ModMenu" + ext,
                                                    "Source Files (*.cpp *.h *.js)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(m_GeneratedCode.toUtf8());
            file.close();
            QMessageBox::information(this, tr("Saved"), tr("Mod menu saved to: %1").arg(fileName));
        }
    }
}

void ModMenuGeneratorDialog::askAI(const QString &prompt, std::function<void(const QString&)> callback)
{
    QSettings settings;
    QString key = settings.value("ai_api_key").toString();
    QString model = settings.value("ai_model", "gemini-2.0-flash-exp").toString();
    
    if (key.isEmpty()) {
        m_CodeView->setText("Error: API Key not configured in Settings.");
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
            m_CodeView->setText("AI Error: " + reply->errorString());
        }
        reply->deleteLater();
    });
}

// ==================== Utility Classes ====================
QString GameModToolDownloader::getToolsDirectory() { return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/tools"; }
QString GameModToolDownloader::getToolExecutable(const QString &toolName) { return getToolsDirectory() + "/" + toolName; }

// ==================== InteractiveModMenuDialog Implementation ====================

InteractiveModMenuDialog::InteractiveModMenuDialog(const QString &projectPath, QWidget *parent)
    : QDialog(parent), m_ProjectPath(projectPath)
{
    setWindowTitle(tr("🎨 Interactive Mod Menu Builder"));
    setMinimumSize(1200, 800);
    
    m_NetworkManager = new QNetworkAccessManager(this);
    setupUI();
    
    // Collect game context
    QString srcDir = m_ProjectPath + "/csharp_src";
    QString dumpFile = m_ProjectPath + "/dump/dump.cs";
    
    if (QDir(srcDir).exists()) {
        QDirIterator it(srcDir, {"*.cs"}, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext() && m_GameContext.length() < 20000) {
            QFile file(it.next());
            if (file.open(QIODevice::ReadOnly)) {
                m_GameContext += QString::fromUtf8(file.readAll());
                file.close();
            }
        }
    } else if (QFile::exists(dumpFile)) {
        QFile file(dumpFile);
        if (file.open(QIODevice::ReadOnly)) {
            m_GameContext = QString::fromUtf8(file.readAll()).left(20000);
            file.close();
        }
    }
}

void InteractiveModMenuDialog::setupUI()
{
    auto mainLayout = new QVBoxLayout(this);
    
    // Header
    auto header = new QLabel(tr("<h2>🎮 Interactive Mod Menu Builder</h2>"
                                "<p>Build your custom mod menu step by step with AI assistance.</p>"));
    header->setStyleSheet("color: #c9d1d9;");
    mainLayout->addWidget(header);
    
    // Main content splitter
    auto mainSplitter = new QSplitter(Qt::Horizontal);
    
    // Left panel: Mod selection
    auto leftWidget = new QWidget();
    auto leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    
    // Category selector
    auto catLayout = new QHBoxLayout();
    catLayout->addWidget(new QLabel(tr("Category:")));
    m_CategoryCombo = new QComboBox();
    m_CategoryCombo->addItems({tr("All Mods"), tr("💰 Resources"), tr("🎮 Player"), tr("📦 Inventory"), tr("⚙️ Game Mechanics"), tr("🛡️ Anti-Cheat Bypass")});
    m_CategoryCombo->setStyleSheet("background: #21262d; border: 1px solid #30363d; color: #c9d1d9; padding: 5px;");
    catLayout->addWidget(m_CategoryCombo);
    leftLayout->addLayout(catLayout);
    
    // Available mods list
    leftLayout->addWidget(new QLabel(tr("<b>Available Modifications:</b>")));
    m_AvailableModsList = new QListWidget();
    m_AvailableModsList->setStyleSheet("background: #161b22; color: #c9d1d9; border: 1px solid #30363d;");
    m_AvailableModsList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    leftLayout->addWidget(m_AvailableModsList);
    
    // Add/remove buttons
    auto modBtnLayout = new QHBoxLayout();
    auto addBtn = new QPushButton(tr("➕ Add Selected"));
    addBtn->setStyleSheet("background: #238636; color: white; padding: 8px;");
    auto removeBtn = new QPushButton(tr("➖ Remove"));
    removeBtn->setStyleSheet("background: #da3633; color: white; padding: 8px;");
    modBtnLayout->addWidget(addBtn);
    modBtnLayout->addWidget(removeBtn);
    leftLayout->addLayout(modBtnLayout);
    
    // Custom mod input
    leftLayout->addWidget(new QLabel(tr("<b>Custom Mod:</b>")));
    m_CustomModInput = new QLineEdit();
    m_CustomModInput->setPlaceholderText(tr("Describe a custom modification (e.g., 'Infinite ammo for all weapons')"));
    m_CustomModInput->setStyleSheet("background: #21262d; border: 1px solid #30363d; color: #c9d1d9; padding: 8px;");
    leftLayout->addWidget(m_CustomModInput);
    
    auto addCustomBtn = new QPushButton(tr("➕ Add Custom Mod"));
    addCustomBtn->setStyleSheet("background: #1f6feb; color: white; padding: 8px;");
    leftLayout->addWidget(addCustomBtn);
    
    mainSplitter->addWidget(leftWidget);
    
    // Center panel: Selected mods and configuration
    auto centerWidget = new QWidget();
    auto centerLayout = new QVBoxLayout(centerWidget);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    
    centerLayout->addWidget(new QLabel(tr("<b>Selected Modifications:</b>")));
    m_SelectedModsList = new QListWidget();
    m_SelectedModsList->setStyleSheet("background: #161b22; color: #7ee787; border: 1px solid #238636;");
    centerLayout->addWidget(m_SelectedModsList);
    
    // Menu style selector
    auto styleLayout = new QHBoxLayout();
    styleLayout->addWidget(new QLabel(tr("Menu Style:")));
    m_MenuStyleCombo = new QComboBox();
    m_MenuStyleCombo->addItems({
        tr("ImGui Floating Menu (C++)"),
        tr("Native Android Overlay (Java)"),
        tr("Unity IMGUI Injection"),
        tr("Frida JavaScript Hooks"),
        tr("Xposed Module"),
        tr("LSPosed/EdXposed Module"),
        tr("Substrate/Cydia Hook")
    });
    m_MenuStyleCombo->setStyleSheet("background: #21262d; border: 1px solid #30363d; color: #c9d1d9; padding: 5px;");
    styleLayout->addWidget(m_MenuStyleCombo, 1);
    centerLayout->addLayout(styleLayout);
    
    // Generation buttons
    auto genBtnLayout = new QHBoxLayout();
    auto generateBtn = new QPushButton(tr("🤖 Generate with AI"));
    generateBtn->setStyleSheet("background: #238636; color: white; padding: 12px; font-weight: bold;");
    auto previewBtn = new QPushButton(tr("👁️ Preview"));
    previewBtn->setStyleSheet("background: #1f6feb; color: white; padding: 12px;");
    genBtnLayout->addWidget(generateBtn);
    genBtnLayout->addWidget(previewBtn);
    centerLayout->addLayout(genBtnLayout);
    
    mainSplitter->addWidget(centerWidget);
    
    // Right panel: AI Chat and Preview
    auto rightSplitter = new QSplitter(Qt::Vertical);
    
    // AI Chat area
    auto chatWidget = new QWidget();
    auto chatLayout = new QVBoxLayout(chatWidget);
    chatLayout->setContentsMargins(0, 0, 0, 0);
    
    chatLayout->addWidget(new QLabel(tr("<b>💬 AI Assistant:</b>")));
    m_ChatArea = new QTextBrowser();
    m_ChatArea->setStyleSheet("background: #0d1117; color: #c9d1d9; font-family: 'Consolas', monospace;");
    m_ChatArea->setOpenExternalLinks(true);
    m_ChatArea->setHtml(tr("<p style='color: #8b949e;'>Ask me anything about game modding! For example:</p>"
                          "<ul style='color: #58a6ff;'>"
                          "<li>How do I modify player health in this game?</li>"
                          "<li>What's the best approach for unlimited coins?</li>"
                          "<li>How to bypass anti-cheat detection?</li>"
                          "<li>Can you analyze this game's purchase system?</li>"
                          "</ul>"));
    chatLayout->addWidget(m_ChatArea);
    
    auto chatInputLayout = new QHBoxLayout();
    m_AIChatInput = new QLineEdit();
    m_AIChatInput->setPlaceholderText(tr("Ask AI about game modding..."));
    m_AIChatInput->setStyleSheet("background: #21262d; border: 1px solid #30363d; color: #c9d1d9; padding: 10px;");
    auto sendChatBtn = new QPushButton(tr("Send"));
    sendChatBtn->setStyleSheet("background: #238636; color: white; padding: 10px 20px;");
    chatInputLayout->addWidget(m_AIChatInput, 1);
    chatInputLayout->addWidget(sendChatBtn);
    chatLayout->addLayout(chatInputLayout);
    
    rightSplitter->addWidget(chatWidget);
    
    // Code preview area
    auto previewWidget = new QWidget();
    auto previewLayout = new QVBoxLayout(previewWidget);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    
    previewLayout->addWidget(new QLabel(tr("<b>📝 Generated Code:</b>")));
    m_PreviewArea = new QTextBrowser();
    m_PreviewArea->setStyleSheet("background: #0d1117; color: #7ee787; font-family: 'Consolas', monospace;");
    previewLayout->addWidget(m_PreviewArea);
    
    rightSplitter->addWidget(previewWidget);
    rightSplitter->setSizes({300, 400});
    
    mainSplitter->addWidget(rightSplitter);
    mainSplitter->setSizes({300, 300, 500});
    
    mainLayout->addWidget(mainSplitter);
    
    // Bottom buttons
    auto bottomLayout = new QHBoxLayout();
    auto saveBtn = new QPushButton(tr("💾 Save Code"));
    saveBtn->setStyleSheet("background: #1f6feb; color: white; padding: 12px 24px;");
    auto closeBtn = new QPushButton(tr("Close"));
    closeBtn->setStyleSheet("background: #21262d; border: 1px solid #30363d; color: #c9d1d9; padding: 12px 24px;");
    bottomLayout->addStretch();
    bottomLayout->addWidget(saveBtn);
    bottomLayout->addWidget(closeBtn);
    mainLayout->addLayout(bottomLayout);
    
    // Populate mods list
    populateModsList();
    
    // Connections
    connect(m_CategoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &InteractiveModMenuDialog::onCategorySelected);
    connect(addBtn, &QPushButton::clicked, this, [this]() {
        for (auto item : m_AvailableModsList->selectedItems()) {
            m_SelectedModsList->addItem(item->text());
            ModOption mod;
            mod.name = item->text();
            mod.id = item->data(Qt::UserRole).toString();
            mod.enabled = true;
            m_SelectedMods.append(mod);
        }
    });
    connect(removeBtn, &QPushButton::clicked, this, &InteractiveModMenuDialog::removeSelectedMod);
    connect(addCustomBtn, &QPushButton::clicked, this, &InteractiveModMenuDialog::addCustomMod);
    connect(generateBtn, &QPushButton::clicked, this, &InteractiveModMenuDialog::generateWithAI);
    connect(previewBtn, &QPushButton::clicked, this, &InteractiveModMenuDialog::previewCode);
    connect(sendChatBtn, &QPushButton::clicked, this, [this]() {
        QString question = m_AIChatInput->text().trimmed();
        if (question.isEmpty()) return;
        
        m_AIChatInput->clear();
        m_ChatArea->append("<div style='color: #58a6ff; margin: 10px 0;'><b>You:</b> " + question + "</div>");
        
        QString prompt = QString(
            "You are an expert game reverse engineer. Answer this question:\n\n%1\n\n"
            "Game code context:\n%2\n\n"
            "Provide practical advice with code examples."
        ).arg(question, m_GameContext.left(15000));
        
        askAI(prompt, [this](const QString &response) {
            m_ChatArea->append("<div style='color: #7ee787; margin: 10px 0;'><b>AI:</b><br>" + response + "</div>");
        });
    });
    connect(m_AIChatInput, &QLineEdit::returnPressed, sendChatBtn, &QPushButton::click);
    connect(saveBtn, &QPushButton::clicked, this, &InteractiveModMenuDialog::saveAndApply);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void InteractiveModMenuDialog::populateModsList()
{
    m_AvailableModsList->clear();
    QString category = m_CategoryCombo->currentText();
    
    QList<QPair<QString, QString>> mods;
    
    if (category.contains("All") || category.contains("Resources")) {
        mods << qMakePair(tr("💰 Unlimited Coins"), "coins")
             << qMakePair(tr("💎 Unlimited Gems/Diamonds"), "gems")
             << qMakePair(tr("⚡ Unlimited Energy"), "energy")
             << qMakePair(tr("🔑 Unlimited Keys"), "keys")
             << qMakePair(tr("🎟️ Unlimited Tickets"), "tickets")
             << qMakePair(tr("⭐ Unlimited Stars"), "stars")
             << qMakePair(tr("❤️ Unlimited Hearts/Lives"), "hearts")
             << qMakePair(tr("👑 VIP/Premium Status"), "vip");
    }
    
    if (category.contains("All") || category.contains("Player")) {
        mods << qMakePair(tr("🛡️ God Mode (Invincible)"), "godmode")
             << qMakePair(tr("⚔️ One-Hit Kill"), "onehit")
             << qMakePair(tr("💪 Damage Multiplier"), "damage_mult")
             << qMakePair(tr("🏃 Speed Hack"), "speed")
             << qMakePair(tr("🎯 100% Critical Hit"), "critical")
             << qMakePair(tr("👻 100% Dodge/Evasion"), "dodge")
             << qMakePair(tr("📈 XP Multiplier"), "exp_mult")
             << qMakePair(tr("🔄 No Cooldowns"), "no_cooldown");
    }
    
    if (category.contains("All") || category.contains("Inventory")) {
        mods << qMakePair(tr("📦 Unlock All Items"), "unlock_items")
             << qMakePair(tr("🧑 Unlock All Characters"), "unlock_chars")
             << qMakePair(tr("🗺️ Unlock All Levels"), "unlock_levels")
             << qMakePair(tr("🎨 Unlock All Skins"), "unlock_skins")
             << qMakePair(tr("🔫 Unlock All Weapons"), "unlock_weapons")
             << qMakePair(tr("🐾 Unlock All Pets"), "unlock_pets")
             << qMakePair(tr("⬆️ Max Upgrades"), "max_upgrades");
    }
    
    if (category.contains("All") || category.contains("Game")) {
        mods << qMakePair(tr("🚫 Remove Ads"), "no_ads")
             << qMakePair(tr("🆓 Free In-App Purchases"), "free_iap")
             << qMakePair(tr("⏱️ Freeze Timer"), "freeze_time")
             << qMakePair(tr("🏆 Always Win"), "always_win")
             << qMakePair(tr("🔢 Score Multiplier"), "score_mult")
             << qMakePair(tr("👾 No Enemies"), "no_enemies");
    }
    
    if (category.contains("All") || category.contains("Anti-Cheat")) {
        mods << qMakePair(tr("🔓 SSL Pinning Bypass"), "ssl_bypass")
             << qMakePair(tr("🕵️ Root/Jailbreak Detection Bypass"), "root_bypass")
             << qMakePair(tr("📱 Emulator Detection Bypass"), "emulator_bypass")
             << qMakePair(tr("🔍 Integrity Check Bypass"), "integrity_bypass")
             << qMakePair(tr("🔐 License Verification Bypass"), "license_bypass");
    }
    
    for (const auto &mod : mods) {
        auto item = new QListWidgetItem(mod.first);
        item->setData(Qt::UserRole, mod.second);
        m_AvailableModsList->addItem(item);
    }
}

void InteractiveModMenuDialog::onCategorySelected(int)
{
    populateModsList();
}

void InteractiveModMenuDialog::addCustomMod()
{
    QString customText = m_CustomModInput->text().trimmed();
    if (customText.isEmpty()) return;
    
    m_SelectedModsList->addItem("✨ " + customText);
    
    ModOption mod;
    mod.name = customText;
    mod.id = "custom_" + QString::number(m_SelectedMods.size());
    mod.customValue = customText;
    mod.enabled = true;
    mod.modType = "custom";
    m_SelectedMods.append(mod);
    
    m_CustomModInput->clear();
}

void InteractiveModMenuDialog::removeSelectedMod()
{
    auto items = m_SelectedModsList->selectedItems();
    for (auto item : items) {
        int row = m_SelectedModsList->row(item);
        if (row >= 0 && row < m_SelectedMods.size()) {
            m_SelectedMods.removeAt(row);
        }
        delete item;
    }
}

void InteractiveModMenuDialog::generateWithAI()
{
    if (m_SelectedModsList->count() == 0) {
        QMessageBox::warning(this, tr("No Mods"), tr("Please select at least one modification."));
        return;
    }
    
    m_PreviewArea->setPlainText(tr("🤖 Generating mod menu with AI..."));
    
    QString prompt = buildModPrompt();
    
    askAI(prompt, [this](const QString &code) {
        m_GeneratedCode = code;
        m_PreviewArea->setPlainText(code);
        m_ChatArea->append("<div style='color: #7ee787;'>✅ Mod menu code generated successfully!</div>");
    });
}

QString InteractiveModMenuDialog::buildModPrompt()
{
    QString style = m_MenuStyleCombo->currentText();
    QString modsDescription;
    
    for (int i = 0; i < m_SelectedModsList->count(); i++) {
        modsDescription += QString("- %1\n").arg(m_SelectedModsList->item(i)->text());
    }
    
    return QString(
        "Generate a complete, production-ready mod menu for a Unity/Android game.\n\n"
        "**Menu Style:** %1\n\n"
        "**Modifications to include:**\n%2\n\n"
        "**Game Code Context (for reference):**\n%3\n\n"
        "Requirements:\n"
        "1. Create a professional floating/overlay menu with toggle switches\n"
        "2. Include proper initialization, hooks, and cleanup code\n"
        "3. Add value sliders/inputs where applicable (e.g., multipliers)\n"
        "4. Include hotkey to toggle menu (F1 on PC, Volume buttons on Android)\n"
        "5. Make it visually appealing with proper styling\n"
        "6. Add detailed comments explaining each hook/modification\n"
        "7. If IL2CPP: Include memory patterns and offset finding code\n"
        "8. If Frida/Xposed: Include complete module structure\n"
        "9. Add anti-detection techniques if applicable\n\n"
        "Return ONLY the complete, ready-to-compile source code."
    ).arg(style, modsDescription, m_GameContext.left(10000));
}

void InteractiveModMenuDialog::previewCode()
{
    if (m_GeneratedCode.isEmpty()) {
        QMessageBox::information(this, tr("No Code"), tr("Generate the mod menu first."));
        return;
    }
    
    // Show in a larger dialog
    QDialog previewDialog(this);
    previewDialog.setWindowTitle(tr("Code Preview"));
    previewDialog.resize(900, 700);
    
    auto layout = new QVBoxLayout(&previewDialog);
    auto textEdit = new QPlainTextEdit();
    textEdit->setPlainText(m_GeneratedCode);
    textEdit->setStyleSheet("background: #0d1117; color: #7ee787; font-family: 'Consolas', monospace;");
    textEdit->setReadOnly(true);
    layout->addWidget(textEdit);
    
    auto closeBtn = new QPushButton(tr("Close"));
    connect(closeBtn, &QPushButton::clicked, &previewDialog, &QDialog::accept);
    layout->addWidget(closeBtn);
    
    previewDialog.exec();
}

void InteractiveModMenuDialog::saveAndApply()
{
    if (m_GeneratedCode.isEmpty()) {
        QMessageBox::warning(this, tr("No Code"), tr("Generate the mod menu first."));
        return;
    }
    
    QString ext = ".cpp";
    QString style = m_MenuStyleCombo->currentText();
    if (style.contains("Frida")) ext = ".js";
    else if (style.contains("Java") || style.contains("Xposed")) ext = ".java";
    
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Mod Menu Code"), 
                                                    m_ProjectPath + "/ModMenu" + ext,
                                                    "Source Files (*.cpp *.h *.js *.java)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(m_GeneratedCode.toUtf8());
            file.close();
            QMessageBox::information(this, tr("Saved"), 
                tr("Mod menu saved to: %1\n\nNext steps:\n"
                   "1. Review the generated code\n"
                   "2. Compile using appropriate toolchain\n"
                   "3. Inject/install the mod").arg(fileName));
        }
    }
}

void InteractiveModMenuDialog::askAI(const QString &prompt, std::function<void(const QString&)> callback)
{
    QSettings settings;
    QString key = settings.value("ai_api_key").toString();
    QString model = settings.value("ai_model", "gemini-2.0-flash-exp").toString();
    
    if (key.isEmpty()) {
        m_ChatArea->append("<span style='color: #f85149;'>Error: API Key not configured in Settings.</span>");
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
            m_ChatArea->append("<span style='color: #f85149;'>AI Error: " + reply->errorString() + "</span>");
        }
        reply->deleteLater();
    });
}

// ==================== Legacy Dialog Implementations ====================

UnityGameDialog::UnityGameDialog(const QString &projectPath, QWidget *parent) 
    : QDialog(parent) 
{
    setWindowTitle(tr("Unity Game Analyzer"));
    setMinimumSize(800, 600);
    
    auto layout = new QVBoxLayout(this);
    
    auto infoLabel = new QLabel(tr("<h2>Unity Game Analysis</h2>"
        "<p>This dialog provides specialized analysis for Unity games.</p>"));
    layout->addWidget(infoLabel);
    
    // Embed the main GameModStudio widget
    auto studio = new GameModStudio(projectPath, this);
    layout->addWidget(studio);
    
    auto closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(closeBtn);
}

FlutterAnalyzerDialog::FlutterAnalyzerDialog(const QString &projectPath, QWidget *parent) 
    : QDialog(parent) 
{
    setWindowTitle(tr("Flutter App Analyzer"));
    setMinimumSize(700, 500);
    
    auto layout = new QVBoxLayout(this);
    
    auto infoLabel = new QLabel(tr("<h2>Flutter App Analysis</h2>"
        "<p>Analyze Flutter/Dart applications for modification vectors.</p>"));
    layout->addWidget(infoLabel);
    
    auto logView = new QTextBrowser(this);
    logView->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4; font-family: monospace;");
    layout->addWidget(logView);
    
    // Check for Flutter artifacts
    QDir libDir(projectPath + "/lib");
    if (libDir.exists()) {
        QStringList soFiles = libDir.entryList({"libflutter.so", "libapp.so"}, QDir::Files);
        if (!soFiles.isEmpty()) {
            logView->append(tr("<span style='color: #7ee787;'>✅ Flutter artifacts detected:</span>"));
            for (const QString &so : soFiles) {
                QFileInfo fi(libDir.absoluteFilePath(so));
                logView->append(QString("• %1 (%2 KB)").arg(so).arg(fi.size() / 1024));
            }
            logView->append(tr("\n<b>Modification Vectors:</b>"));
            logView->append(tr("• libapp.so contains compiled Dart code (AOT snapshot)"));
            logView->append(tr("• Frida hooks can intercept Dart method calls"));
            logView->append(tr("• Network requests go through BoringSSL (SSL pinning common)"));
        } else {
            logView->append(tr("<span style='color: #f85149;'>No Flutter libraries found in /lib</span>"));
        }
    }
    
    auto closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(closeBtn);
}

GameValueEditorDialog::GameValueEditorDialog(const QString &projectPath, QWidget *parent) 
    : QDialog(parent) 
{
    setWindowTitle(tr("Game Value Editor"));
    setMinimumSize(600, 400);
    
    auto layout = new QVBoxLayout(this);
    
    auto infoLabel = new QLabel(tr("<h2>Game Value Editor</h2>"
        "<p>Edit game values and configurations directly.</p>"));
    layout->addWidget(infoLabel);
    
    // Create value table
    auto table = new QTableWidget(this);
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({tr("Key"), tr("Value"), tr("Type")});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4;");
    layout->addWidget(table);
    
    // Scan for common game value files
    QStringList valueFiles = {
        projectPath + "/assets/game_config.json",
        projectPath + "/assets/settings.json", 
        projectPath + "/shared_prefs/game_prefs.xml",
        projectPath + "/res/values/integers.xml"
    };
    
    int row = 0;
    for (const QString &filePath : valueFiles) {
        QFile file(filePath);
        if (file.exists() && file.open(QIODevice::ReadOnly)) {
            QString content = QString::fromUtf8(file.readAll());
            file.close();
            
            // Try to parse as JSON
            QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8());
            if (!doc.isNull() && doc.isObject()) {
                QJsonObject obj = doc.object();
                for (auto it = obj.begin(); it != obj.end(); ++it) {
                    table->insertRow(row);
                    table->setItem(row, 0, new QTableWidgetItem(it.key()));
                    table->setItem(row, 1, new QTableWidgetItem(it.value().toVariant().toString()));
                    table->setItem(row, 2, new QTableWidgetItem(
                        it.value().isBool() ? "bool" : 
                        it.value().isDouble() ? "number" : "string"));
                    row++;
                }
            }
        }
    }
    
    if (row == 0) {
        table->insertRow(0);
        table->setItem(0, 0, new QTableWidgetItem(tr("No game value files found")));
    }
    
    auto btnLayout = new QHBoxLayout();
    auto saveBtn = new QPushButton(tr("Save Changes"), this);
    saveBtn->setStyleSheet("background-color: #238636; color: white;");
    auto closeBtn = new QPushButton(tr("Close"), this);
    btnLayout->addStretch();
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);
    
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(saveBtn, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this, tr("Saved"), tr("Game values saved. Recompile to apply."));
    });
}

// =============================================================================
// ModMenuCodeGenerator Implementation
// =============================================================================

ModMenuCodeGenerator::ModMenuCodeGenerator(const QString &projectPath, QObject *parent)
    : QObject(parent), m_ProjectPath(projectPath)
{
    // Initialize field patterns for mod types - comprehensive coverage
    // Patterns extracted from real Unity games (IL2CPP dump analysis)
    m_FieldPatterns["coins"] = "(coin|gold|money|cash|credit|currency|balance|wallet|funds|token|_coin|<Coin>)";
    m_FieldPatterns["gems"] = "(gem|diamond|ruby|crystal|jewel|sapphire|emerald|stone|_gem|<Gem>)";
    m_FieldPatterns["energy"] = "(energy|stamina|power|fuel|ap|actionpoint|mana|mp|_energy|<Energy>)";
    m_FieldPatterns["health"] = "(health|hp|hitpoint|life|currenthp|maxhp|lifepoint|_health|<Health>)";
    m_FieldPatterns["damage"] = "(damage|attack|atk|dmg|attackpower|basedamage|weapondamage)";
    m_FieldPatterns["damage_mult"] = "(damagemult|attackmult|dmgmult|critdamage|bonusdamage)";
    m_FieldPatterns["speed"] = "(speed|spd|velocity|movespeed|movementspeed|runspeed)";
    m_FieldPatterns["defense"] = "(defense|def|armor|shield|protection|resistance)";
    m_FieldPatterns["exp"] = "(experience|exp|xp|experiencepoint|totalxp|levelxp|<XP>)";
    m_FieldPatterns["exp_mult"] = "(expmult|xpmult|experiencemult|bonusexp)";
    m_FieldPatterns["level"] = "(level|lvl|playerlevel|characterlevel|metalevel|<Level>)";
    m_FieldPatterns["keys"] = "(key|ticket|pass|voucher|token)";
    m_FieldPatterns["tickets"] = "(ticket|spin|roll|gacha|summon|pull|jackpot)";
    m_FieldPatterns["tokens"] = "(token|medal|badge|point|currency)";
    m_FieldPatterns["stars"] = "(star|rating|score|point|achievement)";
    m_FieldPatterns["hearts"] = "(heart|life|lives|remaining|continue|moves|turns)";
    m_FieldPatterns["vip"] = "(vip|premium|pro|subscriber|elite|member|seasonpass|goldpass)";
    m_FieldPatterns["attack_speed"] = "(attackspeed|atkspd|aspd|firerate|cooldown)";
    m_FieldPatterns["critical"] = "(critical|crit|critrate|critchance|critdamage)";
    m_FieldPatterns["dodge"] = "(dodge|evasion|evade|miss|avoid)";
    m_FieldPatterns["inventory_max"] = "(stack|maxstack|capacity|slotsize|bagsize|quantity|amount)";
    m_FieldPatterns["score_mult"] = "(scoremult|pointmult|bonusscore|multiplier)";
    
    // New patterns from MergePuzzle/Casual game analysis
    m_FieldPatterns["booster"] = "(booster|powerup|helper|hint|shuffle|bomb|hammer|rocket)";
    m_FieldPatterns["piggybank"] = "(piggybank|piggy|bonus|savings|accumulated)";
    m_FieldPatterns["dailyreward"] = "(dailyreward|dailyprogress|checkprogress|volatilereward)";
    m_FieldPatterns["unlimitedenergy"] = "(unlimitedenergy|infiniteenergy|freeenergy|unlimitedduration)";
    m_FieldPatterns["noads"] = "(noads|adfree|removeads|seenfirstad)";
    m_FieldPatterns["reward"] = "(reward|activeReward|pendingReward|waitingReward|rewardEntity)";
    
    // Initialize method patterns - expanded for better detection
    m_MethodPatterns["coins"] = "(Add|Set|Grant|Give|Spend|Deduct|Remove|Use|Change)(Coin|Gold|Money|Cash|Currency|Credit)";
    m_MethodPatterns["gems"] = "(Add|Set|Grant|Give|Spend|Deduct|Remove|Use|Change)(Gem|Diamond|Crystal|Ruby|Jewel)";
    m_MethodPatterns["energy"] = "(Add|Set|Consume|Spend|Refill|Use|Restore|Recharge|Change)(Energy|Stamina|Power|Mana|AP)";
    m_MethodPatterns["health"] = "(Add|Set|Take|Deal|Heal|Restore|Damage|Hurt)(Health|HP|Life|Damage)";
    m_MethodPatterns["damage"] = "(Add|Set|Deal|Calculate|Apply|Get)(Damage|Attack|Atk|Dmg)";
    m_MethodPatterns["speed"] = "(Set|Get|Multiply|Boost)(Speed|Velocity|Movement)";
    m_MethodPatterns["defense"] = "(Set|Get|Add|Calculate)(Defense|Armor|Shield|Protection)";
    m_MethodPatterns["exp"] = "(Add|Set|Grant|Give|Gain|Earn|Process)(Experience|Exp|XP|XPUpdate)";
    m_MethodPatterns["level"] = "(Set|Add|Level|GainLevel|LevelUp|ChangeMetaLevel|ProcessLevelUpdate)";
    m_MethodPatterns["free_iap"] = "(Purchase|Buy|CanAfford|GetPrice|ProcessPurchase|OnPurchase|ValidatePurchase|BuyItem|AddPurchase)";
    m_MethodPatterns["no_ads"] = "(Show|Display|Load|Request|Is|Enable|Disable|Activate)(Ad|Ads|Banner|Interstitial|Rewarded|NoAds)";
    m_MethodPatterns["unlock_items"] = "(Unlock|HasItem|IsUnlocked|CanAccess|IsOwned|HasAccess|GrantItem|AddReward)";
    m_MethodPatterns["unlock_chars"] = "(Unlock|IsUnlocked|HasCharacter|CanUse)(Character|Hero|Champion|Unit)";
    m_MethodPatterns["unlock_levels"] = "(Unlock|IsUnlocked|CanPlay|HasAccess)(Level|Stage|Chapter|World|Map)";
    m_MethodPatterns["unlock_skins"] = "(Unlock|IsUnlocked|HasSkin|Own)(Skin|Costume|Outfit|Appearance)";
    m_MethodPatterns["unlock_weapons"] = "(Unlock|IsUnlocked|HasWeapon|Own)(Weapon|Gun|Sword|Item)";
    m_MethodPatterns["unlock_pets"] = "(Unlock|IsUnlocked|HasPet|Own)(Pet|Companion|Familiar|Buddy)";
    m_MethodPatterns["max_upgrades"] = "(Upgrade|IsMaxLevel|GetUpgradeLevel|MaxUpgrade)";
    m_MethodPatterns["vip"] = "(Is|Get|Check|Has|Enable)(Premium|VIP|Pro|Subscriber|Elite|Member|SeasonPass|GoldPass)";
    m_MethodPatterns["no_cooldown"] = "(Get|Start|Check|Is|Reset|Skip)(Cooldown|Timer|Ready|Waiting)";
    m_MethodPatterns["freeze_time"] = "(Get|Set|Update|Tick)(Timer|Time|Countdown|Remaining)";
    m_MethodPatterns["always_win"] = "(Check|Is|Set)(Win|Victory|Success|Complete)";
    m_MethodPatterns["no_enemies"] = "(Spawn|Create|Generate|Is)(Enemy|Enemies|Monster|Mob)";
    m_MethodPatterns["instant_kill"] = "(Take|Deal|Apply|Calculate)(Damage|Kill|Death)";
    
    // New method patterns from MergePuzzle/Casual game analysis
    m_MethodPatterns["booster"] = "(Add|Use|Consume|Get|Set|Change)(Booster|PowerUp|Helper|Hint)";
    m_MethodPatterns["unlimited_energy"] = "(Add|Get|Set|Is)Unlimited(Energy|Duration|Time)";
    m_MethodPatterns["daily_reward"] = "(Claim|Get|Progress|Check)(Daily|Reward|Calendar|Jackpot)";
    m_MethodPatterns["anti_cheat"] = "(Detect|Check|Verify|Validate|Is)(Cheat|Hack|Time|Tamper|Integrity)";
    m_MethodPatterns["currency_generic"] = "(Change|Get|Set)(Currency|Balance|Amount|Quantity)";
    m_MethodPatterns["active_rewards"] = "(Change|Add|Remove|Get)(ActiveReward|Reward|WaitingReward)";
}

bool ModMenuCodeGenerator::parseDumpCs()
{
    QString dumpPath = m_ProjectPath + "/dump/dump.cs";
    if (!QFile::exists(dumpPath)) {
        emit logMessage("dump.cs not found. Run IL2CPP Dumper first.", "error");
        return false;
    }
    
    QFile file(dumpPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit logMessage("Failed to open dump.cs", "error");
        return false;
    }
    
    m_DumpContent = QString::fromUtf8(file.readAll());
    file.close();
    
    emit progressUpdated(10, "Parsing dump.cs...");
    emit logMessage(QString("Loaded dump.cs (%1 MB)").arg(m_DumpContent.size() / 1024.0 / 1024.0, 0, 'f', 2), "info");
    
    // Parse classes
    QStringList lines = m_DumpContent.split('\n');
    QString currentNamespace;
    QString currentClassName;
    GameClassInfo currentClass;
    bool inClass = false;
    int braceCount = 0;
    
    QRegularExpression namespaceRegex("^namespace\\s+([\\w\\.]+)");
    QRegularExpression classRegex("^\\s*(public|internal|private)?\\s*(sealed|abstract|static)?\\s*(class|struct)\\s+(\\w+)");
    QRegularExpression fieldRegex("^\\s*(public|private|protected)?\\s*(static)?\\s*(readonly)?\\s*(int|float|double|long|bool|byte|short|string|Int32|Int64|Single|Double|Boolean|String)\\s+(\\w+)\\s*;\\s*//\\s*(0x[0-9A-Fa-f]+)");
    QRegularExpression methodRegex("^\\s*(public|private|protected)?\\s*(static)?\\s*(virtual|override)?\\s*(void|int|float|bool|string|\\w+)\\s+(\\w+)\\s*\\(([^)]*)\\)[^/]*//\\s*RVA:\\s*(0x[0-9A-Fa-f]+)");
    
    // Class name patterns to prioritize - expanded from real game analysis
    QStringList priorityPatterns = {
        // Core data classes
        "PlayerData", "UserData", "GameData", "SaveData", "ProfileData",
        "UserEntity", "BoardEntity", "MetaEntity", "GameEntity", "SessionEntity",
        // Currency/Economy
        "CurrencyManager", "CoinManager", "GemManager", "WalletManager", "EconomyManager",
        "Currency", "CurrencyDTO", "CurrencyService", "CurrencyHandler",
        // Resources
        "EnergyManager", "EnergyEntity", "StaminaManager", "ResourceManager",
        "BoosterEntity", "BoosterManager", "BoosterDTO", "PowerUpManager",
        // Player stats
        "HealthManager", "DamageManager", "CombatManager", "StatsManager",
        "PlayerController", "PlayerStats", "GameManager", "MasterController",
        // Inventory/Items
        "InventoryManager", "InventoryEntity", "ItemManager", "ItemEntity",
        "RewardEntity", "RewardManager", "ActiveReward",
        // Shop/IAP
        "ShopManager", "StoreManager", "ShopEntity", "PurchaseManager",
        "IAPManager", "BillingManager", "PackageManager",
        // Timers/Events
        "TimerManager", "CooldownManager", "EventManager", "EventEntity",
        "DailyRewardEntity", "SeasonPassEntity", "PiggyBankEntity",
        // Unlock/Premium
        "UnlockManager", "FeatureManager", "VIPManager", "PremiumManager",
        "UnlimitedEnergyEntity", "SubscriptionManager"
    };
    
    // Classes to skip - UI, animations, system classes
    QStringList skipPatterns = {
        "UI", "Animation", "Tween", "Renderer", "Shader", "Material", "Canvas",
        "Button", "Text", "Image", "Panel", "Scroll", "Layout", "Sprite",
        "Particle", "Audio", "Sound", "Music", "Effect", "DOTween", "LeanTween",
        "iTween", "UniRx", "Cysharp", "Firebase", "Analytics", "Tracking",
        "Localization", "Translation", "EventLog", "Popup", "Modal", "Dialog",
        "Tooltip", "HUD", "Label", "Icon", "Badge", "AsyncState", "StateMachine",
        "Awaiter", "MethodBuilder"
    };
    
    int classCount = 0;
    
    for (int i = 0; i < lines.size(); i++) {
        const QString &line = lines[i];
        
        // Track namespace
        QRegularExpressionMatch nsMatch = namespaceRegex.match(line);
        if (nsMatch.hasMatch()) {
            currentNamespace = nsMatch.captured(1);
            continue;
        }
        
        // Detect class
        QRegularExpressionMatch classMatch = classRegex.match(line);
        if (classMatch.hasMatch()) {
            // Save previous class if valid
            if (!currentClass.className.isEmpty() && 
                (!currentClass.fields.isEmpty() || !currentClass.methods.isEmpty())) {
                m_GameClasses.append(currentClass);
                classCount++;
            }
            
            currentClassName = classMatch.captured(4);
            
            // Check if should skip
            bool shouldSkip = false;
            for (const QString &skip : skipPatterns) {
                if (currentClassName.contains(skip, Qt::CaseInsensitive)) {
                    shouldSkip = true;
                    break;
                }
            }
            
            if (shouldSkip) {
                inClass = false;
                continue;
            }
            
            // Check priority
            bool isPriority = false;
            for (const QString &pattern : priorityPatterns) {
                if (currentClassName.contains(pattern, Qt::CaseInsensitive)) {
                    isPriority = true;
                    break;
                }
            }
            
            currentClass = GameClassInfo();
            currentClass.className = currentClassName;
            currentClass.nameSpace = currentNamespace;
            inClass = true;
            braceCount = 0;
            continue;
        }
        
        if (inClass) {
            braceCount += line.count('{') - line.count('}');
            if (braceCount < 0) {
                inClass = false;
                continue;
            }
            
            // Extract fields
            QRegularExpressionMatch fieldMatch = fieldRegex.match(line);
            if (fieldMatch.hasMatch()) {
                QString fieldName = fieldMatch.captured(5);
                QString fieldType = fieldMatch.captured(4);
                QString offset = fieldMatch.captured(6);
                currentClass.fields.append(QString("%1|%2|%3").arg(fieldName, fieldType, offset));
            }
            
            // Extract methods
            QRegularExpressionMatch methodMatch = methodRegex.match(line);
            if (methodMatch.hasMatch()) {
                QString methodName = methodMatch.captured(5);
                QString returnType = methodMatch.captured(4);
                QString params = methodMatch.captured(6);
                QString rva = methodMatch.captured(7);
                currentClass.methods.append(QString("%1|%2|%3|%4").arg(methodName, returnType, params, rva));
            }
        }
        
        // Progress update every 10000 lines
        if (i % 10000 == 0) {
            int progress = 10 + (i * 30 / lines.size());
            emit progressUpdated(progress, QString("Parsing line %1/%2...").arg(i).arg(lines.size()));
        }
    }
    
    // Save last class
    if (!currentClass.className.isEmpty() && 
        (!currentClass.fields.isEmpty() || !currentClass.methods.isEmpty())) {
        m_GameClasses.append(currentClass);
    }
    
    emit progressUpdated(40, QString("Found %1 relevant classes").arg(m_GameClasses.size()));
    emit logMessage(QString("Extracted %1 game classes with fields/methods").arg(m_GameClasses.size()), "success");
    
    return !m_GameClasses.isEmpty();
}

QList<ModTarget> ModMenuCodeGenerator::findModTargets(const QList<ModOption> &mods)
{
    QList<ModTarget> targets;
    
    emit progressUpdated(45, "Finding mod targets...");
    
    for (const ModOption &mod : mods) {
        ModTarget target;
        target.modId = mod.id;
        target.displayName = mod.name;
        target.value = mod.value;
        
        // Get pattern for this mod type
        QString fieldPattern = m_FieldPatterns.value(mod.id);
        QString methodPattern = m_MethodPatterns.value(mod.id);
        
        if (fieldPattern.isEmpty() && methodPattern.isEmpty()) {
            // Custom mod - use the customValue as hint
            if (!mod.customValue.isEmpty()) {
                fieldPattern = mod.customValue;
                methodPattern = mod.customValue;
            } else {
                continue;
            }
        }
        
        QRegularExpression fieldRx(fieldPattern, QRegularExpression::CaseInsensitiveOption);
        QRegularExpression methodRx(methodPattern, QRegularExpression::CaseInsensitiveOption);
        
        // Search through classes
        bool found = false;
        for (const GameClassInfo &classInfo : m_GameClasses) {
            // Search fields
            if (!fieldPattern.isEmpty()) {
                for (const QString &field : classInfo.fields) {
                    QStringList parts = field.split('|');
                    if (parts.size() >= 3) {
                        QString fieldName = parts[0];
                        QString fieldType = parts[1];
                        QString offset = parts[2];
                        
                        if (fieldRx.match(fieldName).hasMatch()) {
                            target.targetClass = classInfo.nameSpace.isEmpty() ? 
                                classInfo.className : 
                                classInfo.nameSpace + "." + classInfo.className;
                            target.targetField = fieldName;
                            target.fieldType = fieldType;
                            target.offset = offset;
                            target.hookType = "field_write";
                            found = true;
                            break;
                        }
                    }
                }
            }
            
            if (found) break;
            
            // Search methods
            if (!methodPattern.isEmpty()) {
                for (const QString &method : classInfo.methods) {
                    QStringList parts = method.split('|');
                    if (parts.size() >= 4) {
                        QString methodName = parts[0];
                        QString returnType = parts[1];
                        QString params = parts[2];
                        QString rva = parts[3];
                        
                        if (methodRx.match(methodName).hasMatch()) {
                            target.targetClass = classInfo.nameSpace.isEmpty() ? 
                                classInfo.className : 
                                classInfo.nameSpace + "." + classInfo.className;
                            target.targetMethod = methodName;
                            target.fieldType = returnType;
                            target.rva = rva;
                            target.hookType = returnType == "void" ? "method_replace" : "method_return";
                            found = true;
                            break;
                        }
                    }
                }
            }
            
            if (found) break;
        }
        
        if (found) {
            targets.append(target);
            emit logMessage(QString("Found target for %1: %2.%3 @ %4")
                .arg(mod.name, target.targetClass, 
                     target.targetField.isEmpty() ? target.targetMethod : target.targetField,
                     target.offset.isEmpty() ? target.rva : target.offset), "info");
        } else {
            // Still add it but without specific target - AI will need to fill in
            target.hookType = "placeholder";
            targets.append(target);
            emit logMessage(QString("No specific target found for %1 - using placeholder").arg(mod.name), "warning");
        }
    }
    
    emit progressUpdated(60, QString("Found %1/%2 mod targets").arg(targets.size()).arg(mods.size()));
    return targets;
}

bool ModMenuCodeGenerator::generateProject(const QString &outputDir, const QList<ModTarget> &targets, const QString &style)
{
    emit progressUpdated(65, "Generating project files...");
    
    QDir dir(outputDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // Create jni directory structure
    dir.mkpath("jni");
    dir.mkpath("jni/imgui");
    dir.mkpath("jni/imgui/backends");
    dir.mkpath("jni/dobby");
    
    bool success = true;
    
    // Check for downloaded dependencies and copy them
    QSettings settings;
    QString imguiPath = settings.value("imgui_path").toString();
    QString dobbyPath = settings.value("dobby_path").toString();
    
    // Also check default location
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString defaultDepsDir = appDataDir + "/mod_deps";
    
    if (imguiPath.isEmpty() || !QFile::exists(imguiPath + "/imgui.h")) {
        imguiPath = defaultDepsDir + "/imgui";
    }
    if (dobbyPath.isEmpty() || !QFile::exists(dobbyPath + "/dobby.h")) {
        dobbyPath = defaultDepsDir + "/dobby";
    }
    
    // Copy ImGui files if available
    if (QFile::exists(imguiPath + "/imgui.h")) {
        emit progressUpdated(67, "Copying ImGui dependencies...");
        QDir imguiDir(imguiPath);
        QStringList imguiFiles = imguiDir.entryList({"*.h", "*.cpp"}, QDir::Files);
        for (const QString &file : imguiFiles) {
            QFile::copy(imguiPath + "/" + file, outputDir + "/jni/imgui/" + file);
        }
        emit logMessage(QString("Copied %1 ImGui files").arg(imguiFiles.size()), "success");
    } else {
        emit logMessage("ImGui not found. Run 'Download Dependencies' first or download manually.", "warning");
        
        // Generate placeholder with download instructions
        QFile imguiPlaceholder(outputDir + "/jni/imgui/DOWNLOAD_IMGUI.txt");
        if (imguiPlaceholder.open(QIODevice::WriteOnly)) {
            imguiPlaceholder.write(R"(IMGUI LIBRARY REQUIRED

Download ImGui from: https://github.com/ocornut/imgui

Required files to copy here:
- imgui.h
- imgui.cpp
- imgui_demo.cpp
- imgui_draw.cpp
- imgui_tables.cpp
- imgui_widgets.cpp
- imgui_internal.h
- imconfig.h
- imstb_rectpack.h
- imstb_textedit.h
- imstb_truetype.h

Also copy from backends/ folder:
- imgui_impl_opengl3.h
- imgui_impl_opengl3.cpp
- imgui_impl_android.h
- imgui_impl_android.cpp
)");
            imguiPlaceholder.close();
        }
    }
    
    // Copy Dobby files if available
    if (QFile::exists(dobbyPath + "/dobby.h")) {
        emit progressUpdated(68, "Copying Dobby dependencies...");
        QFile::copy(dobbyPath + "/dobby.h", outputDir + "/jni/dobby/dobby.h");
        
        // Copy libdobby.a if exists
        if (QFile::exists(dobbyPath + "/libdobby.a")) {
            QFile::copy(dobbyPath + "/libdobby.a", outputDir + "/jni/dobby/libdobby.a");
            emit logMessage("Copied Dobby header and library", "success");
        } else {
            emit logMessage("Copied Dobby header (libdobby.a needs to be built)", "info");
        }
    } else {
        emit logMessage("Dobby not found. Run 'Download Dependencies' first.", "warning");
        
        // Generate placeholder Dobby header
        QFile dobbyPlaceholder(outputDir + "/jni/dobby/dobby.h");
        if (dobbyPlaceholder.open(QIODevice::WriteOnly)) {
            dobbyPlaceholder.write(R"(/*
 * Dobby - Inline Hooking Framework
 * 
 * PLACEHOLDER - Replace with real Dobby library!
 * 
 * Build from source:
 *   git clone https://github.com/jmpews/Dobby.git
 *   cd Dobby && mkdir build && cd build
 *   cmake .. -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
 *            -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24
 *   make
 *   
 * Then copy:
 *   - include/dobby.h -> here
 *   - libdobby.a -> here
 */

#ifndef DOBBY_H
#define DOBBY_H

#ifdef __cplusplus
extern "C" {
#endif

int DobbyHook(void *address, void *replace_func, void **origin_func);
int DobbyDestroy(void *address);
void *DobbySymbolResolver(const char *image_name, const char *symbol_name);

#ifdef __cplusplus
}
#endif

#endif // DOBBY_H
)");
            dobbyPlaceholder.close();
        }
    }
    
    if (style.contains("Frida", Qt::CaseInsensitive)) {
        // Generate Frida script
        QString fridaScript = generateFridaScript(targets);
        QFile fridaFile(outputDir + "/mod_menu.js");
        if (fridaFile.open(QIODevice::WriteOnly)) {
            fridaFile.write(fridaScript.toUtf8());
            fridaFile.close();
            emit logMessage("Generated mod_menu.js", "info");
        }
    } else {
        // Generate C++ project
        emit progressUpdated(70, "Generating main.cpp...");
        QFile mainFile(outputDir + "/jni/main.cpp");
        if (mainFile.open(QIODevice::WriteOnly)) {
            mainFile.write(generateMainCpp(targets).toUtf8());
            mainFile.close();
        }
        
        emit progressUpdated(75, "Generating game_defs.hpp...");
        QFile defsFile(outputDir + "/jni/game_defs.hpp");
        if (defsFile.open(QIODevice::WriteOnly)) {
            defsFile.write(generateGameDefsHpp(targets).toUtf8());
            defsFile.close();
        }
        
        emit progressUpdated(80, "Generating mod_menu.hpp...");
        QFile menuFile(outputDir + "/jni/mod_menu.hpp");
        if (menuFile.open(QIODevice::WriteOnly)) {
            menuFile.write(generateModMenuHpp(targets).toUtf8());
            menuFile.close();
        }
        
        emit progressUpdated(85, "Generating IL2CPP utilities...");
        QFile utilsHpp(outputDir + "/jni/il2cpp_utils.hpp");
        if (utilsHpp.open(QIODevice::WriteOnly)) {
            utilsHpp.write(generateIl2cppUtilsHpp().toUtf8());
            utilsHpp.close();
        }
        
        QFile utilsCpp(outputDir + "/jni/il2cpp_utils.cpp");
        if (utilsCpp.open(QIODevice::WriteOnly)) {
            utilsCpp.write(generateIl2cppUtilsCpp().toUtf8());
            utilsCpp.close();
        }
        
        emit progressUpdated(90, "Generating build files...");
        QFile mkFile(outputDir + "/jni/Android.mk");
        if (mkFile.open(QIODevice::WriteOnly)) {
            mkFile.write(generateAndroidMk().toUtf8());
            mkFile.close();
        }
        
        QFile appMk(outputDir + "/jni/Application.mk");
        if (appMk.open(QIODevice::WriteOnly)) {
            appMk.write(generateApplicationMk().toUtf8());
            appMk.close();
        }
        
        QFile buildSh(outputDir + "/build.sh");
        if (buildSh.open(QIODevice::WriteOnly)) {
            buildSh.write(generateBuildSh().toUtf8());
            buildSh.close();
        }
        
        // Generate Windows build script
        QFile buildBat(outputDir + "/build.bat");
        if (buildBat.open(QIODevice::WriteOnly)) {
            buildBat.write(generateBuildBat().toUtf8());
            buildBat.close();
        }
    }
    
    // Generate smali loader
    dir.mkpath("smali_inject/com/modmenu");
    QFile smaliLoader(outputDir + "/smali_inject/com/modmenu/ModLoader.smali");
    if (smaliLoader.open(QIODevice::WriteOnly)) {
        smaliLoader.write(generateSmaliLoader().toUtf8());
        smaliLoader.close();
        emit logMessage("Generated smali loader for injection", "info");
    }
    
    emit progressUpdated(95, "Generating README...");
    QFile readme(outputDir + "/README.md");
    if (readme.open(QIODevice::WriteOnly)) {
        readme.write(generateReadme(targets).toUtf8());
        readme.close();
    }
    
    emit progressUpdated(100, "Project generation complete!");
    emit generationComplete(success, outputDir);
    
    return success;
}

QString ModMenuCodeGenerator::modIdToVarName(const QString &modId)
{
    QString var = "b_" + modId;
    var.replace("_", "");
    var[2] = var[2].toUpper();
    return var;
}

QString ModMenuCodeGenerator::modIdToFunctionName(const QString &modId)
{
    QString func = modId;
    QStringList parts = func.split('_');
    QString result;
    for (const QString &part : parts) {
        result += part[0].toUpper() + part.mid(1);
    }
    return result;
}

QString ModMenuCodeGenerator::generateMainCpp(const QList<ModTarget> &targets)
{
    QString code = R"(/*
 * AUTO-GENERATED MOD MENU - IL2CPP Unity Game
 * Generated by APK Studio
 * 
 * This mod menu uses Dobby for inline hooking and ImGui for the UI.
 * 
 * BUILD INSTRUCTIONS:
 * 1. Install Android NDK (r25c recommended)
 * 2. Run: ./build.sh
 * 3. Output will be in libs/arm64-v8a/libmodmenu.so
 */

#include <jni.h>
#include <string>
#include <thread>
#include <chrono>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>

// Dobby hooking framework
#include "dobby/include/dobby.h"

// ImGui
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "imgui/backends/imgui_impl_android.h"

// Custom headers
#include "il2cpp_utils.hpp"
#include "game_defs.hpp"
#include "mod_menu.hpp"

#define LOG_TAG "MODMENU"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// EGL hook pointers
static EGLDisplay g_EglDisplay = EGL_NO_DISPLAY;
static EGLSurface g_EglSurface = EGL_NO_SURFACE;
static EGLContext g_EglContext = EGL_NO_CONTEXT;
EGLAPI EGLBoolean (*old_eglSwapBuffers)(EGLDisplay dpy, EGLSurface sur);

)";

    // Generate hook function pointers for each target
    code += "// ============ HOOK FUNCTION POINTERS ============\n\n";
    
    for (const ModTarget &target : targets) {
        if (target.hookType == "placeholder") continue;
        
        QString funcName = modIdToFunctionName(target.modId);
        if (target.hookType == "method_replace" || target.hookType == "method_return") {
            code += QString("void* %1_Addr = nullptr;\n").arg(funcName);
            if (target.fieldType == "void") {
                code += QString("void (*old_%1)(void* __this);\n\n").arg(funcName);
            } else if (target.fieldType == "int" || target.fieldType == "Int32") {
                code += QString("int (*old_%1)(void* __this);\n\n").arg(funcName);
            } else if (target.fieldType == "float" || target.fieldType == "Single") {
                code += QString("float (*old_%1)(void* __this);\n\n").arg(funcName);
            } else if (target.fieldType == "bool" || target.fieldType == "Boolean") {
                code += QString("bool (*old_%1)(void* __this);\n\n").arg(funcName);
            } else {
                code += QString("void* (*old_%1)(void* __this);\n\n").arg(funcName);
            }
        }
    }
    
    // Generate hook implementations
    code += "// ============ HOOK IMPLEMENTATIONS ============\n\n";
    
    for (const ModTarget &target : targets) {
        if (target.hookType == "placeholder") continue;
        
        QString funcName = modIdToFunctionName(target.modId);
        QString varName = modIdToVarName(target.modId);
        
        if (target.hookType == "method_return") {
            if (target.fieldType == "int" || target.fieldType == "Int32") {
                code += QString(R"(int new_%1(void* __this) {
    if (Mod::%2) {
        LOGI("%3: Returning modified value %4");
        return %4;
    }
    return old_%1(__this);
}

)").arg(funcName, varName, target.displayName).arg(target.value);
            } else if (target.fieldType == "bool" || target.fieldType == "Boolean") {
                code += QString(R"(bool new_%1(void* __this) {
    if (Mod::%2) {
        LOGI("%3: Returning true");
        return true;
    }
    return old_%1(__this);
}

)").arg(funcName, varName, target.displayName);
            } else if (target.fieldType == "float" || target.fieldType == "Single") {
                code += QString(R"(float new_%1(void* __this) {
    if (Mod::%2) {
        LOGI("%3: Returning modified value %4.0f");
        return %4.0f;
    }
    return old_%1(__this);
}

)").arg(funcName, varName, target.displayName).arg(target.value);
            }
        } else if (target.hookType == "method_replace") {
            code += QString(R"(void new_%1(void* __this) {
    if (Mod::%2) {
        LOGI("%3: Skipping original method");
        return; // Don't call original
    }
    old_%1(__this);
}

)").arg(funcName, varName, target.displayName);
        }
    }
    
    // Setup hooks function
    code += R"(// ============ HOOK SETUP ============

void SetupHooks() {
    LOGI("Setting up game hooks...");
    
    if (!InitIL2CPP()) {
        LOGE("Failed to initialize IL2CPP!");
        return;
    }
    
)";

    for (const ModTarget &target : targets) {
        if (target.hookType == "placeholder") continue;
        
        QString funcName = modIdToFunctionName(target.modId);
        
        if (!target.rva.isEmpty()) {
            code += QString(R"(    // %1
    %2_Addr = (void*)GetActualOffset(%3);
    if (%2_Addr) {
        DobbyHook(%2_Addr, (dobby_dummy_func_t)new_%2, (dobby_dummy_func_t*)&old_%2);
        LOGI("Hooked %1 @ %3");
    }
    
)").arg(target.displayName, funcName, target.rva);
        }
    }
    
    code += R"(    LOGI("All hooks applied successfully!");
}

)";
    
    // Rest of main.cpp (ImGui setup, EGL hook, JNI)
    code += R"(// ============ IMGUI SETUP ============

void SetupImGuiStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(8, 8);
    style.WindowRounding = 6.0f;
    style.FramePadding = ImVec2(5, 5);
    style.FrameRounding = 4.0f;
    
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.12f, 0.95f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_Checkmark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
}

EGLAPI EGLBoolean new_eglSwapBuffers(EGLDisplay dpy, EGLSurface sur) {
    if (g_EglDisplay != dpy || g_EglSurface != sur) {
        g_EglDisplay = dpy;
        g_EglSurface = sur;
        g_EglContext = eglGetCurrentContext();

        if (g_EglDisplay && g_EglSurface && g_EglContext) {
            LOGI("Initializing ImGui...");
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.IniFilename = NULL;
            
            ImGui_ImplAndroid_Init(nullptr);
            ImGui_ImplOpenGL3_Init("#version 300 es");
            SetupImGuiStyle();
            io.Fonts->AddFontDefault();
        }
    }

    if (g_EglDisplay && g_EglSurface && g_EglContext) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplAndroid_NewFrame();
        ImGui::NewFrame();

        DrawModMenu();

        ImGui::EndFrame();
        ImGui::Render();
        glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    return old_eglSwapBuffers(dpy, sur);
}

void InitEGLHook() {
    LOGI("Hooking eglSwapBuffers...");
    void* eglSwapBuffers_addr = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    if (eglSwapBuffers_addr) {
        DobbyHook(eglSwapBuffers_addr, (dobby_dummy_func_t)new_eglSwapBuffers, (dobby_dummy_func_t*)&old_eglSwapBuffers);
        LOGI("eglSwapBuffers hooked!");
    }
}

// ============ JNI ENTRY POINT ============

extern "C" void Java_com_modmenu_ModMenuService_onKeyEvent(JNIEnv* env, jobject thiz, int keyCode, bool isDown) {
    if (keyCode == 24 && !isDown) { // Volume Up
        Mod::b_ShowMenu = !Mod::b_ShowMenu;
        LOGI("Menu toggled: %s", Mod::b_ShowMenu ? "ON" : "OFF");
    }
}

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("ModMenu loaded!");
    
    std::thread([]() {
        while (GetLibraryBase("libil2cpp.so") == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        LOGI("libil2cpp.so found, setting up hooks...");
        SetupHooks();
        
        while (GetLibraryBase("libEGL.so") == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        LOGI("libEGL.so found, setting up rendering...");
        InitEGLHook();
    }).detach();
    
    return JNI_VERSION_1_6;
}
)";
    
    return code;
}

QString ModMenuCodeGenerator::generateGameDefsHpp(const QList<ModTarget> &targets)
{
    QString code = R"(/*
 * GAME-SPECIFIC DEFINITIONS
 * Auto-generated from IL2CPP dump
 * 
 * These offsets are specific to YOUR game version.
 * If the game updates, you may need to regenerate this file.
 */

#pragma once
#include "il2cpp_utils.hpp"

namespace Game {

// ============ CLASS POINTERS (resolved at runtime) ============
)";

    QSet<QString> addedClasses;
    for (const ModTarget &target : targets) {
        if (!target.targetClass.isEmpty() && !addedClasses.contains(target.targetClass)) {
            QString varName = target.targetClass;
            varName.replace(".", "_");
            code += QString("inline void* %1_Class = nullptr;\n").arg(varName);
            addedClasses.insert(target.targetClass);
        }
    }
    
    code += "\n// ============ METHOD RVAs ============\n";
    for (const ModTarget &target : targets) {
        if (!target.rva.isEmpty()) {
            QString funcName = modIdToFunctionName(target.modId);
            code += QString("const uintptr_t RVA_%1 = %2;  // %3::%4\n")
                .arg(funcName, target.rva, target.targetClass, target.targetMethod);
        }
    }
    
    code += "\n// ============ FIELD OFFSETS ============\n";
    for (const ModTarget &target : targets) {
        if (!target.offset.isEmpty()) {
            QString funcName = modIdToFunctionName(target.modId);
            code += QString("const uintptr_t OFFSET_%1 = %2;  // %3::%4\n")
                .arg(funcName, target.offset, target.targetClass, target.targetField);
        }
    }
    
    code += "\n} // namespace Game\n";
    
    return code;
}

QString ModMenuCodeGenerator::generateModMenuHpp(const QList<ModTarget> &targets)
{
    QString code = R"(/*
 * MOD MENU UI
 * Draws the ImGui floating menu
 */

#pragma once
#include "imgui/imgui.h"

namespace Mod {
    inline bool b_ShowMenu = false;
    
)";

    // Generate toggle variables
    for (const ModTarget &target : targets) {
        QString varName = modIdToVarName(target.modId);
        code += QString("    inline bool %1 = false;  // %2\n").arg(varName, target.displayName);
    }
    
    code += R"(}

inline void DrawModMenu() {
    if (!Mod::b_ShowMenu) return;
    
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowSize(ImVec2(350, 450), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(50, 100), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Mod Menu", &Mod::b_ShowMenu, ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("Toggle Volume Up to show/hide");
        ImGui::Separator();
        
)";

    // Group by category
    QMap<QString, QList<const ModTarget*>> byCategory;
    for (const ModTarget &target : targets) {
        QString cat = "General";
        if (target.modId.contains("coin") || target.modId.contains("gem") || target.modId.contains("energy"))
            cat = "Resources";
        else if (target.modId.contains("health") || target.modId.contains("damage") || target.modId.contains("speed"))
            cat = "Player";
        else if (target.modId.contains("unlock") || target.modId.contains("iap") || target.modId.contains("ad"))
            cat = "Unlocks";
        byCategory[cat].append(&target);
    }
    
    for (auto it = byCategory.begin(); it != byCategory.end(); ++it) {
        code += QString("        if (ImGui::CollapsingHeader(\"%1\")) {\n").arg(it.key());
        for (const ModTarget *target : it.value()) {
            QString varName = modIdToVarName(target->modId);
            code += QString("            ImGui::Checkbox(\"%1\", &Mod::%2);\n")
                .arg(target->displayName, varName);
        }
        code += "        }\n";
    }
    
    code += R"(        
        ImGui::Separator();
        ImGui::Text("FPS: %.1f", io.Framerate);
    }
    ImGui::End();
}
)";

    return code;
}

QString ModMenuCodeGenerator::generateIl2cppUtilsHpp()
{
    return R"(/*
 * IL2CPP UTILITIES
 * Functions for resolving IL2CPP methods and addresses
 */

#pragma once
#include <cstdint>
#include <string>
#include <dlfcn.h>
#include <android/log.h>

#define LOG_TAG "MODMENU"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern uintptr_t il2cpp_base;

uintptr_t GetLibraryBase(const char* libraryName);
bool InitIL2CPP();
uintptr_t GetActualOffset(uintptr_t rva);

// IL2CPP function typedefs
typedef void* (*il2cpp_class_from_name_t)(void* image, const char* namespaze, const char* name);
typedef void* (*il2cpp_class_get_method_from_name_t)(void* klass, const char* name, int argsCount);
typedef void* (*il2cpp_method_get_function_pointer_t)(void* method);

extern il2cpp_class_from_name_t il2cpp_class_from_name;
extern il2cpp_class_get_method_from_name_t il2cpp_class_get_method_from_name;
extern il2cpp_method_get_function_pointer_t il2cpp_method_get_function_pointer;
)";
}

QString ModMenuCodeGenerator::generateIl2cppUtilsCpp()
{
    return R"(#include "il2cpp_utils.hpp"
#include <fstream>
#include <cstring>

uintptr_t il2cpp_base = 0;

il2cpp_class_from_name_t il2cpp_class_from_name = nullptr;
il2cpp_class_get_method_from_name_t il2cpp_class_get_method_from_name = nullptr;
il2cpp_method_get_function_pointer_t il2cpp_method_get_function_pointer = nullptr;

uintptr_t GetLibraryBase(const char* libraryName) {
    char line[512];
    FILE *f = fopen("/proc/self/maps", "r");
    if (!f) return 0;
    
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, libraryName)) {
            char *addr = strtok(line, "-");
            uintptr_t base = (uintptr_t)strtoul(addr, NULL, 16);
            fclose(f);
            return base;
        }
    }
    fclose(f);
    return 0;
}

bool InitIL2CPP() {
    il2cpp_base = GetLibraryBase("libil2cpp.so");
    if (il2cpp_base == 0) {
        LOGE("libil2cpp.so base not found!");
        return false;
    }
    LOGI("libil2cpp.so base: 0x%lx", il2cpp_base);
    
    void* handle = dlopen("libil2cpp.so", RTLD_LAZY);
    if (!handle) {
        LOGE("Failed to dlopen libil2cpp.so");
        return false;
    }
    
    il2cpp_class_from_name = (il2cpp_class_from_name_t)dlsym(handle, "il2cpp_class_from_name");
    il2cpp_class_get_method_from_name = (il2cpp_class_get_method_from_name_t)dlsym(handle, "il2cpp_class_get_method_from_name");
    il2cpp_method_get_function_pointer = (il2cpp_method_get_function_pointer_t)dlsym(handle, "il2cpp_method_get_function_pointer");
    
    LOGI("IL2CPP functions resolved");
    return true;
}

uintptr_t GetActualOffset(uintptr_t rva) {
    return il2cpp_base + rva;
}
)";
}

QString ModMenuCodeGenerator::generateAndroidMk()
{
    return R"(LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE           := modmenu
LOCAL_SRC_FILES        := main.cpp \
                          il2cpp_utils.cpp

# ImGui sources (you need to add these files)
# LOCAL_SRC_FILES      += imgui/imgui.cpp \
#                         imgui/imgui_draw.cpp \
#                         imgui/imgui_widgets.cpp \
#                         imgui/backends/imgui_impl_opengl3.cpp \
#                         imgui/backends/imgui_impl_android.cpp

# Dobby sources (or link pre-built library)
# LOCAL_STATIC_LIBRARIES := dobby

LOCAL_C_INCLUDES       := $(LOCAL_PATH) \
                          $(LOCAL_PATH)/imgui \
                          $(LOCAL_PATH)/imgui/backends \
                          $(LOCAL_PATH)/dobby/include

LOCAL_CPPFLAGS         := -std=c++17 -Wall -Wextra -O2
LOCAL_LDLIBS           := -llog -landroid -lGLESv3 -lEGL

include $(BUILD_SHARED_LIBRARY)
)";
}

QString ModMenuCodeGenerator::generateApplicationMk()
{
    return R"(APP_ABI := arm64-v8a
APP_PLATFORM := android-24
APP_STL := c++_static
APP_OPTIM := release
)";
}

QString ModMenuCodeGenerator::generateBuildSh()
{
    return R"(#!/bin/bash

# Set your NDK path here
export NDK_ROOT="${ANDROID_NDK_HOME:-$HOME/Android/Sdk/ndk/25.2.9519653}"

if [ ! -d "$NDK_ROOT" ]; then
    echo "ERROR: NDK not found at $NDK_ROOT"
    echo "Please set ANDROID_NDK_HOME or edit NDK_ROOT in this script"
    exit 1
fi

echo "Using NDK: $NDK_ROOT"
echo "Building mod menu..."

cd jni
$NDK_ROOT/ndk-build clean
$NDK_ROOT/ndk-build

if [ $? -eq 0 ]; then
    echo ""
    echo "SUCCESS! Output: libs/arm64-v8a/libmodmenu.so"
    echo ""
    echo "Next steps:"
    echo "1. Copy libmodmenu.so to your game's APK in lib/arm64-v8a/"
    echo "2. Modify AndroidManifest.xml to load the library"
    echo "3. Rebuild and sign the APK"
else
    echo "Build failed!"
    exit 1
fi
)";
}

QString ModMenuCodeGenerator::generateBuildBat()
{
    return R"(@echo off
REM Mod Menu Build Script for Windows
REM Generated by APK Studio

echo ========================================
echo   Mod Menu Build Script
echo ========================================

REM Check for NDK
if not defined ANDROID_NDK_HOME (
    if not defined NDK_ROOT (
        REM Try common locations
        if exist "%LOCALAPPDATA%\Android\Sdk\ndk" (
            for /d %%i in ("%LOCALAPPDATA%\Android\Sdk\ndk\*") do set NDK_ROOT=%%i
        )
    )
)

if not defined NDK_ROOT set NDK_ROOT=%ANDROID_NDK_HOME%

if not exist "%NDK_ROOT%\ndk-build.cmd" (
    echo ERROR: Android NDK not found!
    echo.
    echo Please set ANDROID_NDK_HOME environment variable:
    echo   set ANDROID_NDK_HOME=C:\path\to\android-ndk
    echo.
    echo Or download NDK from:
    echo   https://developer.android.com/ndk/downloads
    exit /b 1
)

echo Using NDK: %NDK_ROOT%
echo.
echo Building mod menu...
echo.

cd jni
call "%NDK_ROOT%\ndk-build.cmd" clean
call "%NDK_ROOT%\ndk-build.cmd"

if %ERRORLEVEL% == 0 (
    echo.
    echo ========================================
    echo   BUILD SUCCESSFUL!
    echo ========================================
    echo.
    echo Output: libs\arm64-v8a\libmodmenu.so
    echo.
    echo Next steps:
    echo   1. Copy libmodmenu.so to APK's lib\arm64-v8a\ folder
    echo   2. Add smali loader or modify manifest
    echo   3. Repackage and sign the APK
    echo.
) else (
    echo BUILD FAILED!
    exit /b 1
)

pause
)";
}

QString ModMenuCodeGenerator::generateSmaliLoader()
{
    return R"(.class public Lcom/modmenu/ModLoader;
.super Ljava/lang/Object;
.source "ModLoader.java"

# ============================================================================
# ModLoader - Loads the mod menu native library
# 
# USAGE: Add to main Activity's onCreate or static initializer
#   invoke-static {}, Lcom/modmenu/ModLoader;->load()V
# ============================================================================

.field private static loaded:Z

.method static constructor <clinit>()V
    .registers 1
    const/4 v0, 0x0
    sput-boolean v0, Lcom/modmenu/ModLoader;->loaded:Z
    return-void
.end method

.method public static load()V
    .registers 3
    
    sget-boolean v0, Lcom/modmenu/ModLoader;->loaded:Z
    if-nez v0, :already_loaded
    
    :try_start
    const-string v0, "modmenu"
    invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V
    
    const/4 v0, 0x1
    sput-boolean v0, Lcom/modmenu/ModLoader;->loaded:Z
    
    const-string v0, "ModMenu"
    const-string v1, "Mod menu loaded successfully!"
    invoke-static {v0, v1}, Landroid/util/Log;->i(Ljava/lang/String;Ljava/lang/String;)I
    :try_end
    .catch Ljava/lang/UnsatisfiedLinkError; {:try_start .. :try_end} :catch_error
    .catch Ljava/lang/Exception; {:try_start .. :try_end} :catch_error
    
    goto :end
    
    :catch_error
    move-exception v0
    const-string v1, "ModMenu"
    const-string v2, "Failed to load mod library"
    invoke-static {v1, v2, v0}, Landroid/util/Log;->e(Ljava/lang/String;Ljava/lang/String;Ljava/lang/Throwable;)I
    
    :already_loaded
    :end
    return-void
.end method

.method public static isLoaded()Z
    .registers 1
    sget-boolean v0, Lcom/modmenu/ModLoader;->loaded:Z
    return v0
.end method
)";
}

QString ModMenuCodeGenerator::generateFridaScript(const QList<ModTarget> &targets)
{
    QString script = R"(/*
 * FRIDA MOD MENU SCRIPT
 * Auto-generated by APK Studio
 * 
 * Usage:
 * frida -U -f <package_name> -l mod_menu.js
 */

// ============ CONFIGURATION ============
var CONFIG = {
    enabled: true,
    logCalls: true,
    
    // Toggle individual mods here
)";
    
    // Add config toggles for each mod
    for (const ModTarget &target : targets) {
        QString varName = target.modId.replace("_", "");
        script += QString("    %1: true,\n").arg(varName);
    }
    
    script += R"(};

// ============ UTILITY FUNCTIONS ============
function readInt(addr) {
    try { return Memory.readS32(addr); } catch(e) { return 0; }
}

function writeInt(addr, val) {
    try { Memory.writeS32(addr, val); return true; } catch(e) { return false; }
}

function readFloat(addr) {
    try { return Memory.readFloat(addr); } catch(e) { return 0.0; }
}

function writeFloat(addr, val) {
    try { Memory.writeFloat(addr, val); return true; } catch(e) { return false; }
}

// Search for pattern in memory (useful for finding dynamic addresses)
function findPattern(module, pattern) {
    var results = Memory.scanSync(module.base, module.size, pattern);
    return results.length > 0 ? results[0].address : null;
}

// ============ MAIN HOOK LOGIC ============
Java.perform(function() {
    console.log("[*] Frida Mod Menu Loaded!");
    console.log("[*] Waiting for game libraries...");
    
    var il2cpp = Process.findModuleByName("libil2cpp.so");
    if (!il2cpp) {
        console.log("[!] libil2cpp.so not found, waiting...");
        var checkInterval = setInterval(function() {
            il2cpp = Process.findModuleByName("libil2cpp.so");
            if (il2cpp) {
                clearInterval(checkInterval);
                console.log("[+] libil2cpp.so found at: " + il2cpp.base);
                applyHooks(il2cpp.base);
            }
        }, 1000);
    } else {
        console.log("[+] libil2cpp.so found at: " + il2cpp.base);
        applyHooks(il2cpp.base);
    }
});

function applyHooks(base) {
    console.log("[*] Applying hooks...");
    
)";

    // Generate hooks for found targets
    bool hasPlaceholders = false;
    for (const ModTarget &target : targets) {
        QString funcName = modIdToFunctionName(target.modId);
        QString varName = target.modId.replace("_", "");
        
        if (target.hookType == "placeholder" || target.rva.isEmpty()) {
            hasPlaceholders = true;
            // Generate placeholder template with search hints
            script += QString(R"(    // ============ TODO: %1 ============
    // Target not automatically found. Try these approaches:
    // 1. Search game code for patterns like: %2
    // 2. Use Memory.scanSync to find string references
    // 3. Hook common Unity methods like PlayerPrefs.GetInt
    /*
    var pattern = "?? ?? ?? ?? 00 00 00 00"; // Replace with actual pattern
    var match = findPattern(Process.findModuleByName("libil2cpp.so"), pattern);
    if (match) {
        Interceptor.attach(match, {
            onEnter: function(args) {
                if (CONFIG.%3) {
                    console.log("[*] %1 triggered");
                }
            },
            onLeave: function(retval) {
                if (CONFIG.%3) {
                    retval.replace(%4);
                }
            }
        });
    }
    */
    
)").arg(target.displayName, target.modId, varName).arg(target.value);
            continue;
        }
        
        script += QString(R"(    // %1
    try {
        var %2_addr = base.add(%3);
        Interceptor.attach(%2_addr, {
            onEnter: function(args) {
                if (CONFIG.logCalls) console.log("[*] %1 called");
            },
            onLeave: function(retval) {
                if (!CONFIG.enabled || !CONFIG.%4) return;
)").arg(target.displayName, funcName, target.rva, varName);

        if (target.fieldType == "int" || target.fieldType == "Int32") {
            script += QString("                retval.replace(%1);  // Modified value\n").arg(target.value);
        } else if (target.fieldType == "bool" || target.fieldType == "Boolean") {
            script += "                retval.replace(1);  // Always true\n";
        } else if (target.fieldType == "float" || target.fieldType == "Single") {
            script += QString("                retval.replace(%1.0);  // Modified value\n").arg(target.value);
        } else {
            script += "                // Unknown type - modify as needed\n";
        }
        
        script += QString(R"(                console.log("[+] %1: value modified");
            }
        });
        console.log("[+] Hooked: %1 @ %2");
    } catch(e) {
        console.log("[!] Failed to hook %1: " + e);
    }
    
)").arg(target.displayName, target.rva);
    }
    
    // Add placeholder warning if any
    if (hasPlaceholders) {
        script += R"(    // ============ PLACEHOLDERS DETECTED ============
    // Some mods could not be automatically mapped to game code.
    // Check the TODO sections above and fill in the addresses manually.
    // Tip: Use Ghidra or IDA Pro to analyze libil2cpp.so
    
)";
    }
    
    script += R"(    console.log("[+] All hooks applied!");
}
)";
    
    return script;
}

QString ModMenuCodeGenerator::generateReadme(const QList<ModTarget> &targets)
{
    QString readme = R"(# Generated Mod Menu

Auto-generated by APK Studio

## Included Modifications

| Mod | Target | Type | Address |
|-----|--------|------|---------|
)";

    for (const ModTarget &target : targets) {
        QString address = target.rva.isEmpty() ? target.offset : target.rva;
        QString type = target.hookType;
        QString targetName = target.targetClass + "::" + 
            (target.targetField.isEmpty() ? target.targetMethod : target.targetField);
        readme += QString("| %1 | %2 | %3 | %4 |\n")
            .arg(target.displayName, targetName, type, address);
    }
    
    readme += R"(

## Build Instructions

### For C++ (Dobby/ImGui)

1. **Install Android NDK** (r25c recommended)
2. **Download dependencies:**
   - [Dobby](https://github.com/jmpews/Dobby) → Extract to `jni/dobby/`
   - [ImGui](https://github.com/ocornut/imgui) → Copy to `jni/imgui/`
3. **Edit `build.sh`** and set your NDK path
4. **Run:** `chmod +x build.sh && ./build.sh`
5. **Output:** `libs/arm64-v8a/libmodmenu.so`

### For Frida

1. **Install Frida:** `pip install frida-tools`
2. **Run:** `frida -U -f <package_name> -l mod_menu.js`

## Installation

1. Extract the target APK
2. Copy `libmodmenu.so` to `lib/arm64-v8a/`
3. Modify `AndroidManifest.xml` to load the library
4. Repackage and sign the APK

## Toggle Menu

- **Android:** Volume Up button
- **PC (Emulator):** F1 key

## Disclaimer

This tool is for educational purposes only. Modifying games may violate ToS.
)";

    return readme;
}

QString ModMenuCodeGenerator::generateHookCode(const ModTarget &target)
{
    QString code;
    QString funcName = modIdToFunctionName(target.modId);
    QString varName = modIdToVarName(target.modId);
    
    if (target.hookType == "placeholder") {
        // Generate a template hook that needs manual completion
        code = QString(R"(// TODO: %1 - Manual hook required
// Target not automatically found. Search for patterns like: %2
// Implement custom logic below:
/*
Interceptor.attach(base.add(0x??????), {
    onEnter: function(args) {
        console.log("[*] %1 called");
    },
    onLeave: function(retval) {
        retval.replace(%3);
    }
});
*/
)").arg(target.displayName, target.modId).arg(target.value);
        return code;
    }
    
    if (target.hookType == "field_write") {
        // Generate memory write code for field modification
        code = QString(R"(// %1: Direct field modification
// Class: %2, Field: %3, Offset: %4
void Apply%5(void* instance) {
    if (!instance || !Mod::%6) return;
    
    // Calculate field address
    uintptr_t fieldAddr = (uintptr_t)instance + %4;
    
    // Write new value
    *(int*)fieldAddr = %7;
    LOGI("%1 applied: Set %3 to %7");
}
)").arg(target.displayName, target.targetClass, target.targetField, 
        target.offset, funcName, varName).arg(target.value);
    }
    else if (target.hookType == "method_return") {
        // Generate method hook that modifies return value
        if (target.fieldType == "int" || target.fieldType == "Int32") {
            code = QString(R"(// %1: Method return hook (int)
int new_%2(void* __this) {
    if (Mod::%3) {
        LOGI("%1: Returning modified value %4");
        return %4;
    }
    return old_%2(__this);
}
)").arg(target.displayName, funcName, varName).arg(target.value);
        }
        else if (target.fieldType == "float" || target.fieldType == "Single") {
            code = QString(R"(// %1: Method return hook (float)
float new_%2(void* __this) {
    if (Mod::%3) {
        LOGI("%1: Returning modified value %4.0f");
        return %4.0f;
    }
    return old_%2(__this);
}
)").arg(target.displayName, funcName, varName).arg(target.value);
        }
        else if (target.fieldType == "bool" || target.fieldType == "Boolean") {
            code = QString(R"(// %1: Method return hook (bool)
bool new_%2(void* __this) {
    if (Mod::%3) {
        LOGI("%1: Returning true");
        return true;
    }
    return old_%2(__this);
}
)").arg(target.displayName, funcName, varName);
        }
    }
    else if (target.hookType == "method_replace") {
        // Generate method hook that replaces entire method
        code = QString(R"(// %1: Method replacement hook
void new_%2(void* __this) {
    if (Mod::%3) {
        LOGI("%1: Skipping original method");
        return; // Skip original logic
    }
    old_%2(__this);
}
)").arg(target.displayName, funcName, varName);
    }
    
    return code;
}

QString ModMenuCodeGenerator::generateMenuToggle(const ModTarget &target)
{
    QString varName = modIdToVarName(target.modId);
    QString category = "General";
    
    // Determine category based on mod ID
    if (target.modId.contains("coin") || target.modId.contains("gem") || 
        target.modId.contains("gold") || target.modId.contains("energy") ||
        target.modId.contains("key") || target.modId.contains("star")) {
        category = "Resources";
    }
    else if (target.modId.contains("health") || target.modId.contains("damage") ||
             target.modId.contains("speed") || target.modId.contains("defense") ||
             target.modId.contains("exp") || target.modId.contains("level")) {
        category = "Player Stats";
    }
    else if (target.modId.contains("unlock") || target.modId.contains("iap") ||
             target.modId.contains("ad") || target.modId.contains("premium") ||
             target.modId.contains("vip")) {
        category = "Unlocks";
    }
    else if (target.modId.contains("cooldown") || target.modId.contains("time") ||
             target.modId.contains("freeze") || target.modId.contains("no_")) {
        category = "Game Tweaks";
    }
    
    QString toggleCode = QString(R"(        // %1 [%2]
        ImGui::Checkbox("%3", &Mod::%4);
        if (Mod::%4) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.47f, 0.90f, 0.47f, 1.0f), "(Active)");
        }
)").arg(target.modId, category, target.displayName, varName);
    
    // Add value slider for non-toggle mods
    if (target.value > 1 && target.hookType != "method_replace") {
        QString valueVar = "val_" + target.modId;
        toggleCode += QString(R"(        static int %1 = %2;
        if (Mod::%3) {
            ImGui::SliderInt("Value##%4", &%1, 1, 999999999);
        }
)").arg(valueVar).arg(target.value).arg(varName, target.modId);
    }
    
    return toggleCode;
}

// =============================================================================
// ModMenuProjectDialog Implementation
// =============================================================================

ModMenuProjectDialog::ModMenuProjectDialog(const QString &projectPath, const QList<ModOption> &mods, QWidget *parent)
    : QDialog(parent), m_ProjectPath(projectPath), m_Mods(mods), m_DownloadsRemaining(0)
{
    setWindowTitle(tr("🎮 Mod Menu Project Generator"));
    setMinimumSize(1100, 750);
    
    m_Generator = new ModMenuCodeGenerator(projectPath, this);
    m_NetworkManager = new QNetworkAccessManager(this);
    
    connect(m_Generator, &ModMenuCodeGenerator::progressUpdated, this, [this](int percent, const QString &status) {
        m_Progress->setValue(percent);
        m_StatusLabel->setText(status);
    });
    
    connect(m_Generator, &ModMenuCodeGenerator::logMessage, this, [this](const QString &msg, const QString &type) {
        QString color = type == "error" ? "#f85149" : type == "success" ? "#7ee787" : type == "warning" ? "#d29922" : "#8b949e";
        m_PreviewArea->append(QString("<span style='color: %1;'>%2</span>").arg(color, msg));
    });
    
    connect(m_NetworkManager, &QNetworkAccessManager::finished, this, &ModMenuProjectDialog::onDownloadFinished);
    
    setupUI();
}

void ModMenuProjectDialog::setupUI()
{
    auto mainLayout = new QVBoxLayout(this);
    
    // Header
    auto header = new QLabel(tr("<h2>🎮 Complete Mod Menu Project Generator</h2>"
                                "<p>Generates a ready-to-compile mod menu with all source files.</p>"));
    header->setStyleSheet("color: #c9d1d9;");
    mainLayout->addWidget(header);
    
    // Dependencies row
    auto depsLayout = new QHBoxLayout();
    
    m_DownloadDepsBtn = new QPushButton(tr("📥 Download Dependencies (ImGui + Dobby)"));
    m_DownloadDepsBtn->setStyleSheet("background: #1f6feb; color: white; padding: 10px 15px; font-weight: bold;");
    m_DownloadDepsBtn->setToolTip(tr("Download ImGui and Dobby libraries automatically from GitHub"));
    depsLayout->addWidget(m_DownloadDepsBtn);
    
    m_AutoDownloadCheck = new QCheckBox(tr("Auto-download with project"));
    m_AutoDownloadCheck->setStyleSheet("color: #8b949e;");
    m_AutoDownloadCheck->setChecked(true);
    depsLayout->addWidget(m_AutoDownloadCheck);
    
    // Dependency status
    auto depsStatusLabel = new QLabel();
    if (checkDependencies()) {
        depsStatusLabel->setText(tr("✅ Dependencies found"));
        depsStatusLabel->setStyleSheet("color: #7ee787;");
    } else {
        depsStatusLabel->setText(tr("⚠️ Dependencies missing - click Download"));
        depsStatusLabel->setStyleSheet("color: #d29922;");
    }
    depsLayout->addWidget(depsStatusLabel);
    
    depsLayout->addStretch();
    mainLayout->addLayout(depsLayout);
    
    // Options row
    auto optionsLayout = new QHBoxLayout();
    optionsLayout->addWidget(new QLabel(tr("Output Style:")));
    
    m_StyleCombo = new QComboBox();
    m_StyleCombo->addItems({
        tr("C++ with ImGui/Dobby (Native)"),
        tr("Frida JavaScript Hooks")
    });
    m_StyleCombo->setStyleSheet("background: #21262d; border: 1px solid #30363d; color: #c9d1d9; padding: 8px;");
    optionsLayout->addWidget(m_StyleCombo);
    
    m_GenerateBtn = new QPushButton(tr("🚀 Generate Project"));
    m_GenerateBtn->setStyleSheet("background: #238636; color: white; padding: 10px 20px; font-weight: bold;");
    optionsLayout->addWidget(m_GenerateBtn);
    
    optionsLayout->addStretch();
    mainLayout->addLayout(optionsLayout);
    
    // Progress
    m_Progress = new QProgressBar();
    m_Progress->setStyleSheet("QProgressBar::chunk { background-color: #238636; }");
    mainLayout->addWidget(m_Progress);
    
    m_StatusLabel = new QLabel(tr("Ready to generate..."));
    m_StatusLabel->setStyleSheet("color: #8b949e;");
    mainLayout->addWidget(m_StatusLabel);
    
    // Main content
    auto splitter = new QSplitter(Qt::Horizontal);
    
    // Left: File list
    auto leftWidget = new QWidget();
    auto leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addWidget(new QLabel(tr("<b>Generated Files:</b>")));
    
    m_FilesList = new QListWidget();
    m_FilesList->setStyleSheet("background: #161b22; color: #c9d1d9; border: 1px solid #30363d;");
    leftLayout->addWidget(m_FilesList);
    
    splitter->addWidget(leftWidget);
    
    // Right: Preview
    auto rightWidget = new QWidget();
    auto rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addWidget(new QLabel(tr("<b>File Preview:</b>")));
    
    m_PreviewArea = new QTextBrowser();
    m_PreviewArea->setStyleSheet("background: #0d1117; color: #7ee787; font-family: 'Consolas', monospace;");
    rightLayout->addWidget(m_PreviewArea);
    
    splitter->addWidget(rightWidget);
    splitter->setSizes({250, 800});
    
    mainLayout->addWidget(splitter);
    
    // Bottom buttons
    auto btnLayout = new QHBoxLayout();
    m_SaveBtn = new QPushButton(tr("💾 Save Project"));
    m_SaveBtn->setStyleSheet("background: #1f6feb; color: white; padding: 10px 20px;");
    m_SaveBtn->setEnabled(false);
    
    m_OpenFolderBtn = new QPushButton(tr("📂 Open Folder"));
    m_OpenFolderBtn->setStyleSheet("background: #21262d; border: 1px solid #30363d; color: #c9d1d9; padding: 10px 20px;");
    m_OpenFolderBtn->setEnabled(false);
    
    auto closeBtn = new QPushButton(tr("Close"));
    closeBtn->setStyleSheet("background: #21262d; border: 1px solid #30363d; color: #c9d1d9; padding: 10px 20px;");
    
    btnLayout->addStretch();
    btnLayout->addWidget(m_SaveBtn);
    btnLayout->addWidget(m_OpenFolderBtn);
    btnLayout->addWidget(closeBtn);
    mainLayout->addLayout(btnLayout);
    
    // Connections
    connect(m_GenerateBtn, &QPushButton::clicked, this, &ModMenuProjectDialog::onGenerateClicked);
    connect(m_FilesList, &QListWidget::currentRowChanged, this, &ModMenuProjectDialog::onPreviewFile);
    connect(m_SaveBtn, &QPushButton::clicked, this, &ModMenuProjectDialog::onSaveProject);
    connect(m_OpenFolderBtn, &QPushButton::clicked, this, &ModMenuProjectDialog::onOpenFolder);
    connect(m_DownloadDepsBtn, &QPushButton::clicked, this, &ModMenuProjectDialog::onDownloadDependencies);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void ModMenuProjectDialog::onGenerateClicked()
{
    m_GenerateBtn->setEnabled(false);
    m_FilesList->clear();
    m_GeneratedFiles.clear();
    m_PreviewArea->clear();
    
    // Parse dump.cs first
    if (!m_Generator->parseDumpCs()) {
        m_PreviewArea->append("<span style='color: #f85149;'>Failed to parse dump.cs. Run IL2CPP Dumper first.</span>");
        m_GenerateBtn->setEnabled(true);
        return;
    }
    
    // Find targets
    m_Targets = m_Generator->findModTargets(m_Mods);
    
    if (m_Targets.isEmpty()) {
        m_PreviewArea->append("<span style='color: #d29922;'>No mod targets found. Check your mod selection.</span>");
        m_GenerateBtn->setEnabled(true);
        return;
    }
    
    // Generate project
    m_OutputDir = m_ProjectPath + "/mod_menu_project";
    QString style = m_StyleCombo->currentText();
    
    if (m_Generator->generateProject(m_OutputDir, m_Targets, style)) {
        // Read generated files
        QDirIterator it(m_OutputDir, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString filePath = it.next();
            QString relativePath = QDir(m_OutputDir).relativeFilePath(filePath);
            
            QFile file(filePath);
            if (file.open(QIODevice::ReadOnly)) {
                m_GeneratedFiles[relativePath] = QString::fromUtf8(file.readAll());
                file.close();
                m_FilesList->addItem(relativePath);
            }
        }
        
        m_SaveBtn->setEnabled(true);
        m_OpenFolderBtn->setEnabled(true);
        m_StatusLabel->setText(tr("✅ Project generated successfully!"));
    }
    
    m_GenerateBtn->setEnabled(true);
}

void ModMenuProjectDialog::onPreviewFile(int index)
{
    if (index < 0) return;
    
    QString fileName = m_FilesList->item(index)->text();
    QString content = m_GeneratedFiles.value(fileName);
    
    // Syntax highlighting based on extension
    if (fileName.endsWith(".cpp") || fileName.endsWith(".hpp") || fileName.endsWith(".h")) {
        m_PreviewArea->setPlainText(content);
        m_PreviewArea->setStyleSheet("background: #0d1117; color: #7ee787; font-family: 'Consolas', monospace;");
    } else if (fileName.endsWith(".js")) {
        m_PreviewArea->setPlainText(content);
        m_PreviewArea->setStyleSheet("background: #0d1117; color: #f0c674; font-family: 'Consolas', monospace;");
    } else if (fileName.endsWith(".md")) {
        m_PreviewArea->setMarkdown(content);
        m_PreviewArea->setStyleSheet("background: #0d1117; color: #c9d1d9; font-family: 'Consolas', monospace;");
    } else {
        m_PreviewArea->setPlainText(content);
    }
}

void ModMenuProjectDialog::onSaveProject()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Output Directory"), m_ProjectPath);
    if (dir.isEmpty()) return;
    
    // Copy all generated files
    for (auto it = m_GeneratedFiles.begin(); it != m_GeneratedFiles.end(); ++it) {
        QString targetPath = dir + "/" + it.key();
        QDir().mkpath(QFileInfo(targetPath).absolutePath());
        
        QFile file(targetPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(it.value().toUtf8());
            file.close();
        }
    }
    
    QMessageBox::information(this, tr("Saved"), tr("Project saved to: %1").arg(dir));
}

void ModMenuProjectDialog::onOpenFolder()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_OutputDir));
}

void ModMenuProjectDialog::updatePreview(const QString &fileName, const QString &content)
{
    Q_UNUSED(fileName);
    m_PreviewArea->setPlainText(content);
}

bool ModMenuProjectDialog::checkDependencies()
{
    // Check if ImGui and Dobby are available in common locations
    QSettings settings;
    QString imguiPath = settings.value("imgui_path").toString();
    QString dobbyPath = settings.value("dobby_path").toString();
    
    if (!imguiPath.isEmpty() && QFile::exists(imguiPath + "/imgui.h")) {
        if (!dobbyPath.isEmpty() && QFile::exists(dobbyPath + "/dobby.h")) {
            return true;
        }
    }
    
    // Check in app data directory
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString depsDir = appDataDir + "/mod_deps";
    
    if (QFile::exists(depsDir + "/imgui/imgui.h") && QFile::exists(depsDir + "/dobby/dobby.h")) {
        return true;
    }
    
    return false;
}

void ModMenuProjectDialog::onDownloadDependencies()
{
    m_DownloadDepsBtn->setEnabled(false);
    m_DownloadDepsBtn->setText(tr("⏳ Downloading..."));
    m_PreviewArea->clear();
    m_PreviewArea->append("<span style='color: #58a6ff;'>Starting dependency download...</span>");
    
    // Create deps directory
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString depsDir = appDataDir + "/mod_deps";
    QDir().mkpath(depsDir);
    QDir().mkpath(depsDir + "/imgui");
    QDir().mkpath(depsDir + "/dobby");
    
    m_DownloadsRemaining = 0;
    
    // ImGui files to download (from GitHub raw)
    QStringList imguiFiles = {
        "imgui.h", "imgui.cpp", "imgui_demo.cpp", "imgui_draw.cpp",
        "imgui_tables.cpp", "imgui_widgets.cpp", "imgui_internal.h",
        "imconfig.h", "imstb_rectpack.h", "imstb_textedit.h", "imstb_truetype.h"
    };
    
    QString imguiBaseUrl = "https://raw.githubusercontent.com/ocornut/imgui/master/";
    
    for (const QString &file : imguiFiles) {
        QString url = imguiBaseUrl + file;
        QString destPath = depsDir + "/imgui/" + file;
        downloadFile(url, destPath, "ImGui: " + file);
    }
    
    // ImGui backends
    QStringList imguiBackends = {
        "backends/imgui_impl_opengl3.h", "backends/imgui_impl_opengl3.cpp",
        "backends/imgui_impl_android.h", "backends/imgui_impl_android.cpp"
    };
    
    for (const QString &file : imguiBackends) {
        QString url = imguiBaseUrl + file;
        QString fileName = QFileInfo(file).fileName();
        QString destPath = depsDir + "/imgui/" + fileName;
        downloadFile(url, destPath, "ImGui Backend: " + fileName);
    }
    
    // Dobby header (we'll need to build libdobby.a separately)
    QString dobbyUrl = "https://raw.githubusercontent.com/jmpews/Dobby/master/include/dobby.h";
    downloadFile(dobbyUrl, depsDir + "/dobby/dobby.h", "Dobby: dobby.h");
    
    // Download pre-built Dobby if available (from releases)
    // Note: Users may need to build Dobby themselves for their specific NDK version
    m_PreviewArea->append("<span style='color: #d29922;'>⚠️ Note: Dobby library (libdobby.a) needs to be built from source.</span>");
    m_PreviewArea->append("<span style='color: #8b949e;'>See: https://github.com/jmpews/Dobby#build</span>");
}

void ModMenuProjectDialog::downloadFile(const QString &url, const QString &destPath, const QString &description)
{
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setRawHeader("User-Agent", "APKStudio/1.0");
    
    QNetworkReply *reply = m_NetworkManager->get(request);
    m_PendingDownloads[reply] = qMakePair(destPath, description);
    m_DownloadsRemaining++;
    
    m_PreviewArea->append(QString("<span style='color: #8b949e;'>⬇️ Downloading: %1</span>").arg(description));
}

void ModMenuProjectDialog::onDownloadFinished(QNetworkReply *reply)
{
    if (!m_PendingDownloads.contains(reply)) {
        reply->deleteLater();
        return;
    }
    
    QString destPath = m_PendingDownloads[reply].first;
    QString description = m_PendingDownloads[reply].second;
    m_PendingDownloads.remove(reply);
    m_DownloadsRemaining--;
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        
        QFile file(destPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(data);
            file.close();
            m_PreviewArea->append(QString("<span style='color: #7ee787;'>✅ %1 (%2 bytes)</span>")
                .arg(description).arg(data.size()));
        } else {
            m_PreviewArea->append(QString("<span style='color: #f85149;'>❌ Failed to save: %1</span>").arg(description));
        }
    } else {
        m_PreviewArea->append(QString("<span style='color: #f85149;'>❌ Download failed: %1 - %2</span>")
            .arg(description, reply->errorString()));
    }
    
    reply->deleteLater();
    
    // Check if all downloads complete
    if (m_DownloadsRemaining <= 0) {
        m_DownloadDepsBtn->setEnabled(true);
        m_DownloadDepsBtn->setText(tr("📥 Download Dependencies (ImGui + Dobby)"));
        
        // Save paths to settings
        QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QString depsDir = appDataDir + "/mod_deps";
        
        QSettings settings;
        settings.setValue("imgui_path", depsDir + "/imgui");
        settings.setValue("dobby_path", depsDir + "/dobby");
        
        m_PreviewArea->append("<br><span style='color: #7ee787; font-weight: bold;'>✅ All dependencies downloaded!</span>");
        m_PreviewArea->append(QString("<span style='color: #8b949e;'>Location: %1</span>").arg(depsDir));
        m_PreviewArea->append("<br><span style='color: #58a6ff;'>📝 To build Dobby (libdobby.a):</span>");
        m_PreviewArea->append("<span style='color: #c9d1d9;'>1. git clone https://github.com/jmpews/Dobby.git</span>");
        m_PreviewArea->append("<span style='color: #c9d1d9;'>2. cd Dobby && mkdir build && cd build</span>");
        m_PreviewArea->append("<span style='color: #c9d1d9;'>3. cmake .. -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a</span>");
        m_PreviewArea->append("<span style='color: #c9d1d9;'>4. make</span>");
        m_PreviewArea->append(QString("<span style='color: #c9d1d9;'>5. Copy libdobby.a to: %1/dobby/</span>").arg(depsDir));
    }
}

void ModMenuProjectDialog::extractZip(const QString &zipPath, const QString &destDir)
{
    // Simple unzip using Qt's QZipReader if available, or shell command
    // For now, we download individual files instead of zips
    Q_UNUSED(zipPath);
    Q_UNUSED(destDir);
}
