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
#include "aiconsolewidget.h"
#include "gamemodtools.h"

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
        AIStudio
    };

    explicit MainWindow(const QMap<QString, QString> &versions, QWidget *parent = nullptr);
    ~MainWindow();
    void openApkFile(const QString &apkPath);

private slots:
    void switchSection(Section section);
    void analyzeProjectContext(const QString &path);
    void handleToolAIGameMod();
    void handleSecurityAnalysis();
    void handleApkCloning();
    void handleAppModification();
    void updateStatusBar(const QString &msg);
    void openProject(const QString &folder);
    void handleDecompileFinished(const QString &apk, const QString &folder);
    void handleDecompileFailed(const QString &apk);
    void handleDecompileProgress(int percent, const QString &message);

private:
    QString getCurrentProjectPath();
    void setupActivityBar();
    void setupSidebars();
    void setupModernStyles();
    
    // UI Elements (VS Code Layout)
    QToolBar *m_ActivityBar;
    QStackedWidget *m_SidebarStack;
    QStackedWidget *m_CentralStack;
    
    // Section Widgets
    QTreeWidget *m_ExplorerTree;
    AIConsoleWidget *m_AIStudioWidget;
    AIGameModDialog *m_GameModWidget;
    
    // Status Elements
    QLabel *m_StatusProjectInfo;
    QLabel *m_StatusEngineInfo;
    QProgressDialog *m_GlobalProgress;

    QString m_CurrentProjectPath;
    QString m_DetectedContext; // Flutter, Unity, Kotlin, etc.
    
    QFileIconProvider m_IconProvider;
};

#endif // MAINWINDOW_H