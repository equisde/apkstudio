#include "mainwindow.h"
#include "apkdecompiledialog.h"
#include "apkdecompileworker.h"
#include "antisplitworker.h"
#include "settingsdialog.h"
#include "advancedcodeeditor.h"
#include "imageviewerwidget.h"
#include "markdownviewerwidget.h"
#include "hexedit.h"
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QStatusBar>
#include <QMenuBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QSettings>
#include <QThread>
#include <QActionGroup>
#include <QApplication>
#include <QTimer>
#include <QDir>

MainWindow::MainWindow(const QMap<QString, QString> &versions, QWidget *parent)
    : QMainWindow(parent)
{
    setupModernStyles();
    
    auto centralWidget = new QWidget(this);
    auto mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    setCentralWidget(centralWidget);

    setupActivityBar();
    mainLayout->addWidget(m_ActivityBar);

    auto mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainSplitter->setHandleWidth(1);
    mainSplitter->setStyleSheet("QSplitter::handle { background-color: #3c3c3c; }");

    m_SidebarContainer = new QWidget();
    auto sideLayout = new QVBoxLayout(m_SidebarContainer);
    sideLayout->setContentsMargins(0, 0, 0, 0);
    sideLayout->setSpacing(0);
    
    m_SidebarStack = new QStackedWidget();
    sideLayout->addWidget(m_SidebarStack);
    
    m_SidebarContainer->setMinimumWidth(300);
    m_SidebarContainer->setMaximumWidth(500);
    mainSplitter->addWidget(m_SidebarContainer);

    m_CentralStack = new QStackedWidget();
    m_TabEditors = new QTabWidget();
    m_TabEditors->setTabsClosable(true);
    m_TabEditors->setMovable(true);
    m_TabEditors->setDocumentMode(true);
    m_TabEditors->setStyleSheet("QTabBar::tab { height: 35px; min-width: 100px; }");
    connect(m_TabEditors, &QTabWidget::tabCloseRequested, this, &MainWindow::handleTabCloseRequested);
    connect(m_TabEditors, &QTabWidget::currentChanged, this, &MainWindow::handleTabChanged);

    auto welcome = new QLabel("<h1 style='color: #555;'>APK Studio Pro</h1><p style='color: #777;'>Open an APK or XAPK to begin the dissection.</p>");
    welcome->setAlignment(Qt::AlignCenter);
    m_CentralStack->addWidget(welcome);
    m_CentralStack->addWidget(m_TabEditors);
    
    mainSplitter->addWidget(m_CentralStack);
    mainSplitter->setStretchFactor(1, 1);
    mainLayout->addWidget(mainSplitter);

    setupMenuBar();
    setupStatusBarCustom(versions);
    setupSidebars();

    QTimer::singleShot(100, this, [this]() {
        QSettings settings;
        QString last = settings.value("open_project").toString();
        if (!last.isEmpty() && QDir(last).exists()) analyzeProjectContext(last);
    });
}

void MainWindow::setupMenuBar()
{
    auto menu = menuBar();
    menu->setStyleSheet("QMenuBar { background-color: #3c3c3c; color: #cccccc; padding: 5px; } QMenuBar::item:selected { background-color: #505050; border-radius: 4px; }");

    auto fileMenu = menu->addMenu(tr("&File"));
    fileMenu->addAction(QIcon(":/icons/icons8/icons8-android-os-48.png"), tr("Open &APK..."), QKeySequence::New, this, &MainWindow::handleActionApk);
    fileMenu->addAction(QIcon(":/icons/icons8/icons8-folder-48.png"), tr("Open &Folder..."), QKeySequence::Open, this, &MainWindow::handleActionFolder);
    fileMenu->addSeparator();
    m_ActionSave = fileMenu->addAction(tr("&Save"), QKeySequence::Save, this, &MainWindow::handleActionSave);
    m_ActionSaveAll = fileMenu->addAction(tr("Save &All"), this, &MainWindow::handleActionSaveAll);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("&Settings"), QKeySequence::Preferences, this, &MainWindow::handleActionSettings);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("&Quit"), QKeySequence::Quit, this, &MainWindow::handleActionQuit);

    auto viewMenu = menu->addMenu(tr("&View"));
    auto sideAct = viewMenu->addAction(tr("Show Sidebar"));
    sideAct->setCheckable(true);
    sideAct->setChecked(true);
    connect(sideAct, &QAction::toggled, this, &MainWindow::toggleSidebar);

    auto toolsMenu = menu->addMenu(tr("&Tools"));
    auto gameMod = toolsMenu->addMenu(tr("🎮 Game Modding"));
    gameMod->addAction(tr("AI Mod Studio"), QKeySequence("Ctrl+Shift+G"), this, &MainWindow::handleToolAIGameMod);
    
    auto secHub = toolsMenu->addMenu(tr("🛡️ Security Hub"));
    secHub->addAction(tr("Analyze Network Security"), this, &MainWindow::handleSecurityAnalysis);
    
    toolsMenu->addAction(tr("APK Cloner"), this, &MainWindow::handleApkCloning);
}

