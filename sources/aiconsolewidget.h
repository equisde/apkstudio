#ifndef AICONSOLEWIDGET_H
#define AICONSOLEWIDGET_H

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>
#include <QPushButton>
#include <QStackedWidget>
#include <QTextBrowser>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

class AIConsoleWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AIConsoleWidget(QWidget *parent = nullptr);
    void setProjectPath(const QString &path);
    void analyzeProject();
    void appendMessage(const QString &role, const QString &message);
    void appendSystemMessage(const QString &message);
    void clear();
    
signals:
    void analysisComplete(const QString &analysisPath);
    void analysisStarted();
    void fileModified(const QString &filePath);
    void fileCreated(const QString &filePath);
    void requestFileOpen(const QString &filePath);
    
private slots:
    void handleSendMessage();
    void handleApiResponse();
    void handleApiError(QNetworkReply::NetworkError error);
    void handleCliOutput();
    void handleCliError();
    void handleCliFinished(int exitCode, QProcess::ExitStatus status);
    void onModeChanged(int index);
    void checkDependencies();
    void installDependencies();
    void installCliAgent();
    void handleTerminalInput();
    
private:
    // UI elements
    QVBoxLayout *m_MainLayout;
    QWidget *m_HeaderWidget;
    QLabel *m_TitleLabel;
    QStackedWidget *m_StackedWidget;
    QWidget *m_AiAssistantWidget;
    QWidget *m_TerminalWidget;
    QTextBrowser *m_OutputConsole;
    QTextEdit *m_TerminalOutput;
    QLineEdit *m_TerminalInput;
    QLineEdit *m_InputLine;
    QPushButton *m_SendButton;
    QPushButton *m_AnalyzeButton;
    QPushButton *m_ClearButton;
    QComboBox *m_ModeCombo;
    QPushButton *m_DepsButton;
    QLabel *m_TerminalTabLabel;
    QLabel *m_TerminalPromptLabel;
    
    // Network
    QNetworkAccessManager *m_NetworkManager;
    QNetworkReply *m_CurrentReply;
    
    // CLI Agent
    QProcess *m_CliProcess;
    bool m_UseCliAgent;
    
    // State
    QString m_ProjectPath;
    QStringList m_ConversationHistory;
    QString m_CurrentProjectContext;
    QString m_HtmlContent;
    QString m_FullProjectContext;
    bool m_IsProcessingFileOps;
    
    // Environment detection
    bool m_NodeAvailable;
    bool m_NvmAvailable;
    bool m_GeminiCliAvailable;
    bool m_CopilotCliAvailable;
    QString m_NodePath;
    QString m_NodeVersion;
    QString m_NvmPath;
    
    // Existing analysis check
    bool hasExistingAnalysis();
    void sendToAI(const QString &message);
    QString buildProjectContext();
    QString buildFullProjectContext();
    QString getApiEndpoint();
    QByteArray buildRequestBody(const QString &message);
    void parseResponse(const QByteArray &data);
    void saveAnalysisToFile(const QString &analysis);
    QString markdownToHtml(const QString &markdown);
    QString escapeHtml(const QString &text);
    
    // File operations
    void processFileOperations(const QString &response);
    bool createFile(const QString &relativePath, const QString &content);
    bool modifyFile(const QString &relativePath, const QString &oldContent, const QString &newContent);
    bool deleteFile(const QString &relativePath);
    bool appendToFile(const QString &relativePath, const QString &content);
    QString readFileContent(const QString &relativePath);
    
    // CLI agent management
    QString detectNvmPath();
    void detectEnvironment();
    void startCliAgent();
    void stopCliAgent();
    void sendToCliAgent(const QString &message);
    QString getCliAgentCommand();
    QStringList getCliAgentArgs();
    void appendTerminalOutput(const QString &text, const QString &color = QString());
    void switchToTerminalView();
    void switchToAiAssistantView();
    
    // System prompt for file operations
    QString getSystemPrompt();
};

#endif // AICONSOLEWIDGET_H
