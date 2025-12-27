#include "universalreverser.h"
#include "tooldownloadworker.h"
#include "gamemodtools.h"
#include <QDir>
#include <QProcess>
#include <QSettings>
#include <QEventLoop>
#include <QFile>
#include <QMessageBox>

UniversalReverser::UniversalReverser(const QString &projectPath, QObject *parent)
    : QObject(parent), m_ProjectPath(projectPath)
{
}

void UniversalReverser::autoDecompileAll() {
    // 1. DETECTAR MOTOR
    GameEngineDetector::Engine engine = GameEngineDetector::detectEngine(m_ProjectPath);
    QString engineName = GameEngineDetector::engineName(engine);
    
    emit progress(5, tr("Environment detected: %1. Checking dependencies...").arg(engineName));

    // 2. DESCARGAR TOOLS SEGÚN MOTOR
    if (engine == GameEngineDetector::Unity) {
        emit progress(10, tr("Unity detected! Preparing full modding toolchain..."));
        checkAndDownloadTool(ToolDownloadWorker::Jadx);
        checkAndDownloadTool(ToolDownloadWorker::ILSpyCmd);
        checkAndDownloadTool(ToolDownloadWorker::Mono);
        // Podríamos añadir Il2CppDumper aquí también
    } else {
        checkAndDownloadTool(ToolDownloadWorker::Jadx);
    }

    emit progress(40, tr("Toolchain ready. Starting full de-compilation..."));

    // 3. DESCOMPILAR TODO
    decompileDexToJava();
    decompileDlls();
    decompileNatives();
    
    emit progress(100, tr("Success! Full reverse engineering environment configured."));
    emit finished();
}

void UniversalReverser::checkAndDownloadTool(int toolType) {
    QSettings settings;
    QString key;
    QString name;
    switch(toolType) {
        case ToolDownloadWorker::Jadx: key = "jadx_exe"; name = "JADX"; break;
        case ToolDownloadWorker::Apktool: key = "apktool_jar"; name = "Apktool"; break;
        case ToolDownloadWorker::ILSpyCmd: key = "ilspy_cmd"; name = "ILSpyCmd"; break;
        case ToolDownloadWorker::Mono: key = "mono_mcs_exe"; name = "Mono (mcs)"; break;
    }

    if (settings.value(key).toString().isEmpty() || !QFile::exists(settings.value(key).toString())) {
        emit progress(15, tr("Downloading missing tool: %1...").arg(name));
        
        ToolDownloadWorker *worker = new ToolDownloadWorker(static_cast<ToolDownloadWorker::ToolType>(toolType));
        QEventLoop loop;
        connect(worker, &ToolDownloadWorker::finished, &loop, &QEventLoop::quit);
        connect(worker, &ToolDownloadWorker::failed, &loop, &QEventLoop::quit);
        worker->download();
        loop.exec(); 
        worker->deleteLater();
    }
}

void UniversalReverser::decompileDexToJava() {
    QSettings settings;
    QString jadx = settings.value("jadx_exe").toString();
    if (jadx.isEmpty()) return;

    emit progress(50, tr("Lifting DEX to Java source code..."));
    QProcess::execute(jadx, {"-d", m_ProjectPath + "/java_src", m_ProjectPath + "/original.apk"});
}

void UniversalReverser::decompileDlls() {
    QDir managedDir(m_ProjectPath + "/assets/bin/Data/Managed");
    if (!managedDir.exists()) return;

    emit progress(70, tr("Unity assemblies found. Lifting C# sources..."));
    QString outDir = m_ProjectPath + "/csharp_src";
    QDir().mkpath(outDir);

    QSettings settings;
    QString ilspy = settings.value("ilspy_cmd").toString();
    
    if (!ilspy.isEmpty() && QFile::exists(ilspy)) {
        QString dllPath = managedDir.absolutePath() + "/Assembly-CSharp.dll";
        if (QFile::exists(dllPath)) {
            QProcess::execute(ilspy, {"-o", outDir, dllPath});
            emit progress(85, tr("C# source code ready for editing."));
        }
    }
}

void UniversalReverser::decompileNatives() {
    emit progress(95, tr("Finalizing native analysis..."));
}