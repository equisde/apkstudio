#ifndef GAMEMODTOOLS_H
#define GAMEMODTOOLS_H

#include <QWidget>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>
#include <QTableWidget>
#include <QTextBrowser>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QNetworkAccessManager>
#include <functional>

class GameEngineDetector
{
public:
    enum Engine { Unknown, Unity, UnrealEngine, Cocos2dx, Flutter, ReactNative, Godot, NativeAndroid };
    static Engine detectEngine(const QString &projectPath);
    static QString engineName(Engine engine);
};

class GameModStudio : public QWidget
{
    Q_OBJECT
public:
    explicit GameModStudio(const QString &projectPath, QWidget *parent = nullptr);

private slots:
    void downloadTools();
    void runDumper();
    void analyzeWithAI();
    void applyMod();

private:
    void setupUI();
    void detectEngine();
    void updateEngineUI();
    void logMessage(const QString &message, const QString &type = "info");
    void askAI(const QString &prompt, std::function<void(const QString&)> callback);

    QString m_ProjectPath;
    GameEngineDetector::Engine m_DetectedEngine;
    QNetworkAccessManager *m_NetworkManager;

    QLabel *m_EngineBadge;
    QLabel *m_StatusLabel;
    QProgressBar *m_Progress;
    QComboBox *m_ModTypeCombo;
    QTreeWidget *m_ModOptionsTree;
    QTextBrowser *m_AIResponseView;
    QPlainTextEdit *m_LogView;
    QPushButton *m_DownloadToolsBtn;
    QPushButton *m_RunDumperBtn;
    QPushButton *m_AnalyzeBtn;
    QPushButton *m_ApplyBtn;
};

// Retrocompatibilidad
#include <QDialog>
class AIGameModDialog : public QDialog {
    Q_OBJECT
public:
    AIGameModDialog(const QString &p, QWidget *parent = nullptr) : QDialog(parent) {
        auto layout = new QVBoxLayout(this);
        layout->addWidget(new GameModStudio(p, this));
        setWindowTitle("Game Mod Studio");
        resize(1000, 700);
    }
};

class GameModToolDownloader {
public:
    struct Tool { QString name; QString downloadUrl; };
    static QString getToolsDirectory();
    static QString getToolExecutable(const QString &toolName);
};

#endif // GAMEMODTOOLS_H
