#ifndef GAMEMODTOOLS_H
#define GAMEMODTOOLS_H

#include <QWidget>
#include <QDialog>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>
#include <QTableWidget>
#include <QListWidget>
#include <QTextBrowser>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QNetworkAccessManager>
#include <QProcess>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QGroupBox>
#include <QScrollArea>
#include <functional>

class GameEngineDetector
{
public:
    enum Engine { Unknown, Unity, UnrealEngine, Cocos2dx, Flutter, ReactNative, Godot, NativeAndroid };
    static Engine detectEngine(const QString &projectPath);
    static QString engineName(Engine engine);
};

// Structure for mod configuration options
struct ModOption {
    QString id;
    QString name;
    QString description;
    bool enabled = false;
    int value = 0;
    int minValue = 0;
    int maxValue = 999999999;
    QString customValue;
    QString category;
    QString modType; // "toggle", "value", "multiplier", "custom"
};

// Interactive Mod Menu Builder Dialog
class InteractiveModMenuDialog : public QDialog
{
    Q_OBJECT
public:
    explicit InteractiveModMenuDialog(const QString &projectPath, QWidget *parent = nullptr);

private slots:
    void onCategorySelected(int index);
    void addCustomMod();
    void removeSelectedMod();
    void generateWithAI();
    void previewCode();
    void saveAndApply();

private:
    void setupUI();
    void askAI(const QString &prompt, std::function<void(const QString&)> callback);
    void populateModsList();
    QString buildModPrompt();
    
    QString m_ProjectPath;
    QString m_GameContext;
    QComboBox *m_CategoryCombo;
    QListWidget *m_AvailableModsList;
    QListWidget *m_SelectedModsList;
    QComboBox *m_MenuStyleCombo;
    QTextBrowser *m_PreviewArea;
    QTextBrowser *m_ChatArea;
    QLineEdit *m_CustomModInput;
    QLineEdit *m_AIChatInput;
    QString m_GeneratedCode;
    QNetworkAccessManager *m_NetworkManager;
    QList<ModOption> m_SelectedMods;
};

// Widget for configuring individual mod options
class ModConfigWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ModConfigWidget(QWidget *parent = nullptr);
    QList<ModOption> getSelectedMods() const;
    QString generateModDescription() const;
    
signals:
    void configChanged();

private:
    void setupUI();
    void addModCategory(QVBoxLayout *layout, const QString &category, const QList<ModOption> &options);
    
    QMap<QString, QCheckBox*> m_Checkboxes;
    QMap<QString, QSpinBox*> m_ValueSpins;
    QMap<QString, QLineEdit*> m_CustomInputs;
    QList<ModOption> m_AllOptions;
};

class GameModStudio : public QWidget
{
    Q_OBJECT
public:
    explicit GameModStudio(const QString &projectPath, QWidget *parent = nullptr);
    void setProjectPath(const QString &path);

private slots:
    void downloadTools();
    void runDumper();
    void analyzeWithAI();
    void applyMod();
    void generateModMenu();
    void openAIChat();
    void verifyDecompilation();
    void openInteractiveModBuilder();

private:
    void setupUI();
    void detectEngine();
    void updateEngineUI();
    void logMessage(const QString &message, const QString &type = "info");
    void askAI(const QString &prompt, std::function<void(const QString&)> callback);
    bool checkUnityDecompilation();
    QString collectGameContext();
    QString categorizeClass(const QString &className, const QString &content);
    int calculateClassPriority(const QString &className, const QString &content, 
                               const QStringList &high, const QStringList &medium, const QStringList &low);
    bool runILSpyDecompilation();
    bool runILSpyOnDummyDlls();
    bool runIl2CppDumper();
    void checkToolsAndSuggestDownload();

    QString m_ProjectPath;
    GameEngineDetector::Engine m_DetectedEngine;
    QNetworkAccessManager *m_NetworkManager;

    QLabel *m_EngineBadge;
    QLabel *m_StatusLabel;
    QProgressBar *m_Progress;
    ModConfigWidget *m_ModConfig;
    QTextBrowser *m_AIResponseView;
    QPlainTextEdit *m_LogView;
    QLineEdit *m_AIChatInput;
    QPushButton *m_DownloadToolsBtn;
    QPushButton *m_RunDumperBtn;
    QPushButton *m_AnalyzeBtn;
    QPushButton *m_ApplyBtn;
    QPushButton *m_ModMenuBtn;
    QPushButton *m_VerifyBtn;
    QPushButton *m_InteractiveBtn;
};

// Interactive AI Chat Dialog for custom mod requests
class AIModChatDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AIModChatDialog(const QString &projectPath, QWidget *parent = nullptr);

private slots:
    void sendMessage();
    void clearChat();

private:
    void askAI(const QString &prompt, std::function<void(const QString&)> callback);
    
    QString m_ProjectPath;
    QString m_Context;
    QTextBrowser *m_ChatHistory;
    QLineEdit *m_Input;
    QNetworkAccessManager *m_NetworkManager;
};

// Mod Menu Generator Dialog
class ModMenuGeneratorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ModMenuGeneratorDialog(const QString &projectPath, const QList<ModOption> &mods, QWidget *parent = nullptr);

private slots:
    void generateMenu();
    void saveCode();

private:
    void askAI(const QString &prompt, std::function<void(const QString&)> callback);
    
    QString m_ProjectPath;
    QList<ModOption> m_Mods;
    QComboBox *m_StyleCombo;
    QTextBrowser *m_CodeView;
    QString m_GeneratedCode;
    QNetworkAccessManager *m_NetworkManager;
};

