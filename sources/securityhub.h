#ifndef SECURITYHUB_H
#define SECURITYHUB_H

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTextBrowser>
#include <QGroupBox>

class SecurityHub : public QWidget
{
    Q_OBJECT
public:
    explicit SecurityHub(const QString &projectPath, QWidget *parent = nullptr);

private slots:
    void analyzeNetworkSecurity();
    void analyzeTampering();
    void runSSLPinningBypass();
    void generateSecurityReport();

private:
    QString m_ProjectPath;
    QTextBrowser *m_AiSecurityLog;
};

#endif // SECURITYHUB_H