void MainWindow::setupActivityBar()
{
    m_ActivityBar = new QToolBar(this);
    m_ActivityBar->setObjectName("ActivityBar");
    m_ActivityBar->setOrientation(Qt::Vertical);
    m_ActivityBar->setMovable(false);
    m_ActivityBar->setIconSize(QSize(28, 28));
    m_ActivityBar->setFixedWidth(50);
    m_ActivityBar->setStyleSheet("background-color: #333333; border: none; padding-top: 10px;");

    m_ActivityGroup = new QActionGroup(this);
    m_ActivityGroup->setExclusive(true);

    auto addAct = [&](Section s, const QString &icon, const QString &text) {
        auto act = m_ActivityBar->addAction(QIcon(icon), text);
        act->setCheckable(true);
        act->setData(s);
        m_ActivityGroup->addAction(act);
        if (s == Explorer) act->setChecked(true);
    };

    addAct(Explorer, ":/icons/icons8/icons8-folder-48.png", "Explorer");
    addAct(GameModding, ":/icons/icons8/icons8-hammer-48.png", "Game Mod");
    addAct(Security, ":/icons/icons8/icons8-software-installer-48.png", "Security");
    addAct(Cloning, ":/icons/icons8/icons8-android-os-48.png", "Cloner");
    addAct(AppMod, ":/icons/fugue/gear.png", "App Modification");
    addAct(AIStudio, ":/icons/icons8/icons8-gear-48.png", "AI Agent");

    connect(m_ActivityGroup, &QActionGroup::triggered, this, [this](QAction *a) {
        switchSection(static_cast<Section>(a->data().toInt()));
    });
}

void MainWindow::setupSidebars()
{
    m_ExplorerTree = new QTreeWidget();
    m_ExplorerTree->setHeaderLabel(tr("PROJECT EXPLORER"));
    m_ExplorerTree->setStyleSheet("QTreeWidget { background-color: #252526; color: #cccccc; border: none; }");
    connect(m_ExplorerTree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item) {
        QString path = item->data(0, Qt::UserRole).toString();
        if (!path.isEmpty() && QFileInfo(path).isFile()) openFile(path);
    });
    m_SidebarStack->addWidget(m_ExplorerTree);

    m_SidebarStack->addWidget(new QLabel("AI Search Coming Soon..."));
    m_SidebarStack->addWidget(new QLabel("Open a Project..."));
    m_SidebarStack->addWidget(new QLabel("Open a Project..."));
    m_SidebarStack->addWidget(new QLabel("Open a Project..."));
    m_SidebarStack->addWidget(new QLabel("Open a Project..."));

    m_AIStudioWidget = new AIConsoleWidget();
    m_SidebarStack->addWidget(m_AIStudioWidget);
}