// Retrocompatibilidad
class AIGameModDialog : public QDialog {
    Q_OBJECT
public:
    AIGameModDialog(const QString &p, QWidget *parent = nullptr) : QDialog(parent) {
        auto layout = new QVBoxLayout(this);
        layout->addWidget(new GameModStudio(p, this));
        setWindowTitle("Game Mod Studio");
        resize(1200, 800);
    }
};

class UnityGameDialog : public QDialog { Q_OBJECT public: explicit UnityGameDialog(const QString &p, QWidget *par = nullptr); };
class FlutterAnalyzerDialog : public QDialog { Q_OBJECT public: explicit FlutterAnalyzerDialog(const QString &p, QWidget *par = nullptr); };
class GameValueEditorDialog : public QDialog { Q_OBJECT public: explicit GameValueEditorDialog(const QString &p, QWidget *par = nullptr); };

class GameModToolDownloader {
public:
    struct Tool { QString name; QString downloadUrl; };
    static QString getToolsDirectory();
    static QString getToolExecutable(const QString &toolName);
};

// =============================================================================
// Mod Menu Code Generator - Generates complete C++ mod menu projects locally
// =============================================================================

struct GameClassInfo {
    QString className;
    QString nameSpace;
    QStringList fields;      // "fieldName|type|offset"
    QStringList methods;     // "methodName|returnType|params|rva"
};

struct ModTarget {
    QString modId;
    QString displayName;
    QString targetClass;
    QString targetField;
    QString targetMethod;
    QString fieldType;
    QString offset;
    QString rva;
    int value;
    QString hookType;        // "field_write", "method_replace", "method_return"
};

class ModMenuCodeGenerator : public QObject
{
    Q_OBJECT
public:
    explicit ModMenuCodeGenerator(const QString &projectPath, QObject *parent = nullptr);
    
    // Parse dump.cs and extract game class info
    bool parseDumpCs();
    
    // Find targets for selected mods
    QList<ModTarget> findModTargets(const QList<ModOption> &mods);
    
    // Generate complete mod menu project
    bool generateProject(const QString &outputDir, const QList<ModTarget> &targets, const QString &style);
    
    // Get extracted class info
    QList<GameClassInfo> getGameClasses() const { return m_GameClasses; }
    
signals:
    void progressUpdated(int percent, const QString &status);
    void generationComplete(bool success, const QString &outputPath);
    void logMessage(const QString &message, const QString &type);
    
private:
    // Template generators
    QString generateMainCpp(const QList<ModTarget> &targets);
    QString generateGameDefsHpp(const QList<ModTarget> &targets);
    QString generateModMenuHpp(const QList<ModTarget> &targets);
    QString generateIl2cppUtilsHpp();
    QString generateIl2cppUtilsCpp();
    QString generateAndroidMk();
    QString generateApplicationMk();
    QString generateBuildSh();
    QString generateBuildBat();
    QString generateSmaliLoader();
    QString generateFridaScript(const QList<ModTarget> &targets);
    QString generateReadme(const QList<ModTarget> &targets);
    
    // Helper methods
    QString modIdToVarName(const QString &modId);
    QString modIdToFunctionName(const QString &modId);
    QString generateHookCode(const ModTarget &target);
    QString generateMenuToggle(const ModTarget &target);
    
    QString m_ProjectPath;
    QString m_DumpContent;
    QList<GameClassInfo> m_GameClasses;
    QMap<QString, QString> m_FieldPatterns;  // modId -> regex pattern for field search
    QMap<QString, QString> m_MethodPatterns; // modId -> regex pattern for method search
};

// Full project generator dialog
class ModMenuProjectDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ModMenuProjectDialog(const QString &projectPath, const QList<ModOption> &mods, QWidget *parent = nullptr);

private slots:
    void onGenerateClicked();
    void onPreviewFile(int index);
    void onSaveProject();
    void onOpenFolder();
    void onDownloadDependencies();
    void onDownloadFinished(QNetworkReply *reply);
    void onDownloadNDK();
    void onBuildProject();
    void onBuildDobby();
    void onBuildProcessOutput();
    void onBuildProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
    void setupUI();
    void updatePreview(const QString &fileName, const QString &content);
    void downloadFile(const QString &url, const QString &destPath, const QString &description);
    void extractZip(const QString &zipPath, const QString &destDir);
    bool checkDependencies();
    bool checkNDK();
    QString findNDKPath();
    bool checkDobbyLibrary();
    void buildDobbyFromSource();
    void runNdkBuild();
    
    QString m_ProjectPath;
    QList<ModOption> m_Mods;
    ModMenuCodeGenerator *m_Generator;
    QList<ModTarget> m_Targets;
    
    QComboBox *m_StyleCombo;
    QListWidget *m_FilesList;
    QTextBrowser *m_PreviewArea;
    QProgressBar *m_Progress;
    QLabel *m_StatusLabel;
    QLabel *m_NDKStatusLabel;
    QPushButton *m_GenerateBtn;
    QPushButton *m_SaveBtn;
    QPushButton *m_OpenFolderBtn;
    QPushButton *m_DownloadDepsBtn;
    QPushButton *m_DownloadNDKBtn;
    QPushButton *m_BuildBtn;
    QCheckBox *m_AutoDownloadCheck;
    
    QMap<QString, QString> m_GeneratedFiles;
    QString m_OutputDir;
    QString m_NDKPath;
    QNetworkAccessManager *m_NetworkManager;
    QMap<QNetworkReply*, QPair<QString, QString>> m_PendingDownloads; // reply -> (destPath, description)
    int m_DownloadsRemaining;
    QProcess *m_BuildProcess;
};

#endif // GAMEMODTOOLS_H
