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

MainWindow::MainWindow(const QMap<QString, QString> &versions, QWidget *parent)
    : QMainWindow(parent)
{
    setupModernStyles();
    
    // Contenedor principal estilo VS Code
    auto centralWidget = new QWidget(this);
    auto mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    setCentralWidget(centralWidget);

    // 1. Activity Bar (Extremo Izquierdo)
    setupActivityBar();
    mainLayout->addWidget(m_ActivityBar);

    // 2. Sidebar + Central Area Splitter
    auto mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainSplitter->setHandleWidth(1);
    mainSplitter->setStyleSheet("QSplitter::handle { background-color: #3c3c3c; }");

    // Sidebar Container
    m_SidebarContainer = new QWidget();
    auto sideLayout = new QVBoxLayout(m_SidebarContainer);
    sideLayout->setContentsMargins(0, 0, 0, 0);
    sideLayout->setSpacing(0);
    
    m_SidebarStack = new QStackedWidget();
    setupSidebars();
    sideLayout->addWidget(m_SidebarStack);
    
    m_SidebarContainer->setMinimumWidth(260);
    m_SidebarContainer->setMaximumWidth(500);
    mainSplitter->addWidget(m_SidebarContainer);

    // Editor Area (Central)
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
    
    // Cargar último proyecto si existe
    QTimer::singleShot(100, this, [this]() {
        QSettings settings;
        QString last = settings.value("open_project").toString();
        if (!last.isEmpty() && QDir(last).exists()) analyzeProjectContext(last);
    });
}

void MainWindow::setupMenuBar()
{
    auto menu = menuBar();
    menu->setStyleSheet("QMenuBar { background-color: #3c3c3c; color: #cccccc; } QMenuBar::item:selected { background-color: #505050; }");

    // FILE
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

    // EDIT (Básico por ahora)
    auto editMenu = menu->addMenu(tr("&Edit"));
    editMenu->addAction(tr("Undo"), QKeySequence::Undo, this, [](){});
    editMenu->addAction(tr("Redo"), QKeySequence::Redo, this, [](){});

    // VIEW
    auto viewMenu = menu->addMenu(tr("&View"));
    auto sideAct = viewMenu->addAction(tr("Show Sidebar"));
    sideAct->setCheckable(true);
    sideAct->setChecked(true);
    connect(sideAct, &QAction::toggled, this, &MainWindow::toggleSidebar);

    // TOOLS (Categorizado como pediste)
    auto toolsMenu = menu->addMenu(tr("&Tools"));
    
    auto gameMod = toolsMenu->addMenu(tr("🎮 Game Modding"));
    gameMod->addAction(tr("AI Mod Studio"), QKeySequence("Ctrl+Shift+G"), this, &MainWindow::handleToolAIGameMod);
    
    auto secHub = toolsMenu->addMenu(tr("🛡️ Security Hub"));
    secHub->addAction(tr("Analyze Network"), this, &MainWindow::handleSecurityAnalysis);
    secHub->addAction(tr("Bypass SSL Pinning"), this, &MainWindow::handleSecurityAnalysis);
    
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
    addAct(GameModding, ":/icons/icons8/icons8-hammer-48.png", "Game Modding");
    addAct(Security, ":/icons/icons8/icons8-software-installer-48.png", "Security");
    addAct(AIStudio, ":/icons/icons8/icons8-gear-48.png", "AI Studio");

    connect(m_ActivityGroup, &QActionGroup::triggered, this, [this](QAction *a) {
        if (!m_SidebarContainer->isVisible()) m_SidebarContainer->show();
        switchSection(static_cast<Section>(a->data().toInt()));
    });
}

void MainWindow::setupSidebars()
{
    // 0. Explorador de Archivos Profesional
    m_ExplorerTree = new QTreeWidget();
    m_ExplorerTree->setHeaderLabel(tr("PROJECT EXPLORER"));
    m_ExplorerTree->setAnimated(true);
    m_ExplorerTree->setIndentation(15);
    connect(m_ExplorerTree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item) {
        QString path = item->data(0, Qt::UserRole).toString();
        if (!path.isEmpty() && QFileInfo(path).isFile()) openFile(path);
    });
    m_SidebarStack->addWidget(m_ExplorerTree);

    // 1. Placeholder Search
    m_SidebarStack->addWidget(new QLabel("Search Panel (Work in Progress)"));

    // 2. Game Mod Sidebar
    auto gmWidget = new QWidget();
    auto gmLayout = new QVBoxLayout(gmWidget);
    gmLayout->addWidget(new QLabel("<b>GAME ENGINE MODS</b>"));
    auto btnDump = new QPushButton("🚀 Run Engine Dumper (IA)");
    connect(btnDump, &QPushButton::clicked, this, &MainWindow::handleToolAIGameMod);
    gmLayout->addWidget(btnDump);
    gmLayout->addStretch();
    m_SidebarStack->addWidget(gmWidget);

    // 3. Security Hub Sidebar
    auto secWidget = new QWidget();
    auto secLayout = new QVBoxLayout(secWidget);
    secLayout->addWidget(new QLabel("<b>SECURITY & BYPASS</b>"));
    secLayout->addWidget(new QPushButton("🛡️ SSL Unpinning Assist"));
    secLayout->addWidget(new QPushButton("🔒 Anti-Tamper Patch"));
    secLayout->addStretch();
    m_SidebarStack->addWidget(secWidget);
}

