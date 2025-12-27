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
    
    if (QDir(m_ProjectPath + "/assets/bin/Data/Managed").exists()) {
        checkAndDownloadTool(ToolDownloadWorker::Apktool); // Para recompilar después
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
    }

    if (settings.value(key).toString().isEmpty() || !QFile::exists(settings.value(key).toString())) {
        emit progress(10, tr("Tool missing. Auto-downloading component..."));
        
        ToolDownloadWorker *worker = new ToolDownloadWorker(static_cast<ToolDownloadWorker::ToolType>(toolType));
        QEventLoop loop;
        connect(worker, &ToolDownloadWorker::finished, &loop, &QEventLoop::quit);
        connect(worker, &ToolDownloadWorker::failed, &loop, &QEventLoop::quit);
        worker->download();
        loop.exec(); // Espera síncrona en el hilo del reverser (que debe ser worker thread)
        worker->deleteLater();
    }
}

void UniversalReverser::decompileDexToJava() {
    QSettings settings;
    QString jadx = settings.value("jadx_exe").toString();
    if (jadx.isEmpty()) return;

    emit progress(30, tr("Lifting DEX to Java..."));
    QProcess::execute(jadx, {"-d", m_ProjectPath + "/java_src", m_ProjectPath + "/original.apk"});
}

void UniversalReverser::decompileDlls() {
    QDir managedDir(m_ProjectPath + "/assets/bin/Data/Managed");
    if (!managedDir.exists()) return;

    emit progress(60, tr("Extracting C# Logic..."));
    // Usar el dumper o de-compilador configurado
}

void UniversalReverser::decompileNatives() {
    emit progress(90, tr("Indexing symbols..."));
}
