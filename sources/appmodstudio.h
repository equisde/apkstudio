#ifndef APPMODSTUDIO_H
#define APPMODSTUDIO_H

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTableWidget>
#include <QTextBrowser>
#include <QGroupBox>
#include <QNetworkAccessManager>
#include <functional>

class AppModStudio : public QWidget
{
    Q_OBJECT
public:
    explicit AppModStudio(const QString &projectPath, QWidget *parent = nullptr);
    void setProjectPath(const QString &path);

private slots:
    void scanForPremiumLogic();
    void applyUnlockPatch();
    void aiSuggestModifications();

private:
    void setupUI();
    void logMessage(const QString &msg);
    void askAI(const QString &prompt, std::function<void(const QString&)> callback);
    QString collectCodeContext(const QString &pattern);
    
    QString m_ProjectPath;
    QTableWidget *m_ModTable;
    QTextBrowser *m_AiLog;
    QNetworkAccessManager *m_NetworkManager;
    
    struct ModTarget {
        QString filePath;
        int lineNumber;
        QString functionName;
        QString modType;
        QString originalCode;
        QString patchCode;
    };
    QList<ModTarget> m_ModTargets;
};

#endif // APPMODSTUDIO_H