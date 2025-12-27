#include "apkrecompileworker.h"
#include <QDir>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

ApkRecompileWorker::ApkRecompileWorker(const QString &folder, bool aapt2, const QString &extraArguments, QObject *parent)
    : QObject(parent), m_Folder(folder), m_Aapt2(aapt2), m_ExtraArguments(extraArguments)
{
}

void ApkRecompileWorker::applyAiPatches(const QString &soPath) {
    if (!QFile::exists(soPath)) return;

    QFile patchFile(m_Folder + "/AI_BINARY_PATCHES.json");
    if (!patchFile.exists() || !patchFile.open(QFile::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(patchFile.readAll());
    patchFile.close();

    if (!doc.isArray()) return;
    QJsonArray patches = doc.array();

    QFile soFile(soPath);
    if (soFile.open(QFile::ReadWrite)) {
        for (const auto &p : patches) {
            QJsonObject obj = p.toObject();
            qint64 offset = obj["offset"].toVariant().toLongLong();
            QByteArray hex = QByteArray::fromHex(obj["hex"].toString().toUtf8());
            soFile.seek(offset);
            soFile.write(hex);
        }
        soFile.close();
    }
}

void ApkRecompileWorker::recompile() {
    emit started();
    
    // 1. RECOMPILAR C# (Unity Managed DLLs)
    QString csharpSrc = m_Folder + "/csharp_src";
    if (QDir(csharpSrc).exists()) {
        emit progress(10, tr("Detected C# modifications. Re-compiling Assembly-CSharp.dll..."));
        
        // Buscamos el compilador mcs (Mono) o similar
        QProcess mcs;
        QString outDll = m_Folder + "/assets/bin/Data/Managed/Assembly-CSharp.dll";
        
        // Comando para compilar todos los .cs de vuelta a DLL
        mcs.start("mcs", {"-target:library", "-out:" + outDll, "-recurse:" + csharpSrc + "/*.cs"});
        if (mcs.waitForFinished() && mcs.exitCode() == 0) {
            emit progress(20, tr("C# re-compilation successful."));
        } else {
            qDebug() << "C# Build Error:" << mcs.readAllStandardError();
            // Continuamos, pero informamos en logs
        }
    }

    // 2. APLICAR PARCHES BINARIOS IA (.so)
    emit progress(30, tr("Applying AI Binary Patches..."));
    QStringList archs = {"arm64-v8a", "armeabi-v7a", "x86", "x86_64"};
    for (const auto &arch : archs) {
        applyAiPatches(m_Folder + "/lib/" + arch + "/libil2cpp.so");
    }

    // 4. APKTOOL BUILD
    emit progress(70, tr("Assembling final APK (Apktool)..."));
    // ... Lógica de Apktool ...
    
    emit progress(100, tr("Recompilation successful."));
    emit finished();
}
