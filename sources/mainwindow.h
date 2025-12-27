#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QToolBar>
#include <QDockWidget>
#include <QTreeWidget>
#include <QListView>
#include <QLabel>
#include <QProgressBar>
#include <QFileIconProvider>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QTabWidget>
#include <QMenu>
#include <QAction>
#include <QProgressDialog>
#include "aiconsolewidget.h"
#include "gamemodtools.h"
#include "securityhub.h"
#include "cloningstudio.h"
#include "appmodstudio.h"
#include "il2cppstudio.h"
#include "nativesstudio.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum Section {
        Explorer = 0,
        Search,
        GameModding,
        Security,
        Cloning,
        AppMod,
        NativeStudioSec,
        AIStudio
    };

    explicit MainWindow(const QMap<QString, QString> &versions, QWidget *parent = nullptr);
    ~MainWindow();

    void openApkFile(const QString &apkPath);

private slots:
    // UI Navigation
    void switchSection(Section section);
    void toggleSidebar(bool visible);
    
    // File Operations
    void handleActionApk();
    void handleSplitApkOpen(const QString &bundlePath);
    void handleActionFolder();
    void handleActionFile();
    void handleActionSave();
    void handleActionSaveAll();
    void handleActionClose();
    void handleActionCloseAll();
    void handleActionSettings();
    void handleActionQuit();

    // Context & Analysis
    void analyzeProjectContext(const QString &path);
    void handleAIAnalysisComplete(const QString &analysisPath);
    
    // Section Handlers
    void handleToolAIGameMod();
    void handleSecurityAnalysis();
    void handleApkCloning();
    void handleAppModification();
    
    // Decompile Handlers
    void handleDecompileFinished(const QString &apk, const QString &folder);
    void handleDecompileFailed(const QString &apk);
    void handleDecompileProgress(int percent, const QString &message);

    // Tab Management
    void handleTabChanged(int index);
    void handleTabCloseRequested(int index);
    void openFile(const QString &path);

private:
    void updateStatusBar(const QString &msg);
    void setupActivityBar();
    void setupSidebars();
    void setupModernStyles();
    void setupMenuBar();
    void setupStatusBarCustom(const QMap<QString, QString> &versions);
    void reloadChildren(QTreeWidgetItem *item);
    QString getCurrentProjectPath();

    // VS Code Layout Elements
    QToolBar *m_ActivityBar;
    QStackedWidget *m_SidebarStack;
    QTabWidget *m_TabEditors;
    QStackedWidget *m_CentralStack;
    QWidget *m_SidebarContainer;
    
    // Actions
    QActionGroup *m_ActivityGroup;
    QAction *m_ActionSave;
    QAction *m_ActionSaveAll;
    QAction *m_ActionClose;
    
    // Section Widgets
    QTreeWidget *m_ExplorerTree;
    AIConsoleWidget *m_AIStudioWidget;
    SecurityHub *m_SecurityHub;
    CloningStudio *m_CloningStudio;
    AppModStudio *m_AppModStudio;
    Il2CppStudio *m_Il2CppStudio;
    NativeStudio *m_NativeStudio;
    
    // Status
    QLabel *m_StatusProjectInfo;
    QLabel *m_StatusEngineInfo;
    QProgressDialog *m_GlobalProgress;

    QString m_CurrentProjectPath;
    QString m_DetectedContext;
    QFileIconProvider m_IconProvider;
    QStandardItemModel *m_ModelOpenFiles;
};

#endif // MAINWINDOW_H
