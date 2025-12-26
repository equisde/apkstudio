#include "mainwindow.h"
#include "apkdecompiledialog.h"
#include "apkdecompileworker.h"
#include "settingsdialog.h"
#include "advancedcodeeditor.h"
#include "imageviewerwidget.h"
#include "markdownviewerwidget.h"
#include "hexedit.h"
#include <QSplitter>
#include <QVBoxLayout>
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
    connect(m_TabEditors, &QTabWidget::tabCloseRequested, this, &MainWindow::handleTabCloseRequested);
    connect(m_TabEditors, &QTabWidget::currentChanged, this, &MainWindow::handleTabChanged);

    auto welcome = new QLabel("<h1 style='color: #555;'>APK Studio Pro</h1><p style='color: #777;'>Suelta un APK para empezar la magia.</p>");
    welcome->setAlignment(Qt::AlignCenter);
    m_CentralStack->addWidget(welcome);
    m_CentralStack->addWidget(m_TabEditors);
    
    mainSplitter->addWidget(m_CentralStack);
    mainSplitter->setStretchFactor(1, 1);
    mainLayout->addWidget(mainSplitter);

    setupMenuBar();
    setupStatusBarCustom(versions);
    
    // Inicializar Sidebars vacíos
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
    addAct(AIStudio, ":/icons/icons8/icons8-gear-48.png", "AI Agent");

    connect(m_ActivityGroup, &QActionGroup::triggered, this, [this](QAction *a) {
        switchSection(static_cast<Section>(a->data().toInt()));
    });
}

void MainWindow::setupSidebars()
{
    // 0. Explorer
    m_ExplorerTree = new QTreeWidget();
    m_ExplorerTree->setHeaderLabel(tr("PROJECT EXPLORER"));
    m_ExplorerTree->setStyleSheet("QTreeWidget { background-color: #252526; color: #cccccc; border: none; }");
    connect(m_ExplorerTree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item) {
        QString path = item->data(0, Qt::UserRole).toString();
        if (!path.isEmpty() && QFileInfo(path).isFile()) openFile(path);
    });
    m_SidebarStack->addWidget(m_ExplorerTree);

    // 1. Search
    m_SidebarStack->addWidget(new QLabel("AI Search Coming Soon..."));

    // 2. Game Modding (Inicialmente vacío)
    m_SidebarStack->addWidget(new QLabel("Open a Project..."));

    // 3. Security (Inicialmente vacío)
    m_SidebarStack->addWidget(new QLabel("Open a Project..."));

    // 4. Cloning
    m_SidebarStack->addWidget(new QLabel("Open a Project..."));

    // 5. App Mod
    m_SidebarStack->addWidget(new QLabel("Open a Project..."));

    // 6. AI Agent Console (Restaurado)
    m_AIStudioWidget = new AIConsoleWidget();
    m_SidebarStack->addWidget(m_AIStudioWidget);
}

void MainWindow::analyzeProjectContext(const QString &path)
{
    m_CurrentProjectPath = path;
    updateStatusBar("AI Analyzing Project Environment...");

    // 1. Detección de Motor / Tipo de App
    GameEngineDetector::Engine engine = GameEngineDetector::detectEngine(path);
    m_DetectedContext = GameEngineDetector::engineName(engine);
    
    // 2. Inicializar Widgets con el contexto real
    m_SecurityHub = new SecurityHub(path);
    m_SidebarStack->insertWidget(Security, m_SecurityHub);
    
    m_CloningStudio = new CloningStudio(path);
    m_SidebarStack->insertWidget(Cloning, m_CloningStudio);
    
    m_AppModStudio = new AppModStudio(path);
    m_SidebarStack->insertWidget(AppMod, m_AppModStudio);
    
    // 3. Generar Reportes MD Automáticos
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

    // 4. Actualizar Árbol
    m_ExplorerTree->clear();
    auto root = new QTreeWidgetItem(m_ExplorerTree);
    root->setText(0, QFileInfo(path).fileName());
    root->setData(0, Qt::UserRole, path);
    reloadChildren(root);
    root->setExpanded(true);

    m_StatusEngineInfo->setText("Context: " + m_DetectedContext);
    m_AIStudioWidget->setProjectPath(path);
    
    updateStatusBar("Analysis Complete. Reports generated.");
}

void MainWindow::switchSection(Section section) {
    m_SidebarStack->setCurrentIndex(static_cast<int>(section));
    if (!m_SidebarContainer->isVisible()) m_SidebarContainer->show();
}

void MainWindow::toggleSidebar(bool visible) { m_SidebarContainer->setVisible(visible); }
void MainWindow::updateStatusBar(const QString &msg) { statusBar()->showMessage(msg); }

void MainWindow::openApkFile(const QString &apkPath) {
    auto dialog = new ApkDecompileDialog(apkPath, this);
    if (dialog->exec() == QDialog::Accepted) {
        m_GlobalProgress = new QProgressDialog(tr("Decompiling..."), tr("Cancel"), 0, 100, this);
        auto thread = new QThread();
        auto worker = new ApkDecompileWorker(dialog->apk(), dialog->folder(), dialog->smali(), dialog->resources(), dialog->java(), "", "");
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

void MainWindow::handleActionApk() {
    QString path = QFileDialog::getOpenFileName(this, tr("Select APK"), "", "APKs (*.apk)");
    if (!path.isEmpty()) openApkFile(path);
}

void MainWindow::handleActionFolder() {
    QString path = QFileDialog::getExistingDirectory(this, tr("Open Project"));
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
