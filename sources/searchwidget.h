#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>

class SearchWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SearchWidget(const QString &projectPath, QWidget *parent = nullptr);
    void setProjectPath(const QString &path);

signals:
    void fileOpenRequested(const QString &path, int line);

private slots:
    void performSearch();
    void onResultClicked(QListWidgetItem *item);

private:
    QString m_ProjectPath;
    QLineEdit *m_SearchEdit;
    QListWidget *m_ResultsList;
    QLabel *m_StatusLabel;
};

#endif