void MainWindow::switchSection(Section section) {
    m_SidebarStack->setCurrentIndex(static_cast<int>(section));
}

void MainWindow::toggleSidebar(bool visible) {
    m_SidebarContainer->setVisible(visible);
}

void MainWindow::handleActionApk() {
    QString path = QFileDialog::getOpenFileName(this, tr("Select APK"), "", "APKs (*.apk)");
    if (!path.isEmpty()) openApkFile(path);
}

void MainWindow::openApkFile(const QString &apkPath) {
    auto dialog = new ApkDecompileDialog(apkPath, this);
    if (dialog->exec() == QDialog::Accepted) {
        m_GlobalProgress = new QProgressDialog(tr("Extracting APK..."), tr("Cancel"), 0, 100, this);
        m_GlobalProgress->setWindowModality(Qt::WindowModal);
        
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

void MainWindow::analyzeProjectContext(const QString &path) {
    m_CurrentProjectPath = path;
    m_ExplorerTree->clear();
    auto root = new QTreeWidgetItem(m_ExplorerTree);
    root->setText(0, QFileInfo(path).fileName());
    root->setData(0, Qt::UserRole, path);
    root->setIcon(0, m_IconProvider.icon(QFileInfo(path)));
    reloadChildren(root);
    root->setExpanded(true);
    
    // IA contextual logic
    GameEngineDetector::Engine engine = GameEngineDetector::detectEngine(path);
    m_DetectedContext = GameEngineDetector::engineName(engine);
    m_StatusEngineInfo->setText("Target: " + m_DetectedContext);
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

void MainWindow::openFile(const QString &path) {
    if (m_CentralStack->currentIndex() == 0) m_CentralStack->setCurrentIndex(1);
    
    // Buscar si ya está abierta
    for (int i=0; i<m_TabEditors->count(); ++i) {
        if (m_TabEditors->tabToolTip(i) == path) {
            m_TabEditors->setCurrentIndex(i);
            return;
        }
    }

    QWidget *editor = nullptr;
    QFileInfo info(path);
    QString ext = info.suffix().toLower();

    if (ext == "png" || ext == "jpg") {
        auto v = new ImageViewerWidget();
        v->open(path);
        editor = v;
    } else {
        auto e = new AdvancedCodeEditor();
        e->open(path);
        editor = e;
    }

    int idx = m_TabEditors->addTab(editor, m_IconProvider.icon(info), info.fileName());
    m_TabEditors->setTabToolTip(idx, path);
    m_TabEditors->setCurrentIndex(idx);
}

void MainWindow::handleTabCloseRequested(int index) {
    m_TabEditors->removeTab(index);
    if (m_TabEditors->count() == 0) m_CentralStack->setCurrentIndex(0);
}

void MainWindow::setupModernStyles() {
    setStyleSheet(R"(
        QMainWindow { background-color: #1e1e1e; }
        QToolBar#ActivityBar { background-color: #333333; border: none; }
        QStatusBar { background-color: #007acc; color: white; }
        QTabWidget::pane { border-top: 1px solid #3c3c3c; }
        QTabBar::tab { background: #2d2d2d; color: #969696; padding: 8px 15px; border-right: 1px solid #1e1e1e; }
        QTabBar::tab:selected { background: #1e1e1e; color: white; }
    )");
}

void MainWindow::setupStatusBarCustom(const QMap<QString, QString> &versions) {
    auto sb = statusBar();
    m_StatusEngineInfo = new QLabel("Ready");
    sb->addPermanentWidget(m_StatusEngineInfo);
}

void MainWindow::handleActionFolder() {
    QString path = QFileDialog::getExistingDirectory(this, tr("Open Project Folder"));
    if (!path.isEmpty()) analyzeProjectContext(path);
}

void MainWindow::handleActionSave() {
    auto editor = dynamic_cast<AdvancedCodeEditor*>(m_TabEditors->currentWidget());
    if (editor) editor->save();
}

void MainWindow::handleActionSaveAll() {
    for (int i=0; i<m_TabEditors->count(); ++i) {
        auto editor = dynamic_cast<AdvancedCodeEditor*>(m_TabEditors->widget(i));
        if (editor) editor->save();
    }
}

void MainWindow::handleActionSettings() { (new SettingsDialog(0, this))->exec(); }
void MainWindow::handleActionQuit() { qApp->quit(); }
void MainWindow::handleActionClose() { handleTabCloseRequested(m_TabEditors->currentIndex()); }
void MainWindow::handleActionCloseAll() { while(m_TabEditors->count() > 0) handleTabCloseRequested(0); }
void MainWindow::handleTabChanged(int) {}
void MainWindow::handleToolAIGameMod() { (new AIGameModDialog(m_CurrentProjectPath, this))->show(); }
void MainWindow::handleSecurityAnalysis() {}
void MainWindow::handleApkCloning() {}
void MainWindow::handleAppModification() {}
void MainWindow::handleAIAnalysisComplete(const QString&) {}
void MainWindow::handleActionFile() {}
MainWindow::~MainWindow() {}
