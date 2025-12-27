#include "cloningstudio.h"
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>

CloningStudio::CloningStudio(const QString &projectPath, QWidget *parent)
    : QWidget(parent), m_ProjectPath(projectPath)
{
    setupUI();
}

void CloningStudio::setProjectPath(const QString &path) {
    m_ProjectPath = path;
    m_AiLog->append("Project changed: " + path);
}

void CloningStudio::setupUI() {
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(15);

    auto group = new QGroupBox(tr("📦 APK Cloning & Renaming"));
    auto form = new QFormLayout(group);

    m_EditNewPackage = new QLineEdit();
    m_EditNewPackage->setPlaceholderText("com.new.package.name");
    form->addRow(tr("New Package Name:"), m_EditNewPackage);

    m_EditNewName = new QLineEdit();
    m_EditNewName->setPlaceholderText("Cloned App Name");
    form->addRow(tr("New App Name:"), m_EditNewName);

    auto btnAi = new QPushButton(tr("🤖 AI Suggest Clone Config"));
    connect(btnAi, &QPushButton::clicked, this, &CloningStudio::aiSuggestCloneConfig);
    form->addRow(btnAi);

    layout->addWidget(group);

    m_AiLog = new QTextBrowser();
    m_AiLog->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4;");
    layout->addWidget(new QLabel(tr("<b>AI Cloning Log</b>")));
    layout->addWidget(m_AiLog);

    auto btnClone = new QPushButton(tr("🚀 Generate Cloned APK"));
    btnClone->setStyleSheet("background-color: #238636; color: white; font-weight: bold; padding: 10px;");
    connect(btnClone, &QPushButton::clicked, this, &CloningStudio::generateClone);
    layout->addWidget(btnClone);
}

void CloningStudio::generateClone() {
    m_AiLog->append("<i>Starting cloning process...</i>");
}

void CloningStudio::aiSuggestCloneConfig() {
    m_AiLog->append("<i>[AI] Generating stealth clone configuration...</i>");
    
    // Sugerencia dinámica
    QString originalPkg = "com.original.app"; // Esto debería venir de AndroidManifest.xml
    QString suggested = originalPkg + ".clone." + QDateTime::currentDateTime().toString("mmss");
    
    m_EditNewPackage->setText(suggested);
    m_EditNewName->setText("Clon AI " + QDateTime::currentDateTime().toString("hh:mm"));
    m_AiLog->append("<span style='color: #7ee787;'>Suggested Package: " + suggested + "</span>");
}