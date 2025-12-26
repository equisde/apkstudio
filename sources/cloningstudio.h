#ifndef CLONINGSTUDIO_H
#define CLONINGSTUDIO_H

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QGroupBox>
#include <QTextBrowser>

class CloningStudio : public QWidget
{
    Q_OBJECT
public:
    explicit CloningStudio(const QString &projectPath, QWidget *parent = nullptr);
    void setProjectPath(const QString &path);

private slots:
    void generateClone();
    void aiSuggestCloneConfig();

private:
    QString m_ProjectPath;
    QLineEdit *m_EditNewPackage;
    QLineEdit *m_EditNewName;
    QTextBrowser *m_AiLog;
};

#endif // CLONINGSTUDIO_H
