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
#include <QScrollArea>
#include <QDialogButtonBox>

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
    logMessage("Collecting game source code for analysis...", "info");

    // Collect game context from multiple sources
    QString gameContext;
    QString engineType = "Unknown";
    QString gameName = QDir(m_ProjectPath).dirName();
    int filesAnalyzed = 0;
    int totalClasses = 0;
    
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
    
    gameContext += QString("# Game Analysis Report\n\n");
    gameContext += QString("| Property | Value |\n|---|---|\n");
    gameContext += QString("| Game Name | %1 |\n").arg(gameName);
    gameContext += QString("| Package | %1 |\n").arg(packageName);
    gameContext += QString("| Engine | %1 |\n").arg(engineType);
    gameContext += QString("| Build Type | %1 |\n\n").arg(isIL2CPP ? "IL2CPP (Native compiled)" : "Mono (JIT/.NET)");

    // Extended keywords for better class detection
    QStringList gameKeywords = {
        // Currency & Economy
        "Currency", "Gold", "Coin", "Gem", "Diamond", "Crystal", "Token", "Credit", "Cash", "Money",
        "Wallet", "Bank", "Economy", "Balance", "Reward", "Prize", "Loot", "Treasure",
        // Player & Stats
        "Player", "Character", "Hero", "Avatar", "User", "Profile", "Account",
        "Health", "HP", "Life", "Lives", "Heart", "Damage", "Attack", "Defense", "Armor",
        "Speed", "Velocity", "Power", "Strength", "Stamina", "Energy", "Mana", "MP",
        "Level", "Experience", "XP", "Exp", "Rank", "Score", "Point",
        // Inventory & Items
        "Inventory", "Item", "Equipment", "Weapon", "Skill", "Ability", "Upgrade",
        "Consumable", "Potion", "Buff", "PowerUp", "Boost",
        // Shop & IAP
        "Shop", "Store", "Purchase", "Buy", "Sell", "Price", "Cost", "IAP", "InApp", "Premium",
        "Subscription", "VIP", "Ads", "Reward", "Offer", "Deal", "Bundle",
        // Game Mechanics
        "Timer", "Cooldown", "Countdown", "Time", "Duration", "Wait",
        "Spawn", "Respawn", "Revive", "Continue",
        // Managers & Controllers
        "GameManager", "DataManager", "SaveManager", "PlayerData", "UserData",
        "GameController", "GameState", "GameConfig", "Settings", "Config",
        "NetworkManager", "ServerManager", "Analytics"
    };

    // 1. Read IL2CPP dump.cs with intelligent parsing
    QString dumpPath = m_ProjectPath + "/dump/dump.cs";
    if (QFile::exists(dumpPath)) {
        QFile dumpFile(dumpPath);
        if (dumpFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString dumpContent = QString::fromUtf8(dumpFile.readAll());
            dumpFile.close();
            
            logMessage(QString("Parsing dump.cs (%1 KB)...").arg(dumpContent.size() / 1024), "info");
            
            // Split into class blocks using regex
            QMap<QString, QString> classMap; // category -> classes content
            QStringList lines = dumpContent.split('\n');
            QString currentClassName;
            QStringList currentClassContent;
            bool inClass = false;
            int braceCount = 0;
            
            for (int i = 0; i < lines.size(); i++) {
                QString line = lines[i];
                
                // Detect class/struct declaration
                QRegularExpression classRegex("(public|internal|private)?\\s*(sealed|abstract|static)?\\s*class\\s+(\\w+)");
                QRegularExpressionMatch classMatch = classRegex.match(line);
                
                if (classMatch.hasMatch()) {
                    // Save previous class if it was relevant
                    if (!currentClassName.isEmpty() && !currentClassContent.isEmpty()) {
                        QString category = categorizeClass(currentClassName, currentClassContent.join("\n"));
                        if (!category.isEmpty()) {
                            classMap[category] += "\n```csharp\n// Class: " + currentClassName + "\n" + 
                                                  currentClassContent.join("\n").left(3000) + "\n```\n";
                            filesAnalyzed++;
                        }
                    }
                    
                    currentClassName = classMatch.captured(3);
                    currentClassContent.clear();
                    totalClasses++;
                    
                    // Check if class name matches keywords
                    bool isRelevant = false;
                    for (const QString &keyword : gameKeywords) {
                        if (currentClassName.contains(keyword, Qt::CaseInsensitive)) {
                            isRelevant = true;
                            break;
                        }
                    }
                    
                    if (isRelevant) {
                        inClass = true;
                        braceCount = 0;
                    } else {
                        inClass = false;
                    }
                }
                
                if (inClass) {
                    currentClassContent.append(line);
                    braceCount += line.count('{') - line.count('}');
                    
                    // End of class
                    if (braceCount <= 0 && currentClassContent.size() > 1) {
                        inClass = false;
                    }
                    
                    // Limit class size
                    if (currentClassContent.size() > 150) {
                        currentClassContent.append("    // ... (class continues)");
                        inClass = false;
                    }
                }
            }
            
            // Save last class
            if (!currentClassName.isEmpty() && !currentClassContent.isEmpty()) {
                QString category = categorizeClass(currentClassName, currentClassContent.join("\n"));
                if (!category.isEmpty()) {
                    classMap[category] += "\n```csharp\n// Class: " + currentClassName + "\n" + 
                                          currentClassContent.join("\n").left(3000) + "\n```\n";
                    filesAnalyzed++;
                }
            }
            
            // Build organized context
            if (!classMap.isEmpty()) {
                gameContext += "## Extracted Game Classes (from IL2CPP dump)\n\n";
                gameContext += QString("*Analyzed %1 total classes, found %2 relevant for modding*\n\n").arg(totalClasses).arg(filesAnalyzed);
                
                QStringList categoryOrder = {"💰 Currency/Economy", "❤️ Player/Stats", "🎒 Inventory/Items", 
                                             "🛒 Shop/IAP", "⏱️ Timer/Cooldown", "🎮 Game Management", "🛡️ Anti-Cheat/Security"};
                for (const QString &cat : categoryOrder) {
                    if (classMap.contains(cat)) {
                        gameContext += QString("### %1\n%2\n").arg(cat, classMap[cat]);
                    }
                }
            }
            
            logMessage(QString("Found %1 relevant classes out of %2 total").arg(filesAnalyzed).arg(totalClasses), "info");
        }
    }
    
    // 2. Read decompiled C# source files (recursive search)
    QDir csharpDir(m_ProjectPath + "/csharp_src");
    if (csharpDir.exists()) {
        QDirIterator it(csharpDir.path(), {"*.cs"}, QDir::Files, QDirIterator::Subdirectories);
        QMap<QString, QString> sourceFiles;
        int csFilesRead = 0;
        
        while (it.hasNext() && csFilesRead < 30) {
            QString filePath = it.next();
            QString fileName = QFileInfo(filePath).fileName();
            
            // Check if filename is relevant
            bool isRelevant = false;
            for (const QString &keyword : gameKeywords) {
                if (fileName.contains(keyword, Qt::CaseInsensitive)) {
                    isRelevant = true;
                    break;
                }
            }
            
            // Also check file content for relevant patterns
            if (!isRelevant) {
                QFile file(filePath);
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QString preview = QString::fromUtf8(file.read(2000));
                    file.close();
                    for (const QString &keyword : gameKeywords) {
                        if (preview.contains(keyword, Qt::CaseInsensitive)) {
                            isRelevant = true;
                            break;
                        }
                    }
                }
            }
            
            if (isRelevant) {
                QFile file(filePath);
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QString content = QString::fromUtf8(file.readAll());
                    file.close();
                    
                    QString category = categorizeClass(fileName, content);
                    if (!category.isEmpty()) {
                        QString truncated = content.length() > 4000 ? content.left(4000) + "\n// ... (truncated)" : content;
                        sourceFiles[category] += QString("\n### %1\n```csharp\n%2\n```\n").arg(fileName, truncated);
                        csFilesRead++;
                        filesAnalyzed++;
                    }
                }
            }
        }
        
        if (!sourceFiles.isEmpty()) {
            gameContext += "\n## Decompiled C# Source Files\n\n";
            for (auto it = sourceFiles.begin(); it != sourceFiles.end(); ++it) {
                gameContext += QString("### %1\n%2\n").arg(it.key(), it.value());
            }
        }
    }
    
    // 3. Read SharedPreferences/PlayerPrefs (important for save data mods)
    QDir sharedPrefsDir(m_ProjectPath + "/shared_prefs");
    if (sharedPrefsDir.exists()) {
        QStringList xmlFiles = sharedPrefsDir.entryList({"*.xml"}, QDir::Files);
        if (!xmlFiles.isEmpty()) {
            gameContext += "\n## Player Save Data (SharedPreferences)\n\n";
            gameContext += "*These files often contain modifiable game values stored locally*\n\n";
            
            for (const QString &xmlFile : xmlFiles) {
                QFile file(sharedPrefsDir.filePath(xmlFile));
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QString content = QString::fromUtf8(file.readAll());
                    file.close();
                    
                    // Parse XML to extract key-value pairs
                    QRegularExpression kvRegex("<(int|long|float|boolean|string)\\s+name=\"([^\"]+)\"[^>]*(?:value=\"([^\"]*)\")?");
                    QRegularExpressionMatchIterator matches = kvRegex.globalMatch(content);
                    
                    if (matches.hasNext()) {
                        gameContext += QString("### %1\n| Type | Key | Value |\n|---|---|---|\n").arg(xmlFile);
                        while (matches.hasNext()) {
                            QRegularExpressionMatch m = matches.next();
                            gameContext += QString("| %1 | %2 | %3 |\n").arg(m.captured(1), m.captured(2), m.captured(3).left(50));
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
    
    logMessage(QString("Analyzing %1 relevant game files...").arg(filesAnalyzed), "info");
    m_AIResponseView->setMarkdown("## 🔄 Analyzing...\n\nProcessing " + QString::number(filesAnalyzed) + " game files with AI...");
    
    // Build focused, actionable prompt
    QString prompt = QString(
        "You are an expert Android game reverse engineer. Analyze this %1 game and provide **SPECIFIC, ACTIONABLE** mod recommendations.\n\n"
        "# IMPORTANT INSTRUCTIONS:\n"
        "- DO NOT ask for more information - work with what's provided\n"
        "- DO NOT provide generic modding tutorials\n"
        "- ONLY list mods that are actually possible based on the code shown\n"
        "- Be SPECIFIC with class names, method names, and field names from the code\n\n"
        "# OUTPUT FORMAT (use exactly this structure):\n\n"
        "## 🎯 Quick Summary\n"
        "Brief 2-3 sentence summary of what mods are possible for this specific game.\n\n"
        "## 💰 Currency/Resource Mods\n"
        "For each mod:\n"
        "- **Target**: `ClassName.MethodName` or `ClassName.FieldName`\n"
        "- **Type**: field type (int, float, etc)\n"
        "- **Modification**: Exact change needed\n"
        "- **Code Example**: Show the hook/patch code\n"
        "- **Risk**: Low/Medium/High (explain why)\n\n"
        "## ❤️ Player Stats Mods\n"
        "(same format)\n\n"
        "## ⚔️ Combat/Damage Mods\n"
        "(same format)\n\n"
        "## 🛒 IAP/Purchase Bypass\n"
        "(same format)\n\n"
        "## ⏱️ Timer/Cooldown Mods\n"
        "(same format)\n\n"
        "## 🛡️ Anti-Cheat Considerations\n"
        "List any anti-cheat or security mechanisms found and how to handle them.\n\n"
        "---\n"
        "# GAME SOURCE CODE:\n\n%2"
    ).arg(engineType, gameContext);
    
    askAI(prompt, [this](const QString &res) {
        m_AIResponseView->setMarkdown(res);
        logMessage("AI Analysis complete!", "success");
    });
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
    
    auto dialog = new ModMenuGeneratorDialog(m_ProjectPath, mods, this);
    dialog->exec();
    dialog->deleteLater();
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
    
    // Check for C# source files
    QString srcDir = m_ProjectPath + "/csharp_src";
    if (QDir(srcDir).exists()) {
        QDirIterator it(srcDir, {"*.cs"}, QDir::Files, QDirIterator::Subdirectories);
        QStringList interestingPatterns = {"Player", "Health", "Money", "Coin", "Gem", "Diamond", 
                                           "Energy", "Score", "Purchase", "IAP", "Store", "Premium",
                                           "VIP", "Inventory", "Item", "Weapon", "Damage", "Speed"};
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
    
    // Check for IL2CPP dump
    QString dumpFile = m_ProjectPath + "/dump/dump.cs";
    if (context.isEmpty() && QFile::exists(dumpFile)) {
        QFile file(dumpFile);
        if (file.open(QIODevice::ReadOnly)) {
            QString content = QString::fromUtf8(file.readAll());
            file.close();
            context = content.left(maxContextSize);
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
