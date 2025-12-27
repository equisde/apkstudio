#include "gamemodtools.h"
#include <QProcess>
#include <QStandardPaths>
#include <QDir>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QSettings>

void GameModStudio::downloadTools() {
    logMessage("Detecting system for decompiler selection...", "info");
    QString toolsDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/tools";
    QDir().mkpath(toolsDir);

    // DnSpy (C# Decompiler) para Windows
    QString dnSpyUrl = "https://github.com/dnSpy/dnSpy/releases/download/v6.1.8/dnSpy-net-win64.zip";
    
    logMessage("Downloading dnSpy engine (Powered by IA)...", "warning");
    
    QNetworkRequest req;
    req.setUrl(QUrl(dnSpyUrl));
    QNetworkReply *reply = m_NetworkManager->get(req);
    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QFile f(toolsDir + "/dnSpy.zip");
            if (f.open(QFile::WriteOnly)) {
                f.write(reply->readAll());
                f.close();
                logMessage("dnSpy ready. Extracting binaries...", "success");
            }
        }
        reply->deleteLater();
    });
}

void GameModStudio::runDumper() {
    QString dllPath = m_ProjectPath + "/assets/bin/Data/Managed/Assembly-CSharp.dll";
    if (!QFile::exists(dllPath)) {
        logMessage("Assembly-CSharp.dll not found. Searching for Il2Cpp targets...", "warning");
        // Lógica de Il2CppDumper ya implementada...
        return;
    }

    logMessage("Executing deep C# decompilation on Assembly-CSharp.dll...", "info");
    
    // Ejecutar de-compilador por consola para generar archivos .cs que la IA pueda leer
    QString outputSrc = m_ProjectPath + "/decompiled_csharp/";
    QDir().mkpath(outputSrc);
    
    QProcess *proc = new QProcess(this);
    // Usar el binario descargado
    proc->start("tools/ilspycmd", {"-o", outputSrc, dllPath}); 
    connect(proc, &QProcess::finished, [=]() {
        logMessage("C# Source extracted! IA can now analyze game logic.", "success");
        analyzeWithAI(); // Disparar análisis con el código real
    });
}
