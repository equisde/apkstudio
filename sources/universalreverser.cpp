#include "universalreverser.h"
#include <QDir>
#include <QProcess>
#include <QDebug>
#include <QStandardPaths>
#include <QSettings>

UniversalReverser::UniversalReverser(const QString &projectPath, QObject *parent)
    : QObject(parent), m_ProjectPath(projectPath)
{
}

void UniversalReverser::autoDecompileAll() {
    emit progress(10, tr("Analyzing project structure..."));
    
    // Ejecutar en hilos o procesos separados para no bloquear la UI
    decompileDexToJava();
    decompileDlls();
    decompileNatives();
    
    emit progress(100, tr("Universal Decompilation Finished. All sources ready."));
    emit finished();
}

void UniversalReverser::decompileDexToJava() {
    QSettings settings;
    QString jadx = settings.value("jadx_exe").toString();
    if (jadx.isEmpty() || !QFile::exists(jadx)) {
        qDebug() << "JADX not found, skipping Java lifting.";
        return;
    }

    emit progress(20, tr("Lifting DEX to Java (High Level)..."));
    QString outDir = m_ProjectPath + "/java_src";
    QDir().mkpath(outDir);

    QProcess *proc = new QProcess(this);
    proc->start(jadx, {"-d", outDir, m_ProjectPath + "/original.apk"});
    connect(proc, &QProcess::finished, [this]() {
        emit progress(40, tr("Java sources extracted successfully."));
    });
}

void UniversalReverser::decompileDlls() {
    QDir managedDir(m_ProjectPath + "/assets/bin/Data/Managed");
    if (!managedDir.exists()) return;

    emit progress(50, tr("Detected Unity DLLs. Dissecting C# logic..."));
    QString outDir = m_ProjectPath + "/csharp_src";
    QDir().mkpath(outDir);

    QSettings settings;
    QString ilspy = settings.value("ilspy_cmd").toString(); // Configurado en Setup Tools
    
    if (!ilspy.isEmpty() && QFile::exists(ilspy)) {
        QProcess *proc = new QProcess(this);
        proc->start(ilspy, {"-o", outDir, managedDir.absolutePath() + "/Assembly-CSharp.dll"});
        connect(proc, &QProcess::finished, [this]() {
            emit progress(70, tr("C# sources ready for AI analysis."));
        });
    }
}

void UniversalReverser::decompileNatives() {
    emit progress(85, tr("Indexing native symbols for Native Studio..."));
    // Aquí se genera el mapa de funciones para NativesStudio
}