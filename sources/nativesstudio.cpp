#include "nativesstudio.h"
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QTime>

NativeStudio::NativeStudio(const QString &projectPath, QWidget *parent)
    : QWidget(parent), m_ProjectPath(projectPath)
{
    setupUI();
    if (!m_ProjectPath.isEmpty()) scanNativeLibs();
}

void NativeStudio::setupUI() {
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);

    auto header = new QLabel(tr("<b>NATIVE BINARY STUDIO</b>"));
    header->setStyleSheet("color: #f85149; font-size: 14px;");
    layout->addWidget(header);

    auto groupLibs = new QGroupBox(tr("Detected .so Libraries"));
    auto libsLayout = new QVBoxLayout(groupLibs);
    m_LibsList = new QListWidget();
    m_LibsList->setStyleSheet("background-color: #161b22; color: #c9d1d9;");
    libsLayout->addWidget(m_LibsList);
    layout->addWidget(groupLibs);

    auto btnAnalyze = new QPushButton(tr("🤖 AI Binary Dissection"));
    btnAnalyze->setStyleSheet("background-color: #238636; color: white;");
    connect(btnAnalyze, &QPushButton::clicked, this, &NativeStudio::runAiBinaryAnalysis);
    layout->addWidget(btnAnalyze);

    m_AnalysisReport = new QTextBrowser();
    m_AnalysisReport->setStyleSheet("background-color: #0d1117; color: #d1d5da; font-family: monospace;");
    layout->addWidget(new QLabel(tr("<b>AI Reversing Insights</b>")));
    layout->addWidget(m_AnalysisReport);

    auto btnHook = new QPushButton(tr("⚡ Generate Frida/C++ Hook"));
    connect(btnHook, &QPushButton::clicked, this, &NativeStudio::generateNativeHook);
    layout->addWidget(btnHook);
}

void NativeStudio::setProjectPath(const QString &path) {
    m_ProjectPath = path;
    scanNativeLibs();
}

void NativeStudio::scanNativeLibs() {
    m_LibsList->clear();
    QDir libDir(m_ProjectPath + "/lib");
    QDirIterator it(libDir.absolutePath(), {"*.so"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        m_LibsList->addItem(QFileInfo(it.next()).fileName());
    }
}

void NativeStudio::runAiBinaryAnalysis() {
    logMessage("IA analizando cabeceras ELF y tabla de símbolos...");
}

void NativeStudio::generateNativeHook() {
    logMessage("Generando script de Frida para interceptar funciones nativas...");
}

void NativeStudio::logMessage(const QString &msg) {
    m_AnalysisReport->append("<i>[" + QTime::currentTime().toString() + "]</i> " + msg);
}
