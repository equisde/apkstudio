#ifndef APKRECOMPILEWORKER_H
#define APKRECOMPILEWORKER_H

#include <QObject>
#include <QProcess>

class ApkRecompileWorker : public QObject
{
    Q_OBJECT
public:
    explicit ApkRecompileWorker(const QString &folder, bool aapt2, const QString &extraArguments = QString(), QObject *parent = nullptr);
    void recompile();
    
private:
    void applyAiPatches(const QString &soPath);
    bool compileModMenu();
    bool compileNativeLibraries();
    void injectModLoader();
    QString findNDKPath();
    bool buildDobbyIfNeeded();
    void copyModLibraryToApk();
    
    bool m_Aapt2;
    QString m_Folder;
    QString m_ExtraArguments;
    QProcess *m_BuildProcess;
    
signals:
    void finished();
    void progress(int percent, const QString &message);
    void recompileFailed(const QString &folder);
    void recompileFinished(const QString &folder);
    void started();
};

#endif // APKRECOMPILEWORKER_H
