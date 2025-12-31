#include "apkrecompileworker.h"
#include <QDir>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QSettings>
#include <QDebug>
#include <QDirIterator>
#include <QRegularExpression>

ApkRecompileWorker::ApkRecompileWorker(const QString &folder, bool aapt2, const QString &extraArguments, QObject *parent)
    : QObject(parent), m_Folder(folder), m_Aapt2(aapt2), m_ExtraArguments(extraArguments), m_BuildProcess(nullptr)
{
}

QString ApkRecompileWorker::findNDKPath()
{
    QStringList possiblePaths;
    
#ifdef Q_OS_WIN
    QString localAppData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    possiblePaths << localAppData + "/Android/Sdk/ndk";
    possiblePaths << "C:/Android/ndk";
    possiblePaths << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/ndk";
#else
    possiblePaths << QDir::homePath() + "/Android/Sdk/ndk";
    possiblePaths << "/opt/android-ndk";
    possiblePaths << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/ndk";
#endif
    
    QString envNdk = qgetenv("ANDROID_NDK_HOME");
    if (!envNdk.isEmpty()) possiblePaths.prepend(envNdk);
    envNdk = qgetenv("NDK_ROOT");
    if (!envNdk.isEmpty()) possiblePaths.prepend(envNdk);
    
    QSettings settings;
    QString savedNdk = settings.value("ndk_path").toString();
    if (!savedNdk.isEmpty()) possiblePaths.prepend(savedNdk);
    
    for (const QString &basePath : possiblePaths) {
        QDir dir(basePath);
        if (dir.exists()) {
#ifdef Q_OS_WIN
            if (QFile::exists(basePath + "/ndk-build.cmd")) return basePath;
#else
            if (QFile::exists(basePath + "/ndk-build")) return basePath;
#endif
            QStringList versions = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::Reversed);
            for (const QString &version : versions) {
                QString versionPath = basePath + "/" + version;
#ifdef Q_OS_WIN
                if (QFile::exists(versionPath + "/ndk-build.cmd")) return versionPath;
#else
                if (QFile::exists(versionPath + "/ndk-build")) return versionPath;
#endif
            }
        }
    }
    return QString();
}

bool ApkRecompileWorker::buildDobbyIfNeeded()
{
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString dobbyLib = appDataDir + "/mod_deps/dobby/libdobby.a";
    
    if (QFile::exists(dobbyLib)) {
        return true; // Already built
    }
    
    emit progress(35, tr("Building Dobby library (first time only)..."));
    
    QString ndkPath = findNDKPath();
    if (ndkPath.isEmpty()) {
        qWarning() << "NDK not found, cannot build Dobby";
        return false;
    }
    
    QString dobbyDir = appDataDir + "/dobby_src";
    QString buildDir = dobbyDir + "/build";
    
    // Clone if needed
    if (!QDir(dobbyDir).exists()) {
        QProcess git;
        git.start("git", {"clone", "--depth", "1", "https://github.com/jmpews/Dobby.git", dobbyDir});
        if (!git.waitForFinished(120000)) { // 2 min timeout
            qWarning() << "Failed to clone Dobby";
            return false;
        }
    }
    
    QDir().mkpath(buildDir);
    QDir().mkpath(appDataDir + "/mod_deps/dobby");
    
    // Run CMake
    QProcess cmake;
    cmake.setWorkingDirectory(buildDir);
    QString cmakeArgs = QString("-DCMAKE_TOOLCHAIN_FILE=%1/build/cmake/android.toolchain.cmake "
                                "-DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24 -DCMAKE_BUILD_TYPE=Release ..")
                        .arg(ndkPath);
#ifdef Q_OS_WIN
    cmake.start("cmd", {"/c", "cmake " + cmakeArgs});
#else
    cmake.start("sh", {"-c", "cmake " + cmakeArgs});
#endif
    if (!cmake.waitForFinished(120000)) {
        qWarning() << "CMake failed for Dobby";
        return false;
    }
    
    // Run make
    QProcess make;
    make.setWorkingDirectory(buildDir);
#ifdef Q_OS_WIN
    make.start("cmd", {"/c", "cmake --build . --config Release"});
#else
    make.start("make", {"-j4"});
#endif
    if (!make.waitForFinished(300000)) { // 5 min timeout
        qWarning() << "Make failed for Dobby";
        return false;
    }
    
    // Copy library
    QString builtLib = buildDir + "/libdobby.a";
    if (QFile::exists(builtLib)) {
        QFile::copy(builtLib, dobbyLib);
        // Also copy header
        QFile::copy(dobbyDir + "/include/dobby.h", appDataDir + "/mod_deps/dobby/dobby.h");
        return true;
    }
    
    return false;
}

