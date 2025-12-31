#ifndef GAMEMODTOOLS_H
#define GAMEMODTOOLS_H

#include <QWidget>
#include <QDialog>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>
#include <QTableWidget>
#include <QTextBrowser>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QNetworkAccessManager>
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

private:
    void setupUI();
    void detectEngine();
    void updateEngineUI();
    void logMessage(const QString &message, const QString &type = "info");
    void askAI(const QString &prompt, std::function<void(const QString&)> callback);
    bool checkUnityDecompilation();
    QString collectGameContext();

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

#endif // GAMEMODTOOLS_H