void MainWindow::analyzeProjectContext(const QString &path) {
    m_CurrentProjectPath = path;

    updateStatusBar(tr("AI Analyzing Environment..."));

    // 1. Detección de Motor
    GameEngineDetector::Engine engine = GameEngineDetector::detectEngine(path);
    m_DetectedContext = GameEngineDetector::engineName(engine);
    
    // 2. Inyectar Widgets Reales en el Sidebar (Reemplaza los labels iniciales)
    m_SidebarStack->removeWidget(m_SecurityHub);
    if(m_SecurityHub) m_SecurityHub->deleteLater();
    m_SecurityHub = new SecurityHub(path);
    m_SidebarStack->insertWidget(Security, m_SecurityHub);
    
    m_SidebarStack->removeWidget(m_CloningStudio);
    if(m_CloningStudio) m_CloningStudio->deleteLater();
    m_CloningStudio = new CloningStudio(path);
    m_SidebarStack->insertWidget(Cloning, m_CloningStudio);
    
    m_SidebarStack->removeWidget(m_AppModStudio);
    if(m_AppModStudio) m_AppModStudio->deleteLater();
    m_AppModStudio = new AppModStudio(path);
    m_SidebarStack->insertWidget(AppMod, m_AppModStudio);

    // Il2Cpp Studio Pro Integration
    m_SidebarStack->removeWidget(m_Il2CppStudio);
    if(m_Il2CppStudio) m_Il2CppStudio->deleteLater();
    m_Il2CppStudio = new Il2CppStudio(path);
    m_SidebarStack->insertWidget(GameModding, m_Il2CppStudio);

    // 3. Actualizar Explorador
    m_ExplorerTree->clear();
    auto root = new QTreeWidgetItem(m_ExplorerTree);
    root->setText(0, QFileInfo(path).fileName());
    root->setData(0, Qt::UserRole, path);
    reloadChildren(root);
    root->setExpanded(true);

    m_StatusEngineInfo->setText("Context: " + m_DetectedContext);
    if (m_AIStudioWidget) m_AIStudioWidget->setProjectPath(path);
    
    // Generar Reportes MD Automáticos
    QDir dir(path);
    QString report;
    if (engine != GameEngineDetector::NativeAndroid) {
        report = "# AI Game Modding Report\nEngine: " + m_DetectedContext + "\n\n## Vectors\n- IL2CPP detected\n- Assets ready.";
        QFile f(path + "/AI_GAME_MOD.md");
        if(f.open(QFile::WriteOnly)) { f.write(report.toUtf8()); f.close(); }
    } else {
        report = "# AI App Modding Report\nContext: Native Android\n\n## Recommendations\n- Scan for Smali patches.";
        QFile f(path + "/AI_APP_AUDIT.md");
        if(f.open(QFile::WriteOnly)) { f.write(report.toUtf8()); f.close(); }
    }
    updateStatusBar(tr("Analysis Complete. Reports generated."));
}

void MainWindow::switchSection(Section section) {
    m_SidebarStack->setCurrentIndex(static_cast<int>(section));
    if (!m_SidebarContainer->isVisible()) m_SidebarContainer->show();
}

void MainWindow::toggleSidebar(bool visible) { m_SidebarContainer->setVisible(visible); }
void MainWindow::updateStatusBar(const QString &msg) { statusBar()->showMessage(msg); }

void MainWindow::handleActionApk() {
    QString path = QFileDialog::getOpenFileName(this, tr("Select APK"), "", "Android Files (*.apk *.xapk *.apks *.apkm);;All Files (*)");
    if (path.isEmpty()) return;

    QFileInfo info(path);
    QString ext = info.suffix().toLower();

    if (ext == "xapk" || ext == "apks" || ext == "apkm") {
        handleSplitApkOpen(path);
    } else {
        openApkFile(path);
    }
}

void MainWindow::handleSplitApkOpen(const QString &bundlePath) {
    QFileInfo info(bundlePath);
    QString outputPath = info.absolutePath() + "/" + info.baseName() + "_merged.apk";
    
    m_GlobalProgress = new QProgressDialog(tr("Fusing Split APK (AntiSplit)..."), tr("Cancel"), 0, 100, this);
    m_GlobalProgress->setWindowModality(Qt::WindowModal);
    
    auto thread = new QThread();
    auto worker = new AntiSplitWorker(QStringList() << bundlePath, outputPath, false);
    worker->moveToThread(thread);
    
    connect(thread, &QThread::started, worker, &AntiSplitWorker::merge);
    connect(worker, &AntiSplitWorker::mergeProgress, this, &MainWindow::handleDecompileProgress);
    connect(worker, &AntiSplitWorker::mergeFinished, this, [this, outputPath](const QString &mergedFile) {
        if (m_GlobalProgress) { m_GlobalProgress->close(); m_GlobalProgress->deleteLater(); m_GlobalProgress = nullptr; }
        openApkFile(mergedFile);
    });
    
    thread->start();
    m_GlobalProgress->exec();
}

