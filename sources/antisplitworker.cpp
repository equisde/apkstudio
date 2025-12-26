#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>
#include <QTemporaryDir>
#include <QUuid>
#include "antisplitworker.h"
#include "processutils.h"

// Use Qt's built-in zip support via QProcess with jar/zip commands or manual implementation
#ifdef Q_OS_WIN
#include <windows.h>
#endif

AntiSplitWorker::AntiSplitWorker(const QStringList &inputFiles, const QString &outputFile, const bool signApk, QObject *parent)
    : QObject(parent), m_InputFiles(inputFiles), m_OutputFile(outputFile), m_SignApk(signApk)
{
}

void AntiSplitWorker::merge()
{
    emit started();
    emit mergeProgress(0, tr("Starting AntiSplit merge..."));
    
    // Create temporary working directory
    QString tempPath = QDir::tempPath() + "/apkstudio_antisplit_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    QDir tempDir;
    if (!tempDir.mkpath(tempPath)) {
        emit mergeFailed(tr("Failed to create temporary directory"));
        emit finished();
        return;
    }
    
    QStringList allApkFiles;
    int progress = 5;
    
    // Process each input file
    for (const QString &inputFile : m_InputFiles) {
        QFileInfo info(inputFile);
        QString ext = info.suffix().toLower();
        
        if (ext == "xapk" || ext == "apks" || ext == "apkm") {
            // Extract XAPK/APKS/APKM file (they are ZIP archives)
            emit mergeProgress(progress, tr("Extracting %1...").arg(info.fileName()));
            QString extractDir = tempPath + "/" + info.baseName();
            if (!extractXapk(inputFile, extractDir)) {
                emit mergeFailed(tr("Failed to extract %1").arg(info.fileName()));
                QDir(tempPath).removeRecursively();
                emit finished();
                return;
            }
            
            // Find all APK files in extracted directory
            QDirIterator it(extractDir, QStringList() << "*.apk", QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                allApkFiles << it.next();
            }
        } else if (ext == "apk") {
            // Add APK file directly
            allApkFiles << inputFile;
        }
        progress += 10;
    }
    
    if (allApkFiles.isEmpty()) {
        emit mergeFailed(tr("No APK files found in the input"));
        QDir(tempPath).removeRecursively();
        emit finished();
        return;
    }
    
    emit mergeProgress(30, tr("Found %1 APK file(s) to merge...").arg(allApkFiles.count()));
    
    // Merge all APKs into one
    if (!mergeApks(allApkFiles, tempPath, m_OutputFile)) {
        emit mergeFailed(tr("Failed to merge APK files"));
        QDir(tempPath).removeRecursively();
        emit finished();
        return;
    }
    
    emit mergeProgress(80, tr("APK files merged successfully"));
    
    // Sign the output APK if requested
    if (m_SignApk) {
        emit mergeProgress(85, tr("Signing output APK..."));
        if (!signOutputApk(m_OutputFile)) {
            emit mergeFailed(tr("Failed to sign output APK"));
            QDir(tempPath).removeRecursively();
            emit finished();
            return;
        }
    }
    
    // Cleanup
    emit mergeProgress(95, tr("Cleaning up..."));
    QDir(tempPath).removeRecursively();
    
    emit mergeProgress(100, tr("Merge completed successfully!"));
    emit mergeFinished(m_OutputFile);
    emit finished();
}

