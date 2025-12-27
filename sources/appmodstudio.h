#ifndef APPMODSTUDIO_H
#define APPMODSTUDIO_H

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTableWidget>
#include <QTextBrowser>
#include <QGroupBox>

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
    QString m_ProjectPath;
    QTableWidget *m_ModTable;
    QTextBrowser *m_AiLog;
};

#endif // APPMODSTUDIO_H