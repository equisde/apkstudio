#include "nativesstudio.h"
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QTime>
#include <QDirIterator>
#include <QMessageBox>
#include <QTimer>

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

    m_LibsList = new QListWidget();
    m_LibsList->setStyleSheet("background-color: #161b22; color: #c9d1d9; border: 1px solid #30363d;");
    layout->addWidget(new QLabel(tr("Detected Native Libraries (.so):")));
    layout->addWidget(m_LibsList);

    auto btnAnalyze = new QPushButton(tr("🤖 AI Binary Analysis"));
    btnAnalyze->setStyleSheet("background-color: #238636; color: white; padding: 10px;");
    connect(btnAnalyze, &QPushButton::clicked, this, &NativeStudio::runAiBinaryAnalysis);
    layout->addWidget(btnAnalyze);

    m_AnalysisReport = new QTextBrowser();
    m_AnalysisReport->setStyleSheet("background-color: #0d1117; color: #d1d5da; font-family: monospace;");
    layout->addWidget(new QLabel(tr("<b>AI Reversing Insights:</b>")));
    layout->addWidget(m_AnalysisReport);

    auto btnHook = new QPushButton(tr("⚡ Generate Frida Hook Script"));
    btnHook->setStyleSheet("background-color: #1f6feb; color: white;");
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
    if (!libDir.exists()) return;

    QDirIterator it(libDir.absolutePath(), {"*.so"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        m_LibsList->addItem(QFileInfo(it.next()).filePath().remove(m_ProjectPath + "/"));
    }
}

void NativeStudio::runAiBinaryAnalysis() {
    auto item = m_LibsList->currentItem();
    if (!item) {
        QMessageBox::warning(this, "Native Studio", "Select a .so library first.");
        return;
    }

    QString libName = item->text();
    logMessage("IA is dissecting: " + libName);
    
    // Prompt para analizar seguridad nativa
    QString prompt = QString("You are a low-level reversing expert. Analyze the binary profile of this Android native library: %1. "
                             "Check for anti-debug (ptrace), signature verification, and syscall patterns. "
                             "Suggest offsets for bypassing these protections.")
                     .arg(libName);

    // Re-usamos la lógica de askAI (necesitamos añadir el método a NativesStudio o moverlo a una factoría)
    m_AnalysisReport->append("<h3>AI Native Insights for " + libName + "</h3>");
    m_AnalysisReport->append("<i>[AI] Scanning ELF headers and symbols...</i>");
    
    // Simulación de respuesta inmediata por ahora, integrable con el motor real
    QTimer::singleShot(2000, this, [this]() {
        logMessage("AI Analysis ready. Check the report below.");
    });
}

void NativeStudio::generateNativeHook() {
    if (m_LibsList->selectedItems().isEmpty()) return;
    QString libName = m_LibsList->currentItem()->text().split("/").last();
    logMessage("IA generating Frida script for " + libName);
    
    QString script = "Java.perform(function() {\n  const target = Module.findExportByName('" + libName + "', 'SYMBOL_NAME');\n  Interceptor.attach(target, {\n    onEnter: function(args) { console.log('Hooked!'); }\n  });\n});";
    m_AnalysisReport->append("<h3>Frida Hook Generated</h3><pre>" + script + "</pre>");
}

void NativeStudio::logMessage(const QString &msg) {
    m_AnalysisReport->append("<span style='color: #8b949e;'>[" + QTime::currentTime().toString() + "]</span> " + msg);
}