bool AntiSplitWorker::extractXapk(const QString &xapkPath, const QString &extractDir)
{
    // XAPK/APKS/APKM files are ZIP archives - use Java's jar command or 7z/unzip
    QDir dir;
    if (!dir.mkpath(extractDir)) {
        return false;
    }
    
    QString java = ProcessUtils::javaExe();
    if (!java.isEmpty()) {
        // Use Java's jar command to extract (it can handle ZIP files)
        QStringList args;
        args << "-xf" << QDir::toNativeSeparators(xapkPath);
        
        QProcess process;
        process.setWorkingDirectory(extractDir);
        
        // Try using jar command from JDK
        QString jarExe;
#ifdef Q_OS_WIN
        QFileInfo javaInfo(java);
        jarExe = javaInfo.absolutePath() + "/jar.exe";
#else
        QFileInfo javaInfo(java);
        jarExe = javaInfo.absolutePath() + "/jar";
#endif
        
        if (QFile::exists(jarExe)) {
            process.start(jarExe, args);
            if (process.waitForFinished(300000) && process.exitCode() == 0) {
                return true;
            }
        }
    }
    
    // Try PowerShell's Expand-Archive on Windows
#ifdef Q_OS_WIN
    QProcess process;
    QStringList args;
    args << "-NoProfile" << "-Command";
    args << QString("Expand-Archive -Path '%1' -DestinationPath '%2' -Force")
            .arg(xapkPath.replace("'", "''"))
            .arg(extractDir.replace("'", "''"));
    process.start("powershell.exe", args);
    if (process.waitForFinished(300000) && process.exitCode() == 0) {
        return true;
    }
#else
    // Try unzip on Unix
    QProcess process;
    QStringList args;
    args << "-o" << xapkPath << "-d" << extractDir;
    process.start("unzip", args);
    if (process.waitForFinished(300000) && process.exitCode() == 0) {
        return true;
    }
#endif
    
    return false;
}

