#include "dllstudio.h"
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QTime>

DllStudio::DllStudio(const QString &projectPath, QWidget *parent)
    : QWidget(parent), m_ProjectPath(projectPath)
{
    setupUI();
    if (!m_ProjectPath.isEmpty()) scanDlls();
}

void DllStudio::setupUI() {
    auto layout = new QVBoxLayout(this);
    
    auto header = new QLabel(tr("<b>UNITY C# STUDIO</b>"));
    header->setStyleSheet("color: #58a6ff; font-size: 14px;");
    layout->addWidget(header);

    m_DllList = new QListWidget();
    m_DllList->setStyleSheet("background-color: #161b22; color: #c9d1d9;");
    layout->addWidget(new QLabel(tr("Managed DLLs:")));
    layout->addWidget(m_DllList);

    auto btnAnalyze = new QPushButton(tr("🤖 AI Code Analysis (C#)"));
    btnAnalyze->setStyleSheet("background-color: #238636; color: white;");
    connect(btnAnalyze, &QPushButton::clicked, this, &DllStudio::analyzeWithAI);
    layout->addWidget(btnAnalyze);

    m_AiInsights = new QTextBrowser();
    m_AiInsights->setStyleSheet("background-color: #0d1117; color: #d1d5da; font-family: monospace;");
    layout->addWidget(m_AiInsights);
}

void DllStudio::setProjectPath(const QString &path) {
    m_ProjectPath = path;
    scanDlls();
    scanExtractedSource();
}

void DllStudio::scanDlls() {
    m_DllList->clear();
    QDir managedDir(m_ProjectPath + "/assets/bin/Data/Managed");
    for (const auto &info : managedDir.entryInfoList({"*.dll"}, QDir::Files)) {
        m_DllList->addItem(info.fileName());
    }
}

void DllStudio::scanExtractedSource() {
    QDir srcDir(m_ProjectPath + "/csharp_src");
    if (srcDir.exists()) {
        m_AiInsights->append(tr("Extracted C# sources found in /csharp_src/"));
    }
}

void DllStudio::analyzeWithAI() {
    m_AiInsights->append("<i>[" + QTime::currentTime().toString() + "]</i> IA analizando lógica de Assembly-CSharp.dll...");
}
