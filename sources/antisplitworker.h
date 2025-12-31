#ifndef ANTISPLITWORKER_H
#define ANTISPLITWORKER_H

#include <QObject>
#include <QStringList>

class AntiSplitWorker : public QObject
{
    Q_OBJECT
public:
    explicit AntiSplitWorker(const QStringList &inputFiles, const QString &outputFile, const bool signApk, 
                             const QString &targetArch = "all", QObject *parent = nullptr);
    void merge();
private:
    QStringList m_InputFiles;
    QString m_OutputFile;
    bool m_SignApk;
    QString m_TargetArch;
    
    bool extractXapk(const QString &xapkPath, const QString &extractDir);
    bool mergeApks(const QStringList &apkFiles, const QString &workDir, const QString &outputApk);
    bool signOutputApk(const QString &apkPath);
    void cleanManifestSplitAttributes(const QString &extractDir);
    void filterArchitectures(const QString &extractDir);
signals:
    void mergeFailed(const QString &error);
    void mergeFinished(const QString &outputFile);
    void mergeProgress(const int percent, const QString &message);
    void finished();
    void started();
};

#endif // ANTISPLITWORKER_H
