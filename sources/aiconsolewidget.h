#ifndef AICONSOLEWIDGET_H
#define AICONSOLEWIDGET_H

#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPlainTextEdit>
#include <QPushButton>
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
    
private slots:
    void handleSendMessage();
    void handleApiResponse();
    void handleApiError(QNetworkReply::NetworkError error);
    
private:
    QPlainTextEdit *m_OutputConsole;
    QLineEdit *m_InputLine;
    QPushButton *m_SendButton;
    QPushButton *m_AnalyzeButton;
    QPushButton *m_ClearButton;
    QNetworkAccessManager *m_NetworkManager;
    QNetworkReply *m_CurrentReply;
    QString m_ProjectPath;
    QStringList m_ConversationHistory;
    QString m_CurrentProjectContext;
    
    bool hasExistingAnalysis();
    void sendToAI(const QString &message);
    QString buildProjectContext();
    QString getApiEndpoint();
    QByteArray buildRequestBody(const QString &message);
    void parseResponse(const QByteArray &data);
    void saveAnalysisToFile(const QString &analysis);
};

#endif // AICONSOLEWIDGET_H