bool ApkRecompileWorker::compileModMenu()
{
    QString modProjectDir = m_Folder + "/mod_menu_project";
    
    if (!QDir(modProjectDir).exists()) {
        // No mod project, skip
        return true;
    }
    
    // Check for jni/main.cpp
    if (!QFile::exists(modProjectDir + "/jni/main.cpp")) {
        qDebug() << "No mod menu source found";
        return true;
    }
    
    emit progress(40, tr("Compiling mod menu native library..."));
    
    QString ndkPath = findNDKPath();
    if (ndkPath.isEmpty()) {
        emit progress(40, tr("⚠️ NDK not found - skipping mod compilation. Using Frida script instead."));
        qWarning() << "NDK not found, cannot compile mod menu";
        return true; // Don't fail, just skip native compilation
    }
    
    // Ensure dependencies exist
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString imguiPath = appDataDir + "/mod_deps/imgui";
    QString dobbyPath = appDataDir + "/mod_deps/dobby";
    
    // Copy ImGui if exists in global deps but not in project
    if (QFile::exists(imguiPath + "/imgui.h") && !QFile::exists(modProjectDir + "/jni/imgui/imgui.cpp")) {
        emit progress(42, tr("Copying ImGui dependencies..."));
        QDir imguiDir(imguiPath);
        QStringList files = imguiDir.entryList({"*.h", "*.cpp"}, QDir::Files);
        for (const QString &file : files) {
            QFile::copy(imguiPath + "/" + file, modProjectDir + "/jni/imgui/" + file);
        }
    }
    
    // Copy Dobby if exists
    if (QFile::exists(dobbyPath + "/dobby.h") && !QFile::exists(modProjectDir + "/jni/dobby/dobby.h")) {
        QFile::copy(dobbyPath + "/dobby.h", modProjectDir + "/jni/dobby/dobby.h");
    }
    if (QFile::exists(dobbyPath + "/libdobby.a") && !QFile::exists(modProjectDir + "/jni/dobby/libdobby.a")) {
        QFile::copy(dobbyPath + "/libdobby.a", modProjectDir + "/jni/dobby/libdobby.a");
    }
    
    // Build Dobby if needed
    if (!QFile::exists(modProjectDir + "/jni/dobby/libdobby.a")) {
        if (!buildDobbyIfNeeded()) {
            emit progress(45, tr("⚠️ Could not build Dobby - mod menu may not hook properly"));
        } else {
            // Copy after building
            QFile::copy(dobbyPath + "/libdobby.a", modProjectDir + "/jni/dobby/libdobby.a");
            QFile::copy(dobbyPath + "/dobby.h", modProjectDir + "/jni/dobby/dobby.h");
        }
    }
    
    emit progress(50, tr("Running ndk-build..."));
    
    // Run ndk-build
    QProcess ndkBuild;
    ndkBuild.setWorkingDirectory(modProjectDir);
    
#ifdef Q_OS_WIN
    QString ndkBuildCmd = ndkPath + "/ndk-build.cmd";
    ndkBuild.start(ndkBuildCmd, {"NDK_PROJECT_PATH=.", "APP_BUILD_SCRIPT=./jni/Android.mk", "-j4"});
#else
    QString ndkBuildCmd = ndkPath + "/ndk-build";
    ndkBuild.start(ndkBuildCmd, {"NDK_PROJECT_PATH=.", "APP_BUILD_SCRIPT=./jni/Android.mk", "-j4"});
#endif
    
    if (!ndkBuild.waitForFinished(300000)) { // 5 min timeout
        qWarning() << "ndk-build timed out";
        emit progress(55, tr("⚠️ Mod compilation timed out"));
        return false;
    }
    
    if (ndkBuild.exitCode() != 0) {
        qWarning() << "ndk-build failed:" << ndkBuild.readAllStandardError();
        emit progress(55, tr("⚠️ Mod compilation failed - check logs"));
        return false;
    }
    
    emit progress(55, tr("✅ Mod menu compiled successfully"));
    return true;
}

