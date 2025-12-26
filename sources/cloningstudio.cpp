#include "cloningstudio.h"
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>

CloningStudio::CloningStudio(const QString &projectPath, QWidget *parent)
    : QWidget(parent), m_ProjectPath(projectPath)
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(15);

    auto group = new QGroupBox(tr("👯 APK Cloner"));
    auto form = new QFormLayout(group);

    m_EditNewPackage = new QLineEdit();
    m_EditNewPackage->setPlaceholderText("com.example.clone");
    form->addRow(tr("New Package Name:"), m_EditNewPackage);

    m_EditNewName = new QLineEdit();
    m_EditNewName->setPlaceholderText("App Clone");
    form->addRow(tr("New App Name:"), m_EditNewName);

    auto btnAi = new QPushButton(tr("AI Suggest Stealth Clone Config"));
    connect(btnAi, &QPushButton::clicked, this, &CloningStudio::aiSuggestCloneConfig);
    form->addRow(btnAi);

    layout->addWidget(group);

    m_AiLog = new QTextBrowser();
    m_AiLog->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4;");
    layout->addWidget(new QLabel(tr("<b>Cloning Process & AI Suggestions</b>")));
    layout->addWidget(m_AiLog);

    auto btnClone = new QPushButton(tr("⚡ Generate Clone Project"));
    btnClone->setStyleSheet("background-color: #0e639c; color: white; font-weight: bold; padding: 10px;");
    connect(btnClone, &QPushButton::clicked, this, &CloningStudio::generateClone);
    layout->addWidget(btnClone);
}

void CloningStudio::aiSuggestCloneConfig() {
    m_AiLog->append("<i>[AI] Analyzing package structure for stealth cloning...</i>");
}

void CloningStudio::generateClone() {
    QMessageBox::information(this, tr("Cloner"), tr("Cloning process started. AI is renaming resources and patching manifest..."));
}
