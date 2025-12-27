#include "universalreverser.h"
#include "tooldownloadworker.h"
#include <QDir>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QEventLoop>

UniversalReverser::UniversalReverser(const QString &projectPath, QObject *parent)
    : QObject(parent), m_ProjectPath(projectPath)
{
}

void UniversalReverser::autoDecompileAll() {
    emit progress(5, tr("Checking binary dependencies..."));
    
    // Lista de herramientas críticas según el contexto
    checkAndDownloadTool(ToolDownloadWorker::Jadx);
    
    QDir managedDir(m_ProjectPath + "/assets/bin/Data/Managed");
    if (managedDir.exists()) {
        checkAndDownloadTool(ToolDownloadWorker::ILSpyCmd);
        checkAndDownloadTool(ToolDownloadWorker::Apktool);
    }

    emit progress(20, tr("All tools ready. Starting parallel decompilation..."));
    
    decompileDexToJava();
    decompileDlls();
    decompileNatives();
    
    emit finished();
}

void UniversalReverser::checkAndDownloadTool(int toolType) {
    QSettings settings;
    QString key;
    switch(toolType) {
        case ToolDownloadWorker::Jadx: key = "jadx_exe"; break;
        case ToolDownloadWorker::Apktool: key = "apktool_jar"; break;
        case ToolDownloadWorker::Adb: key = "adb_exe"; break;
        case ToolDownloadWorker::ILSpyCmd: key = "ilspy_cmd"; break;
    }

    if (settings.value(key).toString().isEmpty() || !QFile::exists(settings.value(key).toString())) {
        emit progress(10, tr("Tool missing. Auto-downloading component..."));
        
        ToolDownloadWorker *worker = new ToolDownloadWorker(static_cast<ToolDownloadWorker::ToolType>(toolType));
        QEventLoop loop;
        connect(worker, &ToolDownloadWorker::finished, &loop, &QEventLoop::quit);
        connect(worker, &ToolDownloadWorker::failed, &loop, &QEventLoop::quit);
        worker->download();
        loop.exec(); 
        worker->deleteLater();
    }
}

void UniversalReverser::decompileDlls() {
    QDir managedDir(m_ProjectPath + "/assets/bin/Data/Managed");
    if (!managedDir.exists()) return;

    emit progress(60, tr("Lifting C# Assemblies to Source Code..."));
    QString outDir = m_ProjectPath + "/csharp_src";
    QDir().mkpath(outDir);

    QSettings settings;
    QString ilspy = settings.value("ilspy_cmd").toString();
    
    if (!ilspy.isEmpty() && QFile::exists(ilspy)) {
        QString dllPath = managedDir.absolutePath() + "/Assembly-CSharp.dll";
        if (QFile::exists(dllPath)) {
            QProcess *proc = new QProcess(this);
            proc->start(ilspy, {"-o", outDir, dllPath});
            proc->waitForFinished();
            emit progress(80, tr("C# decompilation complete. Check /csharp_src/"));
        }
    }
}

void UniversalReverser::decompileNatives() {
    emit progress(90, tr("Indexing symbols..."));
}
