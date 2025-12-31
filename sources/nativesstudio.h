#ifndef NATIVESTUDIO_H
#define NATIVESTUDIO_H

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QTextBrowser>
#include <QGroupBox>
#include <QNetworkAccessManager>
#include <functional>
#include "aitoolfactory.h"

class NativeStudio : public QWidget
{
    Q_OBJECT
public:
    explicit NativeStudio(const QString &projectPath, QWidget *parent = nullptr);
    void setProjectPath(const QString &path);

private slots:
    void scanNativeLibs();
    void runAiBinaryAnalysis();
    void generateNativeHook();

private:
    void setupUI();
    void logMessage(const QString &msg);
    void askAI(const QString &prompt, std::function<void(const QString&)> callback);
    QString extractLibraryInfo(const QString &libPath);
    
    QString m_ProjectPath;
    QListWidget *m_LibsList;
    QTextBrowser *m_AnalysisReport;
    QNetworkAccessManager *m_NetworkManager;
};

#endif
