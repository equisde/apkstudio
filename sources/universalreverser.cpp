#include "universalreverser.h"
#include <QDir>
#include <QProcess>
#include <QDebug>
#include <QStandardPaths>

UniversalReverser::UniversalReverser(const QString &projectPath, QObject *parent)
    : QObject(parent), m_ProjectPath(projectPath)
{
}

void UniversalReverser::autoDecompileAll() {
    emit progress(10, tr("Scanning APK contents..."));
    
    // 1. Descompilar Java (DEX -> Java)
    decompileDexToJava();
    
    // 2. Descompilar Unity DLLs (Si existen)
    decompileDlls();
    
    // 3. Analizar Binarios Nativos
    decompileNatives();
    
    emit progress(100, tr("Full Project Decryption Complete."));
    emit finished();
}

void UniversalReverser::decompileDexToJava() {
    emit progress(30, tr("Lifting DEX to Java (JADX Engine)..."));
    QString jadxPath = QSettings().value("jadx_exe").toString();
    if (!jadxPath.isEmpty()) {
        QProcess::execute(jadxPath, {"-d", m_ProjectPath + "/java_src", m_ProjectPath + "/original.apk"});
    }
}

void UniversalReverser::decompileDlls() {
    QDir managedDir(m_ProjectPath + "/assets/bin/Data/Managed");
    if (managedDir.exists()) {
        emit progress(60, tr("Extracting C# Logic from Unity DLLs..."));
        QString toolsDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/tools";
        // Usar ILSpyCmd o similar descargado por Setup Tools
        QProcess::execute(toolsDir + "/ilspycmd", {"-o", m_ProjectPath + "/csharp_src", managedDir.absolutePath() + "/Assembly-CSharp.dll"});
    }
}

void UniversalReverser::decompileNatives() {
    emit progress(80, tr("Dissecting Native Libraries (.so)..."));
    // Escaneo de símbolos y preparación para NativeStudio
}
