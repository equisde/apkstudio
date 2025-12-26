#ifndef GAMEMODTOOLS_H
#define GAMEMODTOOLS_H

#include <functional>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextBrowser>
#include <QTreeWidget>

// Game Engine Auto-Detector
class GameEngineDetector
{
public:
    enum Engine {
        Unknown,
        Unity,
        UnrealEngine,
        Cocos2dx,
        Flutter,
        ReactNative,
        Godot,
        LibGDX,
        Cordova,
        Xamarin,
        NativeAndroid
    };
    
    static Engine detectEngine(const QString &projectPath);
    static QString engineName(Engine engine);
    static QStringList getEngineFiles(Engine engine);
    static bool supportsDecompilation(Engine engine);
};

// Tool Downloader
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
    
    static QList<Tool> getRequiredTools(GameEngineDetector::Engine engine);
    static bool isToolInstalled(const QString &toolPath);
    static QString getToolsDirectory();
    static QString getToolExecutable(const QString &toolName);
    static void downloadTool(const Tool &tool, QWidget *parent, std::function<void(bool, const QString&)> callback = nullptr);
    static void downloadAllTools(GameEngineDetector::Engine engine, QWidget *parent, 
                                  std::function<void(int, int)> progressCallback = nullptr,
                                  std::function<void(bool)> completionCallback = nullptr);
};

// AI-Powered Game Mod Dialog
class AIGameModDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AIGameModDialog(const QString &projectPath, QWidget *parent = nullptr);
    
private slots:
    void detectEngine();
    void downloadTools();
    void runDumper();
    void analyzeWithAI();
    void applyMod();
    void generatePatch();
    void searchValues();
    void bypassSSL();
    void bypassAntiCheat();
    void bypassIAP();
    void extractAssets();
    void saveModProfile();
    
private:
    void setupUI();
    void askAI(const QString &prompt, std::function<void(const QString&)> callback);
    void applyPatch(const QString &file, const QByteArray &find, const QByteArray &replace);
    void logMessage(const QString &message, const QString &type = "info");
    void updateEngineUI();
    
    QString m_ProjectPath;
    GameEngineDetector::Engine m_DetectedEngine;
    QNetworkAccessManager *m_NetworkManager;
    
    // UI Elements
    QLabel *m_EngineBadge;
    QLabel *m_StatusLabel;
    QProgressBar *m_Progress;
    QTreeWidget *m_ModOptionsTree;
    QTableWidget *m_ValuesTable;
    QPlainTextEdit *m_LogView;
    QTextBrowser *m_AIResponseView;
    
    QPushButton *m_DownloadToolsBtn;
    QPushButton *m_RunDumperBtn;
    QPushButton *m_AnalyzeBtn;
    QPushButton *m_SearchBtn;
    QPushButton *m_ApplyBtn;
    
    QLineEdit *m_SearchInput;
    QComboBox *m_ModTypeCombo;
};

// Legacy dialogs kept for compatibility if needed, but AIGameModDialog is preferred
class UnityGameDialog : public QDialog { Q_OBJECT public: explicit UnityGameDialog(const QString &p, QWidget *par = nullptr); };
class FlutterAnalyzerDialog : public QDialog { Q_OBJECT public: explicit FlutterAnalyzerDialog(const QString &p, QWidget *par = nullptr); };
class GameValueEditorDialog : public QDialog { Q_OBJECT public: explicit GameValueEditorDialog(const QString &p, QWidget *par = nullptr); };

#endif // GAMEMODTOOLS_H