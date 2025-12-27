#ifndef UNIVERSALREVERSER_H
#define UNIVERSALREVERSER_H

#include <QObject>
#include <QStringList>
#include <QSettings>

class UniversalReverser : public QObject
{
    Q_OBJECT
public:
    explicit UniversalReverser(const QString &projectPath, QObject *parent = nullptr);
    
    void autoDecompileAll();
    void decompileDlls();
    void decompileDexToJava();
    void decompileNatives();

signals:
    void progress(int percent, const QString &msg);
    void finished();

private:
    void checkAndDownloadTool(int toolType);
    QString m_ProjectPath;
};

#endif