bool AntiSplitWorker::mergeApks(const QStringList &apkFiles, const QString &workDir, const QString &outputApk)
{
    if (apkFiles.isEmpty()) {
        return false;
    }
    
    // Find the base APK (usually named base.apk or the main app APK)
    QString baseApk;
    QStringList splitApks;
    
    for (const QString &apk : apkFiles) {
        QFileInfo info(apk);
        QString name = info.fileName().toLower();
        if (name == "base.apk" || name.contains("base")) {
            baseApk = apk;
        } else {
            splitApks << apk;
        }
    }
    
    // If no base.apk found, use the first APK as base
    if (baseApk.isEmpty() && !apkFiles.isEmpty()) {
        baseApk = apkFiles.first();
        splitApks = apkFiles.mid(1);
    }
    
    // Create merge directory
    QString mergeDir = workDir + "/merge";
    QDir dir;
    dir.mkpath(mergeDir);
    
    QString java = ProcessUtils::javaExe();
    if (java.isEmpty()) {
#ifdef QT_DEBUG
        qDebug() << "Java not found";
#endif
        return false;
    }
    
    // Extract base APK
    emit mergeProgress(35, tr("Extracting base APK..."));
    QString baseExtractDir = mergeDir + "/base";
    dir.mkpath(baseExtractDir);
    
#ifdef Q_OS_WIN
    // Use jar to extract base APK
    QString jarExe;
    QFileInfo javaInfo(java);
    jarExe = javaInfo.absolutePath() + "/jar.exe";
    
    if (QFile::exists(jarExe)) {
        QProcess process;
        process.setWorkingDirectory(baseExtractDir);
        QStringList args;
        args << "-xf" << QDir::toNativeSeparators(baseApk);
        process.start(jarExe, args);
        if (!process.waitForFinished(300000) || process.exitCode() != 0) {
            // Try PowerShell
            QProcess ps;
            QStringList psArgs;
            psArgs << "-NoProfile" << "-Command";
            psArgs << QString("Expand-Archive -Path '%1' -DestinationPath '%2' -Force")
                    .arg(baseApk.replace("'", "''"))
                    .arg(baseExtractDir.replace("'", "''"));
            ps.start("powershell.exe", psArgs);
            if (!ps.waitForFinished(300000) || ps.exitCode() != 0) {
                return false;
            }
        }
    } else {
        // Use PowerShell
        QProcess ps;
        QStringList psArgs;
        psArgs << "-NoProfile" << "-Command";
        psArgs << QString("Expand-Archive -Path '%1' -DestinationPath '%2' -Force")
                .arg(baseApk.replace("'", "''"))
                .arg(baseExtractDir.replace("'", "''"));
        ps.start("powershell.exe", psArgs);
        if (!ps.waitForFinished(300000) || ps.exitCode() != 0) {
            return false;
        }
    }
#else
    QProcess process;
    QStringList args;
    args << "-o" << baseApk << "-d" << baseExtractDir;
    process.start("unzip", args);
    if (!process.waitForFinished(300000) || process.exitCode() != 0) {
        return false;
    }
#endif
    
    // Extract and merge each split APK
    int splitProgress = 40;
    int progressPerSplit = 30 / qMax(1, splitApks.count());
    
    for (const QString &splitApk : splitApks) {
        QFileInfo splitInfo(splitApk);
        emit mergeProgress(splitProgress, tr("Merging %1...").arg(splitInfo.fileName()));
        
        QString splitExtractDir = mergeDir + "/split_" + splitInfo.baseName();
        dir.mkpath(splitExtractDir);
        
#ifdef Q_OS_WIN
        QString jarExe2;
        QFileInfo javaInfo2(java);
        jarExe2 = javaInfo2.absolutePath() + "/jar.exe";
        
        bool extracted = false;
        if (QFile::exists(jarExe2)) {
            QProcess proc;
            proc.setWorkingDirectory(splitExtractDir);
            QStringList jarArgs;
            jarArgs << "-xf" << QDir::toNativeSeparators(splitApk);
            proc.start(jarExe2, jarArgs);
            if (proc.waitForFinished(300000) && proc.exitCode() == 0) {
                extracted = true;
            }
        }
        
        if (!extracted) {
            QProcess ps;
            QStringList psArgs;
            psArgs << "-NoProfile" << "-Command";
            QString splitApkEscaped = splitApk;
            QString splitExtractDirEscaped = splitExtractDir;
            psArgs << QString("Expand-Archive -Path '%1' -DestinationPath '%2' -Force")
                    .arg(splitApkEscaped.replace("'", "''"))
                    .arg(splitExtractDirEscaped.replace("'", "''"));
            ps.start("powershell.exe", psArgs);
            if (!ps.waitForFinished(300000) || ps.exitCode() != 0) {
                continue; // Skip this split APK if extraction fails
            }
        }
#else
        QProcess proc;
        QStringList unzipArgs;
        unzipArgs << "-o" << splitApk << "-d" << splitExtractDir;
        proc.start("unzip", unzipArgs);
        if (!proc.waitForFinished(300000) || proc.exitCode() != 0) {
            continue;
        }
#endif
        
        // Copy contents from split to base (excluding META-INF and AndroidManifest.xml)
        QDirIterator it(splitExtractDir, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString srcPath = it.next();
            QString relativePath = srcPath.mid(splitExtractDir.length() + 1);
            
            // Skip META-INF and AndroidManifest.xml from splits
            if (relativePath.startsWith("META-INF") || relativePath == "AndroidManifest.xml") {
                continue;
            }
            
            QString destPath = baseExtractDir + "/" + relativePath;
            QFileInfo srcInfo(srcPath);
            
            if (srcInfo.isDir()) {
                dir.mkpath(destPath);
            } else {
                // Create parent directory if needed
                QFileInfo destInfo(destPath);
                dir.mkpath(destInfo.absolutePath());
                
                // Copy file (overwrite if exists)
                if (QFile::exists(destPath)) {
                    QFile::remove(destPath);
                }
                QFile::copy(srcPath, destPath);
            }
        }
        
        splitProgress += progressPerSplit;
    }
    
    // Remove signature files from merged APK (they will be invalid after modification)
    emit mergeProgress(70, tr("Removing old signatures..."));
    QString metaInfDir = baseExtractDir + "/META-INF";
    if (QDir(metaInfDir).exists()) {
        QDir(metaInfDir).removeRecursively();
    }
    
    // Create output APK using jar command
    emit mergeProgress(75, tr("Creating merged APK..."));
    
    // Ensure output directory exists
    QFileInfo outputInfo(outputApk);
    dir.mkpath(outputInfo.absolutePath());
    
    // Remove existing output file
    if (QFile::exists(outputApk)) {
        QFile::remove(outputApk);
    }
    
#ifdef Q_OS_WIN
    QString jarExe3;
    QFileInfo javaInfo3(java);
    jarExe3 = javaInfo3.absolutePath() + "/jar.exe";
    
    if (QFile::exists(jarExe3)) {
        QProcess proc;
        proc.setWorkingDirectory(baseExtractDir);
        QStringList jarArgs;
        jarArgs << "-cfM" << QDir::toNativeSeparators(outputApk) << ".";
        proc.start(jarExe3, jarArgs);
        if (!proc.waitForFinished(300000) || proc.exitCode() != 0) {
#ifdef QT_DEBUG
            qDebug() << "jar failed:" << proc.readAllStandardError();
#endif
            // Try PowerShell Compress-Archive
            QProcess ps;
            QStringList psArgs;
            psArgs << "-NoProfile" << "-Command";
            QString outputApkEscaped = outputApk;
            QString baseExtractDirEscaped = baseExtractDir;
            psArgs << QString("Compress-Archive -Path '%1\\*' -DestinationPath '%2' -Force")
                    .arg(baseExtractDirEscaped.replace("'", "''"))
                    .arg(outputApkEscaped.replace("'", "''").replace(".apk", ".zip"));
            ps.start("powershell.exe", psArgs);
            if (ps.waitForFinished(300000) && ps.exitCode() == 0) {
                // Rename .zip to .apk
                QString zipPath = outputApk;
                zipPath.replace(".apk", ".zip");
                QFile::rename(zipPath, outputApk);
            } else {
                return false;
            }
        }
    } else {
        // Use PowerShell Compress-Archive
        QProcess ps;
        QStringList psArgs;
        psArgs << "-NoProfile" << "-Command";
        QString outputApkEscaped = outputApk;
        QString baseExtractDirEscaped = baseExtractDir;
        QString zipPath = outputApk;
        zipPath.replace(".apk", ".zip");
        psArgs << QString("Compress-Archive -Path '%1\\*' -DestinationPath '%2' -Force")
                .arg(baseExtractDirEscaped.replace("'", "''"))
                .arg(zipPath.replace("'", "''"));
        ps.start("powershell.exe", psArgs);
        if (ps.waitForFinished(300000) && ps.exitCode() == 0) {
            // Rename .zip to .apk
            QFile::rename(zipPath, outputApk);
        } else {
            return false;
        }
    }
#else
    // Use zip on Unix
    QProcess proc;
    proc.setWorkingDirectory(baseExtractDir);
    QStringList zipArgs;
    zipArgs << "-r" << "-0" << outputApk << ".";
    proc.start("zip", zipArgs);
    if (!proc.waitForFinished(300000) || proc.exitCode() != 0) {
        return false;
    }
#endif
    
    return QFile::exists(outputApk);
}

