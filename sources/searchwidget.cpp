#include "searchwidget.h"
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

SearchWidget::SearchWidget(const QString &projectPath, QWidget *parent)
    : QWidget(parent), m_ProjectPath(projectPath)
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);

    m_SearchEdit = new QLineEdit();
    m_SearchEdit->setPlaceholderText(tr("Search in project..."));
    connect(m_SearchEdit, &QLineEdit::returnPressed, this, &SearchWidget::performSearch);
    layout->addWidget(m_SearchEdit);

    m_StatusLabel = new QLabel(tr("Ready to search"));
    m_StatusLabel->setStyleSheet("color: #808080; font-size: 11px;");
    layout->addWidget(m_StatusLabel);

    m_ResultsList = new QListWidget();
    m_ResultsList->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4; border: none;");
    connect(m_ResultsList, &QListWidget::itemDoubleClicked, this, &SearchWidget::onResultClicked);
    layout->addWidget(m_ResultsList);
}

void SearchWidget::setProjectPath(const QString &path) {
    m_ProjectPath = path;
    m_ResultsList->clear();
    m_StatusLabel->setText(tr("Project path updated."));
}

void SearchWidget::performSearch() {
    QString term = m_SearchEdit->text();
    if (term.isEmpty() || m_ProjectPath.isEmpty()) return;

    m_ResultsList->clear();
    m_StatusLabel->setText(tr("Searching..."));

    int count = 0;
    QDirIterator it(m_ProjectPath, {"*.smali", "*.java", "*.cs", "*.xml"}, QDir::Files, QDirIterator::Subdirectories);
    
    while (it.hasNext()) {
        QString path = it.next();
        QFile f(path);
        if (f.open(QFile::ReadOnly | QFile::Text)) {
            QTextStream in(&f);
            int lineNum = 1;
            while (!in.atEnd()) {
                QString line = in.readLine();
                if (line.contains(term, Qt::CaseInsensitive)) {
                    auto item = new QListWidgetItem(QString("%1:%2 - %3").arg(QFileInfo(path).fileName()).arg(lineNum).arg(line.trimmed()));
                    item->setData(Qt::UserRole, path);
                    item->setData(Qt::UserRole + 1, lineNum);
                    m_ResultsList->addItem(item);
                    count++;
                }
                lineNum++;
                if (count > 500) break; // Limitar resultados
            }
            f.close();
        }
        if (count > 500) break;
    }
    m_StatusLabel->setText(tr("Found %1 matches").arg(count));
}

void SearchWidget::onResultClicked(QListWidgetItem *item) {
    // Aquí dispararíamos la apertura del archivo en MainWindow
}
