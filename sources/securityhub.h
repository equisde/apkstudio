#ifndef SECURITYHUB_H
#define SECURITYHUB_H

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTextBrowser>
#include <QGroupBox>
#include <QNetworkAccessManager>
#include <functional>

class SecurityHub : public QWidget
{
    Q_OBJECT
public:
    explicit SecurityHub(const QString &projectPath, QWidget *parent = nullptr);
    void setProjectPath(const QString &path);

private slots:
    void analyzeNetworkSecurity();
    void analyzeTampering();
    void runSSLPinningBypass();
    void generateSecurityReport();

private:
    void setupUI();
    void askAI(const QString &prompt, std::function<void(const QString&)> callback);
    QString collectSmaliContext(const QString &searchPattern);
    void logMessage(const QString &msg, const QString &type = "info");
    
    QString m_ProjectPath;
    QTextBrowser *m_AiSecurityLog;
    QNetworkAccessManager *m_NetworkManager;
};

#endif // SECURITYHUB_H