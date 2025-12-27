#include "apkrecompileworker.h"
#include <QDir>
#include <QProcess>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

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
    emit progress(10, "Scanning for modified sub-components...");

    // 1. RECOMPILAR DLLs (Si hay cambios en C#)
    if (QDir(m_Folder + "/decompiled_csharp/").exists()) {
        emit progress(20, "Re-compiling Assembly-CSharp.dll (C#)...");
        QProcess::execute("tools/mcs", {"-target:library", "-out:assets/bin/Data/Managed/Assembly-CSharp.dll", "decompiled_csharp/*.cs"});
    }

    // 2. RECOMPILAR NATIVO (Si hay cambios en C++)
    if (QFile::exists(m_Folder + "/jni/")) {
        emit progress(40, "Building native libraries (ndk-build)...");
        QProcess::execute("ndk-build", {"-C", m_Folder + "/jni/"});
    }

    // 3. RECOMPILAR APK (Smali + Resources)
    emit progress(60, "Applying AI Binary Patches to libil2cpp.so...");
    applyAiPatches(m_Folder + "/lib/arm64-v8a/libil2cpp.so");
    applyAiPatches(m_Folder + "/lib/armeabi-v7a/libil2cpp.so");

    emit progress(70, "Running Apktool build...");
    // ... Lógica de Apktool ya existente ...
}