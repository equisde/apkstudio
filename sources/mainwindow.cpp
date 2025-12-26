#include "mainwindow.h"
#include "gamemodtools.h"
#include "apkdecompiledialog.h"
#include "apkdecompileworker.h"
#include <QStyleHints>
#include <QApplication>
#include <QSplitter>
#include <QVBoxLayout>
#include <QToolButton>
#include <QStatusBar>
#include <QMenuBar>
#include <QMessageBox>
#include <QDir>
#include <QDebug>
#include <QThread>
#include <QProgressDialog>
#include <QSettings>

MainWindow::MainWindow(const QMap<QString, QString> &versions, QWidget *parent)
    : QMainWindow(parent)
{
    setupModernStyles();
    
    // Central Splitter (Sidebar + Editor Area)
    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal, this);
    
    // 1. Activity Bar (Izquierda extrema)
    setupActivityBar();
    
    // 2. Sidebar Stack (Explorer, Search, Modding, etc.)
    m_SidebarStack = new QStackedWidget();
    m_SidebarStack->setMinimumWidth(300);
    m_SidebarStack->setMaximumWidth(450);
    setupSidebars();
    
    // 3. Central Editor Area
    m_CentralStack = new QStackedWidget();
    auto welcome = new QLabel("<h1>APK Studio Pro</h1><p>Open a project to begin AI-guided analysis.</p>");
    welcome->setAlignment(Qt::AlignCenter);
    m_CentralStack->addWidget(welcome);

    mainSplitter->addWidget(m_SidebarStack);
    mainSplitter->addWidget(m_CentralStack);
    mainSplitter->setStretchFactor(1, 1);

    setCentralWidget(mainSplitter);
    
    // Status Bar Modernizada
    setStatusBar(new QStatusBar());
    m_StatusProjectInfo = new QLabel("No Project");
    m_StatusEngineInfo = new QLabel("");
    statusBar()->addPermanentWidget(m_StatusProjectInfo);
    statusBar()->addPermanentWidget(m_StatusEngineInfo);
    
    switchSection(Explorer);
}

void MainWindow::setupModernStyles()
{
    // VS Code Dark Theme Colors
    setStyleSheet(R"(
        QMainWindow { background-color: #1e1e1e; }
        QToolBar#ActivityBar { 
            background-color: #333333; 
            border: none; 
            spacing: 10px; 
            padding-top: 10px;
        }
        QToolButton#ActivityButton {
            background-color: transparent;
            border: none;
            color: #858585;
            padding: 10px;
        }
        QToolButton#ActivityButton:checked {
            color: #ffffff;
            border-left: 2px solid #ffffff;
        }
        QStackedWidget#Sidebar { background-color: #252526; border-right: 1px solid #3c3c3c; }
        QTreeWidget { background-color: #252526; color: #cccccc; border: none; }
        QStatusBar { background-color: #007acc; color: #ffffff; }
    )");
}

void MainWindow::setupActivityBar()
{
    m_ActivityBar = new QToolBar("Activity Bar", this);
    m_ActivityBar->setObjectName("ActivityBar");
    m_ActivityBar->setMovable(false);
    m_ActivityBar->setOrientation(Qt::Vertical);
    m_ActivityBar->setIconSize(QSize(28, 28));
    addToolBar(Qt::LeftToolBarArea, m_ActivityBar);

    auto addActivity = [&](Section s, const QString &icon, const QString &tip) {
        QAction *act = m_ActivityBar->addAction(QIcon(icon), tip);
        act->setCheckable(true);
        act->setData(s);
        connect(act, &QAction::triggered, this, [this, act]() {
            switchSection(static_cast<Section>(act->data().toInt()));
        });
    };

    addActivity(Explorer, ":/icons/icons8/icons8-folder-48.png", "Explorer");
    addActivity(GameModding, ":/icons/icons8/icons8-hammer-48.png", "Game Modding");
    addActivity(Security, ":/icons/icons8/icons8-software-installer-48.png", "Security & Bypasses");
    addActivity(Cloning, ":/icons/icons8/icons8-android-os-48.png", "APK Cloning");
    addActivity(AIStudio, ":/icons/icons8/icons8-gear-48.png", "AI Mod Studio");
}

void MainWindow::switchSection(Section section)
{
    m_SidebarStack->setCurrentIndex(static_cast<int>(section));
    // Actualizar estados visuales de la Activity Bar
}

void MainWindow::analyzeProjectContext(const QString &path)
{
    m_CurrentProjectPath = path;
    updateStatusBar("Analyzing environment...");
    
    // Motor de Detección Cautelosa
    GameEngineDetector::Engine engine = GameEngineDetector::detectEngine(path);
    
    if (engine != GameEngineDetector::NativeAndroid) {
        m_DetectedContext = "Game: " + GameEngineDetector::engineName(engine);
        // Generar MD de Modding orientado al motor
        QString report = QString("# Game Modding Analysis\nEngine: %1\nStatus: Ready for Injection").arg(m_DetectedContext);
        QFile f(path + "/ANALYSIS_GAME.md");
        if(f.open(QFile::WriteOnly)) { f.write(report.toUtf8()); f.close(); }
    } else {
        // Analizar si es Web/Framework
        if (QDir(path + "/assets/www").exists()) m_DetectedContext = "App: Vue/Cordova";
        else if (QFile::exists(path + "/lib/arm64-v8a/libflutter.so")) m_DetectedContext = "App: Flutter";
        else m_DetectedContext = "App: Native (Kotlin/Java)";
        
        QString report = "# App Modding Analysis\nFramework: " + m_DetectedContext + "\nAI Suggestion: Ready for UI patches";
        QFile f(path + "/ANALYSIS_APP.md");
        if(f.open(QFile::WriteOnly)) { f.write(report.toUtf8()); f.close(); }
    }
    
    m_StatusEngineInfo->setText(m_DetectedContext);
    m_StatusProjectInfo->setText(QFileInfo(path).fileName());
}

