#ifndef AICONSOLEWIDGET_H
#define AICONSOLEWIDGET_H

#include <QWidget>
#include <QTextBrowser>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <functional>

class AIConsoleWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AIConsoleWidget(QWidget *parent = nullptr);
    void setProjectPath(const QString &path);

private slots:
    void handleCommand();

private:
    void logMessage(const QString &msg, const QString &sender);
    void askAI(const QString &prompt);

    QTextBrowser *m_Output;
    QLineEdit *m_Input;
    QNetworkAccessManager *m_NetworkManager;
    QString m_ProjectPath;
};

#endif // AICONSOLEWIDGET_H