bool AntiSplitWorker::signOutputApk(const QString &apkPath)
{
    QString java = ProcessUtils::javaExe();
    QString uas = ProcessUtils::uberApkSignerJar();
    
    if (java.isEmpty() || uas.isEmpty()) {
#ifdef QT_DEBUG
        qDebug() << "Java or Uber APK Signer not configured";
#endif
        return false;
    }
    
    QSettings settings;
    QString keystore = settings.value("signing_keystore").toString();
    QString keystorePass = settings.value("signing_keystore_password").toString();
    QString alias = settings.value("signing_alias").toString();
    QString aliasPass = settings.value("signing_alias_password").toString();
    
    QString heap("-Xmx%1m");
    heap = heap.arg(QString::number(ProcessUtils::javaHeapSize()));
    
    QStringList args;
    args << heap << "-jar" << uas;
    args << "--apks" << QDir::toNativeSeparators(apkPath);
    
    if (!keystore.isEmpty() && QFile::exists(keystore)) {
        args << "--ks" << QDir::toNativeSeparators(keystore);
        if (!keystorePass.isEmpty()) {
            args << "--ksPass" << keystorePass;
        }
        if (!alias.isEmpty()) {
            args << "--ksAlias" << alias;
            if (!aliasPass.isEmpty()) {
                args << "--ksKeyPass" << aliasPass;
            }
        }
    }
    
    if (settings.value("signing_zipalign", true).toBool()) {
        args << "--zipalign";
    }
    
    args << "--overwrite";
    
    ProcessResult result = ProcessUtils::runCommand(java, args, 600);
    
    return result.code == 0;
}