void MainWindow::setupSidebars()
{
    // 0. Explorer
    m_ExplorerTree = new QTreeWidget();
    m_ExplorerTree->setHeaderLabel("PROJECT EXPLORER");
    m_SidebarStack->addWidget(m_ExplorerTree);

    // 1. Placeholder para Search
    m_SidebarStack->addWidget(new QLabel("Search..."));

    // 2. Game Modding Panel
    auto gameMod = new QWidget();
    auto gmLayout = new QVBoxLayout(gameMod);
    gmLayout->addWidget(new QLabel("GAME MOD ENGINE"));
    auto runDumper = new QPushButton("🚀 Run Engine Dumper (IA)");
    gmLayout->addWidget(runDumper);
    m_SidebarStack->addWidget(gameMod);

    // 3. Security Panel
    auto secPanel = new QWidget();
    auto secLayout = new QVBoxLayout(secPanel);
    secLayout->addWidget(new QLabel("SECURITY HUB"));
    secLayout->addWidget(new QPushButton("🛡️ SSL Unpinning (IA)"));
    secLayout->addWidget(new QPushButton("🔒 Anti-Tampering Bypass"));
    m_SidebarStack->addWidget(secPanel);

    // 4. Cloning Panel
    auto clonePanel = new QWidget();
    auto cloneLayout = new QVBoxLayout(clonePanel);
    cloneLayout->addWidget(new QLabel("APK CLONING STUDIO"));
    cloneLayout->addWidget(new QPushButton("👥 Create Clone Package"));
    m_SidebarStack->addWidget(clonePanel);
}

void MainWindow::updateStatusBar(const QString &msg) { statusBar()->showMessage(msg); }

void MainWindow::openApkFile(const QString &apkPath)
{
    if (apkPath.isEmpty() || !QFile::exists(apkPath)) {
        return;
    }
    
    auto dialog = new ApkDecompileDialog(QDir::toNativeSeparators(apkPath), this);
    if (dialog->exec() == QDialog::Accepted) {
        auto thread = new QThread();
        auto worker = new ApkDecompileWorker(dialog->apk(), dialog->folder(), dialog->smali(), dialog->resources(), dialog->java(), dialog->frameworkTag(), dialog->extraArguments());
        worker->moveToThread(thread);
        
        connect(worker, &ApkDecompileWorker::decompileFailed, this, &MainWindow::handleDecompileFailed);
        connect(worker, &ApkDecompileWorker::decompileFinished, this, &MainWindow::handleDecompileFinished);
        connect(worker, &ApkDecompileWorker::decompileProgress, this, &MainWindow::handleDecompileProgress);
        connect(thread, &QThread::started, worker, &ApkDecompileWorker::decompile);
        connect(worker, &ApkDecompileWorker::finished, thread, &QThread::quit);
        connect(worker, &ApkDecompileWorker::finished, worker, &QObject::deleteLater);
        connect(thread, &QThread::finished, thread, &QObject::deleteLater);
        
        m_GlobalProgress = new QProgressDialog(this);
        m_GlobalProgress->setCancelButton(nullptr);
        m_GlobalProgress->setLabelText(tr("Decompiling APK..."));
        m_GlobalProgress->setRange(0, 100);
        m_GlobalProgress->setWindowFlags(m_GlobalProgress->windowFlags() & ~Qt::WindowCloseButtonHint);
        m_GlobalProgress->setWindowTitle(tr("Decompiling"));
        
        thread->start();
        m_GlobalProgress->exec();
    }
    dialog->deleteLater();
}

void MainWindow::handleDecompileFinished(const QString &apk, const QString &folder)
{
    Q_UNUSED(apk)
    if (m_GlobalProgress) {
        m_GlobalProgress->close();
        m_GlobalProgress->deleteLater();
        m_GlobalProgress = nullptr;
    }
    updateStatusBar(tr("Decompilation finished."));
    openProject(folder);
}

void MainWindow::handleDecompileFailed(const QString &apk)
{
    Q_UNUSED(apk)
    if (m_GlobalProgress) {
        m_GlobalProgress->close();
        m_GlobalProgress->deleteLater();
        m_GlobalProgress = nullptr;
    }
    updateStatusBar(tr("Decompilation failed."));
    QMessageBox::critical(this, tr("Error"), tr("Failed to decompile APK. Check console for details."));
}

void MainWindow::handleDecompileProgress(int percent, const QString &message)
{
    if (m_GlobalProgress) {
        m_GlobalProgress->setLabelText(message);
        m_GlobalProgress->setValue(percent);
    }
}

void MainWindow::openProject(const QString &folder)
{
    QSettings settings;
    settings.setValue("open_project", folder);
    
    analyzeProjectContext(folder);
    
    // Refresh explorer tree (stub for now)
    m_ExplorerTree->clear();
    auto root = new QTreeWidgetItem(m_ExplorerTree);
    root->setText(0, QFileInfo(folder).fileName());
    root->setData(0, Qt::UserRole, folder);
    m_ExplorerTree->addTopLevelItem(root);
}

QString MainWindow::getCurrentProjectPath()
{
    if (m_ExplorerTree->topLevelItemCount() > 0) {
        return m_ExplorerTree->topLevelItem(0)->data(0, Qt::UserRole).toString();
    }
    return QString();
}

void MainWindow::handleToolAIGameMod() {}
void MainWindow::handleSecurityAnalysis() {}
void MainWindow::handleApkCloning() {}
void MainWindow::handleAppModification() {}
MainWindow::~MainWindow() {}