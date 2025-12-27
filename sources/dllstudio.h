#ifndef DLLSTUDIO_H
#define DLLSTUDIO_H

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QTextBrowser>
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
    QString m_ProjectPath;
    QListWidget *m_DllList;
    QTextBrowser *m_AiInsights;
};

#endif
