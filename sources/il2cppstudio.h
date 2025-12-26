#ifndef IL2CPPSTUDIO_H
#define IL2CPPSTUDIO_H

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QTextBrowser>
#include <QGroupBox>
#include <QNetworkAccessManager>
#include "aitoolfactory.h"

class Il2CppStudio : public QWidget
{
    Q_OBJECT
public:
    explicit Il2CppStudio(const QString &projectPath, QWidget *parent = nullptr);
    void setProjectPath(const QString &path);

private slots:
    void loadDumpFile();
    void runAiTool();
    void generateModMenu();

private:
    void setupUI();
    void logMessage(const QString &msg);
    void askAI(const QString &prompt, std::function<void(const QString&)> callback);

    QString m_ProjectPath;
    QString m_DumpContent;
    QListWidget *m_ToolsList;
    QTextBrowser *m_AnalysisReport;
    QList<AITool> m_Tools;
    QNetworkAccessManager *m_NetworkManager;
};

#endif // IL2CPPSTUDIO_H
