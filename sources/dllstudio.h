#ifndef DLLSTUDIO_H
#define DLLSTUDIO_H

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QTextBrowser>
#include <QNetworkAccessManager>
#include <functional>
#include "aitoolfactory.h"

class DllStudio : public QWidget
{
    Q_OBJECT
public:
    explicit DllStudio(const QString &projectPath, QWidget *parent = nullptr);
    void setProjectPath(const QString &path);

private slots:
    void scanDlls();
    void analyzeWithAI();

private:
    void setupUI();
    void scanExtractedSource();
    void askAI(const QString &prompt, std::function<void(const QString&)> callback);
    QString extractDllInfo(const QString &dllPath);
    void logMessage(const QString &msg);
    
    QString m_ProjectPath;
    QListWidget *m_DllList;
    QTextBrowser *m_AiInsights;
    QNetworkAccessManager *m_NetworkManager;
};

#endif // DLLSTUDIO_H