void MainWindow::openApkFile(const QString &apkPath) {
    auto dialog = new ApkDecompileDialog(apkPath, this);
    if (dialog->exec() == QDialog::Accepted) {
        m_GlobalProgress = new QProgressDialog(tr("Decompiling..."), tr("Cancel"), 0, 100, this);
        auto thread = new QThread();
        auto worker = new ApkDecompileWorker(dialog->apk(), dialog->folder(), true, true, true, "", "");
        worker->moveToThread(thread);
        connect(worker, &ApkDecompileWorker::decompileFinished, this, &MainWindow::handleDecompileFinished);
        connect(worker, &ApkDecompileWorker::decompileProgress, this, &MainWindow::handleDecompileProgress);
        connect(thread, &QThread::started, worker, &ApkDecompileWorker::decompile);
        thread->start();
        m_GlobalProgress->exec();
    }
}

void MainWindow::handleDecompileFinished(const QString &apk, const QString &folder) {
    if (m_GlobalProgress) m_GlobalProgress->close();
    analyzeProjectContext(folder);
}

void MainWindow::handleDecompileFailed(const QString &apk) {
    if (m_GlobalProgress) m_GlobalProgress->close();
    updateStatusBar(tr("Decompilation failed for: ") + apk);
}

void MainWindow::handleDecompileProgress(int percent, const QString &message) {
    if (m_GlobalProgress) {
        m_GlobalProgress->setValue(percent);
        m_GlobalProgress->setLabelText(message);
    }
}

void MainWindow::openFile(const QString &path) {
    if (m_CentralStack->currentIndex() == 0) m_CentralStack->setCurrentIndex(1);
    for (int i=0; i<m_TabEditors->count(); ++i) {
        if (m_TabEditors->tabToolTip(i) == path) { m_TabEditors->setCurrentIndex(i); return; }
    }
    auto e = new AdvancedCodeEditor();
    e->open(path);
    int idx = m_TabEditors->addTab(e, m_IconProvider.icon(QFileInfo(path)), QFileInfo(path).fileName());
    m_TabEditors->setTabToolTip(idx, path);
    m_TabEditors->setCurrentIndex(idx);
}

void MainWindow::reloadChildren(QTreeWidgetItem *item) {
    QDir dir(item->data(0, Qt::UserRole).toString());
    for (auto info : dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::DirsFirst)) {
        auto child = new QTreeWidgetItem(item);
        child->setText(0, info.fileName());
        child->setData(0, Qt::UserRole, info.absoluteFilePath());
        child->setIcon(0, m_IconProvider.icon(info));
        if (info.isDir()) reloadChildren(child);
    }
}

void MainWindow::handleTabCloseRequested(int index) {
    m_TabEditors->removeTab(index);
    if (m_TabEditors->count() == 0) m_CentralStack->setCurrentIndex(0);
}

void MainWindow::setupModernStyles() {
    setStyleSheet("QMainWindow { background-color: #1e1e1e; } QStatusBar { background-color: #007acc; color: white; }");
}

void MainWindow::setupStatusBarCustom(const QMap<QString, QString>&) {
    m_StatusEngineInfo = new QLabel("Ready");
    statusBar()->addPermanentWidget(m_StatusEngineInfo);
}

void MainWindow::handleActionFolder() {
    QString path = QFileDialog::getExistingDirectory(this, tr("Open Folder"));
    if (!path.isEmpty()) analyzeProjectContext(path);
}

void MainWindow::handleActionSave() {
    auto e = dynamic_cast<AdvancedCodeEditor*>(m_TabEditors->currentWidget());
    if (e) e->save();
}

void MainWindow::handleActionSaveAll() {
    for (int i=0; i<m_TabEditors->count(); ++i) {
        auto e = dynamic_cast<AdvancedCodeEditor*>(m_TabEditors->widget(i));
        if (e) e->save();
    }
}

void MainWindow::handleActionSettings() { (new SettingsDialog())->exec(); }
void MainWindow::handleActionQuit() { qApp->quit(); }
void MainWindow::handleToolAIGameMod() { (new AIGameModDialog(m_CurrentProjectPath, this))->show(); }
void MainWindow::handleSecurityAnalysis() {}
void MainWindow::handleApkCloning() {}
void MainWindow::handleAppModification() {}
void MainWindow::handleAIAnalysisComplete(const QString&) {}
void MainWindow::handleActionFile() {}
void MainWindow::handleActionClose() {}
void MainWindow::handleActionCloseAll() {}
void MainWindow::handleTabChanged(int) {}
MainWindow::~MainWindow() {}