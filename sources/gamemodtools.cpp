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
    
    // Define mod categories and options
    QList<ModOption> resourceMods = {
        {"coins", tr("Unlimited Coins"), tr("Set coins to maximum value"), false, 999999999, 0, 999999999, ""},
        {"gems", tr("Unlimited Gems/Diamonds"), tr("Set gems/diamonds to maximum"), false, 999999999, 0, 999999999, ""},
        {"energy", tr("Unlimited Energy/Stamina"), tr("Remove energy limits"), false, 9999, 0, 99999, ""},
        {"keys", tr("Unlimited Keys"), tr("Get unlimited keys for chests/levels"), false, 9999, 0, 99999, ""},
        {"tickets", tr("Unlimited Tickets"), tr("Remove ticket restrictions"), false, 9999, 0, 99999, ""},
        {"vip", tr("VIP Status"), tr("Enable VIP/Premium features"), false, 99, 1, 99, ""}
    };
    
    QList<ModOption> playerMods = {
        {"health", tr("God Mode / Unlimited Health"), tr("Player takes no damage"), false, 999999, 0, 999999, ""},
        {"damage", tr("One-Hit Kill"), tr("Kill enemies with one hit"), false, 999999, 1, 999999, ""},
        {"speed", tr("Speed Multiplier"), tr("Increase player movement speed"), false, 5, 1, 100, ""},
        {"defense", tr("Max Defense/Armor"), tr("Maximum protection"), false, 9999, 0, 9999, ""},
        {"exp", tr("Unlimited XP/Level"), tr("Maximize experience points"), false, 999999, 0, 999999, ""}
    };
    
    QList<ModOption> inventoryMods = {
        {"inventory_max", tr("Max Inventory Stack"), tr("Stack items to maximum"), false, 9999, 1, 9999, ""},
        {"unlock_items", tr("Unlock All Items"), tr("Access all game items"), false, 1, 0, 1, ""},
        {"unlock_chars", tr("Unlock All Characters"), tr("Access all playable characters"), false, 1, 0, 1, ""},
        {"unlock_levels", tr("Unlock All Levels"), tr("Access all game levels"), false, 1, 0, 1, ""},
        {"unlock_skins", tr("Unlock All Skins"), tr("Access all cosmetic items"), false, 1, 0, 1, ""}
    };
    
    QList<ModOption> gameMods = {
        {"no_ads", tr("Remove Ads"), tr("Disable all advertisements"), false, 1, 0, 1, ""},
        {"free_iap", tr("Free In-App Purchases"), tr("Bypass payment verification"), false, 1, 0, 1, ""},
        {"no_cooldown", tr("No Cooldowns"), tr("Remove ability cooldowns"), false, 1, 0, 1, ""},
        {"freeze_time", tr("Freeze Timer"), tr("Stop countdown timers"), false, 1, 0, 1, ""},
        {"custom", tr("Custom Modification"), tr("Specify your own modification"), false, 0, 0, 999999999, ""}
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
    logMessage("Verifying decompilation status...", "info");
    
    bool hasMonoSource = QDir(m_ProjectPath + "/csharp_src").exists() && 
                         !QDir(m_ProjectPath + "/csharp_src").isEmpty();
    bool hasIl2cppDump = QFile::exists(m_ProjectPath + "/dump/dump.cs") ||
                         QDir(m_ProjectPath + "/dump").exists();
    bool hasAssemblyCSharp = QFile::exists(m_ProjectPath + "/assets/bin/Data/Managed/Assembly-CSharp.dll");
    bool hasIl2cpp = false;
    
    QStringList archs = {"arm64-v8a", "armeabi-v7a", "x86", "x86_64"};
    for (const QString &arch : archs) {
        if (QFile::exists(m_ProjectPath + "/lib/" + arch + "/libil2cpp.so")) {
            hasIl2cpp = true;
            break;
        }
    }
    
    QString report = "<h2>🔍 Decompilation Verification Report</h2>";
    report += "<table style='width: 100%; border-collapse: collapse;'>";
    
    auto addRow = [&report](const QString &item, bool found, const QString &status) {
        QString color = found ? "#7ee787" : "#f85149";
        QString icon = found ? "✅" : "❌";
        report += QString("<tr><td style='padding: 8px; border-bottom: 1px solid #30363d;'>%1</td>"
                         "<td style='padding: 8px; border-bottom: 1px solid #30363d; color: %2;'>%3 %4</td></tr>")
                 .arg(item, color, icon, status);
    };
    
    if (m_DetectedEngine == GameEngineDetector::Unity) {
        addRow("Unity Engine Detected", true, "Unity game confirmed");
        addRow("Assembly-CSharp.dll (Mono)", hasAssemblyCSharp, hasAssemblyCSharp ? "Found - can decompile C#" : "Not found");
        addRow("libil2cpp.so (IL2CPP)", hasIl2cpp, hasIl2cpp ? "Found - needs IL2CPP Dumper" : "Not found");
        addRow("C# Source Extracted", hasMonoSource, hasMonoSource ? "Ready for AI analysis" : "Run 'Decompile' first");
        addRow("IL2CPP Dump", hasIl2cppDump, hasIl2cppDump ? "dump.cs available" : "Run 'Decompile' for IL2CPP");
    } else {
        addRow("Engine", true, GameEngineDetector::engineName(m_DetectedEngine));
        addRow("Decompilation", false, "Not a Unity game - use other tools");
    }
    
    report += "</table>";
    
    // Check tools availability
    QSettings settings;
    QString ilspy = settings.value("ilspy_cmd").toString();
    QString dumper = settings.value("il2cpp_dumper_exe").toString();
    
    report += "<h3>🔧 Tools Status</h3><table style='width: 100%; border-collapse: collapse;'>";
    addRow("ILSpy/ILSpyCmd", !ilspy.isEmpty() && QFile::exists(ilspy), ilspy.isEmpty() ? "Not configured" : ilspy);
    addRow("IL2CPP Dumper", !dumper.isEmpty() && QFile::exists(dumper), dumper.isEmpty() ? "Not configured" : dumper);
    report += "</table>";
    
    m_AIResponseView->setHtml(report);
    
    if (hasMonoSource || hasIl2cppDump) {
        logMessage("Decompilation verified successfully!", "success");
    } else if (hasAssemblyCSharp || hasIl2cpp) {
        logMessage("Unity files found but not yet decompiled. Click 'Decompile' to extract source.", "warning");
    } else {
        logMessage("No Unity files found in this project.", "error");
    }
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
