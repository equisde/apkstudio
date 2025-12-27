#include "apkrecompileworker.h"
#include <QDir>
#include <QProcess>

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
    emit progress(60, "Running Apktool build...");
    // ... Lógica de Apktool ya existente ...
}