void ApkRecompileWorker::copyModLibraryToApk()
{
    QString modProjectDir = m_Folder + "/mod_menu_project";
    QStringList archs = {"arm64-v8a", "armeabi-v7a"};
    
    for (const QString &arch : archs) {
        QString srcLib = modProjectDir + "/libs/" + arch + "/libmodmenu.so";
        QString destDir = m_Folder + "/lib/" + arch;
        QString destLib = destDir + "/libmodmenu.so";
        
        if (QFile::exists(srcLib)) {
            QDir().mkpath(destDir);
            
            // Remove existing if present
            if (QFile::exists(destLib)) {
                QFile::remove(destLib);
            }
            
            if (QFile::copy(srcLib, destLib)) {
                qDebug() << "Copied mod library to:" << destLib;
            } else {
                qWarning() << "Failed to copy mod library to:" << destLib;
            }
        }
    }
}

void ApkRecompileWorker::injectModLoader()
{
    // Find the main Activity's smali file and inject the mod loader
    QString smaliDir = m_Folder + "/smali";
    if (!QDir(smaliDir).exists()) {
        smaliDir = m_Folder + "/smali_classes2"; // Some APKs use multidex
    }
    
    // Copy ModLoader.smali to the APK
    QString modLoaderSrc = m_Folder + "/mod_menu_project/smali_inject/com/modmenu/ModLoader.smali";
    QString modLoaderDest = smaliDir + "/com/modmenu/ModLoader.smali";
    
    if (QFile::exists(modLoaderSrc)) {
        QDir().mkpath(QFileInfo(modLoaderDest).absolutePath());
        
        if (QFile::exists(modLoaderDest)) {
            QFile::remove(modLoaderDest);
        }
        
        if (QFile::copy(modLoaderSrc, modLoaderDest)) {
            emit progress(62, tr("Injected ModLoader.smali"));
        }
    }
    
    // Find main activity from AndroidManifest.xml
    QFile manifest(m_Folder + "/AndroidManifest.xml");
    if (!manifest.open(QIODevice::ReadOnly)) return;
    
    QString manifestContent = QString::fromUtf8(manifest.readAll());
    manifest.close();
    
    // Find launcher activity
    QRegularExpression activityRegex(
        R"(<activity[^>]*android:name="([^"]+)"[^>]*>[\s\S]*?<intent-filter[\s\S]*?android\.intent\.action\.MAIN[\s\S]*?</intent-filter>)",
        QRegularExpression::MultilineOption);
    
    QRegularExpressionMatch match = activityRegex.match(manifestContent);
    if (!match.hasMatch()) {
        // Try simpler pattern
        activityRegex.setPattern(R"(<activity[^>]*android:name="([^"]+)"[^>]*android\.intent\.category\.LAUNCHER)");
        match = activityRegex.match(manifestContent);
    }
    
    if (match.hasMatch()) {
        QString mainActivity = match.captured(1);
        
        // Convert to smali path
        QString smaliPath = mainActivity.replace(".", "/");
        if (!smaliPath.startsWith("/")) {
            // Relative class name, try to find package
            QRegularExpression pkgRegex(R"(package="([^"]+)")");
            QRegularExpressionMatch pkgMatch = pkgRegex.match(manifestContent);
            if (pkgMatch.hasMatch()) {
                QString pkg = pkgMatch.captured(1).replace(".", "/");
                smaliPath = pkg + "/" + smaliPath;
            }
        } else {
            smaliPath = smaliPath.mid(1); // Remove leading dot
        }
        
        QString activitySmaliPath = smaliDir + "/" + smaliPath + ".smali";
        
        if (QFile::exists(activitySmaliPath)) {
            QFile activityFile(activitySmaliPath);
            if (activityFile.open(QIODevice::ReadWrite)) {
                QString content = QString::fromUtf8(activityFile.readAll());
                
                // Check if already injected
                if (!content.contains("Lcom/modmenu/ModLoader;->load()V")) {
                    // Find onCreate method and inject
                    QRegularExpression onCreateRegex(
                        R"(\.method[^\n]*onCreate\(Landroid/os/Bundle;\)V[\s\S]*?\.locals\s+(\d+))");
                    
                    QRegularExpressionMatch onCreateMatch = onCreateRegex.match(content);
                    if (onCreateMatch.hasMatch()) {
                        int insertPos = onCreateMatch.capturedEnd();
                        QString injection = "\n\n    # APK Studio: Load mod menu\n"
                                            "    invoke-static {}, Lcom/modmenu/ModLoader;->load()V\n";
                        
                        content.insert(insertPos, injection);
                        
                        activityFile.seek(0);
                        activityFile.write(content.toUtf8());
                        activityFile.resize(activityFile.pos());
                        
                        emit progress(65, tr("Injected mod loader into %1").arg(mainActivity));
                    }
                } else {
                    emit progress(65, tr("Mod loader already injected"));
                }
                
                activityFile.close();
            }
        }
    }
}

bool ApkRecompileWorker::compileNativeLibraries()
{
    // This compiles any native libraries that need recompilation
    // For now, focuses on mod menu
    return compileModMenu();
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

    // 3. COMPILAR MOD MENU NATIVO (si existe proyecto)
    QString modProjectDir = m_Folder + "/mod_menu_project";
    if (QDir(modProjectDir).exists() && QFile::exists(modProjectDir + "/jni/main.cpp")) {
        emit progress(40, tr("Compiling mod menu native library..."));
        
        if (compileModMenu()) {
            // 3.1 Copiar librería compilada al APK
            emit progress(58, tr("Copying mod library to APK..."));
            copyModLibraryToApk();
            
            // 3.2 Inyectar loader smali
            emit progress(60, tr("Injecting mod loader into main activity..."));
            injectModLoader();
        } else {
            emit progress(58, tr("⚠️ Mod compilation failed, using Frida script only"));
        }
    } else {
        emit progress(60, tr("No mod menu project found, skipping native compilation"));
    }

    // 4. APKTOOL BUILD
    emit progress(70, tr("Assembling final APK (Apktool)..."));
    
    QProcess apktool;
    QStringList args;
    args << "b" << m_Folder;
    
    if (m_Aapt2) {
        args << "--use-aapt2";
    }
    
    if (!m_ExtraArguments.isEmpty()) {
        args << m_ExtraArguments.split(" ", Qt::SkipEmptyParts);
    }
    
    // Find apktool
    QString apktoolPath = "apktool"; // Assume in PATH
    QSettings settings;
    QString customApktool = settings.value("apktool_path").toString();
    if (!customApktool.isEmpty() && QFile::exists(customApktool)) {
        apktoolPath = customApktool;
    }
    
    apktool.start("java", QStringList() << "-jar" << apktoolPath << args);
    
    emit progress(75, tr("Running Apktool build..."));
    
    if (!apktool.waitForFinished(600000)) { // 10 min timeout
        emit progress(80, tr("⚠️ Apktool build timed out"));
        emit recompileFailed(m_Folder);
        emit finished();
        return;
    }
    
    if (apktool.exitCode() != 0) {
        QString error = QString::fromUtf8(apktool.readAllStandardError());
        qWarning() << "Apktool build failed:" << error;
        emit progress(80, tr("❌ Apktool build failed"));
        emit recompileFailed(m_Folder);
        emit finished();
        return;
    }
    
    emit progress(90, tr("APK assembled successfully"));
    
    // 5. SIGN APK (if signer available)
    QString outputApk = m_Folder + "/dist/" + QFileInfo(m_Folder).fileName() + ".apk";
    if (QFile::exists(outputApk)) {
        emit progress(95, tr("Signing APK..."));
        
        // Try uber-apk-signer
        QProcess signer;
        QString signerPath = settings.value("uber_signer_path").toString();
        
        if (!signerPath.isEmpty() && QFile::exists(signerPath)) {
            signer.start("java", {"-jar", signerPath, "-a", outputApk, "--overwrite"});
            if (signer.waitForFinished(120000) && signer.exitCode() == 0) {
                emit progress(98, tr("APK signed successfully"));
            } else {
                emit progress(98, tr("⚠️ Signing failed - APK is unsigned"));
            }
        } else {
            emit progress(98, tr("ℹ️ No signer configured - APK is unsigned"));
        }
    }
    
    emit progress(100, tr("✅ Recompilation complete!"));
    emit recompileFinished(m_Folder);
    emit finished();
}
