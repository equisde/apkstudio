#ifndef NATIVESTUDIO_H
#define NATIVESTUDIO_H

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QTextBrowser>
#include <QGroupBox>
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
    QString m_ProjectPath;
    QListWidget *m_LibsList;
    QTextBrowser *m_AnalysisReport;
};

#endif
