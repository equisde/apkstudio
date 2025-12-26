#ifndef APKANALYSISTOOLS_H
#define APKANALYSISTOOLS_H

#include <QCheckBox>
#include <QDialog>
#include <QJsonObject>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QTextBrowser>
#include <QTextEdit>
#include <QTreeWidget>
#include <QWidget>

// APK Info Dialog - Shows package info, version, permissions
class ApkInfoDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ApkInfoDialog(const QString &projectPath, QWidget *parent = nullptr);
    
private:
    void loadApkInfo();
    void parseManifest();
    
    QString m_ProjectPath;
    QTableWidget *m_InfoTable;
    QListWidget *m_PermissionsList;
    QTreeWidget *m_ComponentsTree;
};

// Permission Analyzer - Analyzes and explains permissions
class PermissionAnalyzerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PermissionAnalyzerDialog(const QString &projectPath, QWidget *parent = nullptr);
    
private:
    struct PermissionInfo {
        QString name;
        QString description;
        QString riskLevel; // LOW, MEDIUM, HIGH, DANGEROUS
        QString category;
    };
    
    void analyzePermissions();
    PermissionInfo getPermissionInfo(const QString &permission);
    
    QString m_ProjectPath;
    QTableWidget *m_PermTable;
    QTextBrowser *m_Details;
};

// String Search Dialog - Search across all files
class StringSearchDialog : public QDialog
{
    Q_OBJECT
public:
    explicit StringSearchDialog(const QString &projectPath, QWidget *parent = nullptr);
    
signals:
    void fileSelected(const QString &path, int line);
    
private slots:
    void performSearch();
    void handleResultClicked(QTableWidgetItem *item);
    
private:
    QString m_ProjectPath;
    QLineEdit *m_SearchInput;
    QCheckBox *m_CaseSensitive;
    QCheckBox *m_Regex;
    QCheckBox *m_WholeWord;
    QTableWidget *m_Results;
    QProgressBar *m_Progress;
};

// API Key Finder - Finds hardcoded API keys, secrets
class ApiKeyFinderDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ApiKeyFinderDialog(const QString &projectPath, QWidget *parent = nullptr);
    
signals:
    void fileSelected(const QString &path, int line);
    
private slots:
    void startScan();
    
private:
    void scanFile(const QString &path);
    
    QString m_ProjectPath;
    QTableWidget *m_Results;
    QProgressBar *m_Progress;
    
    QStringList m_Patterns; // Regex patterns for API keys
};

// Native Library Inspector - Inspect .so files
class NativeLibraryDialog : public QDialog
{
    Q_OBJECT
public:
    explicit NativeLibraryDialog(const QString &projectPath, QWidget *parent = nullptr);
    
private:
    void scanLibraries();
    void analyzeLibrary(const QString &path);
    
    QString m_ProjectPath;
    QTreeWidget *m_LibTree;
    QTextBrowser *m_Details;
};

// APK Size Analyzer
class ApkSizeAnalyzerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ApkSizeAnalyzerDialog(const QString &projectPath, QWidget *parent = nullptr);
    
private:
    void analyzeSize();
    QString formatSize(qint64 bytes);
    
    QString m_ProjectPath;
    QTreeWidget *m_SizeTree;
    QProgressBar *m_UsageBar;
};

// Smali Patcher - Find and replace in smali
class SmaliPatcherDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SmaliPatcherDialog(const QString &projectPath, QWidget *parent = nullptr);
    
private slots:
    void findOccurrences();
    void replaceAll();
    void replaceSelected();
    
private:
    QString m_ProjectPath;
    QLineEdit *m_FindInput;
    QLineEdit *m_ReplaceInput;
    QCheckBox *m_CaseSensitive;
    QCheckBox *m_Regex;
    QTableWidget *m_Results;
};

// Resource String Editor
class StringResourceEditorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit StringResourceEditorDialog(const QString &projectPath, QWidget *parent = nullptr);
    
private slots:
    void loadStrings();
    void saveStrings();
    void addString();
    void removeString();
    
private:
    QString m_ProjectPath;
    QTableWidget *m_StringsTable;
    QComboBox *m_LanguageCombo;
};

// Hardcoded URL/IP Finder
class HardcodedFinderDialog : public QDialog
{
    Q_OBJECT
public:
    explicit HardcodedFinderDialog(const QString &projectPath, QWidget *parent = nullptr);
    
signals:
    void fileSelected(const QString &path, int line);
    
private slots:
    void startScan();
    
private:
    QString m_ProjectPath;
    QTableWidget *m_Results;
    QCheckBox *m_FindUrls;
    QCheckBox *m_FindIps;
    QCheckBox *m_FindEmails;
};

// Firebase Config Extractor
class FirebaseExtractorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FirebaseExtractorDialog(const QString &projectPath, QWidget *parent = nullptr);
    
private:
    void extractConfig();
    
    QString m_ProjectPath;
    QTableWidget *m_ConfigTable;
};

// Android TV Optimizer
class AndroidTVOptimizerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AndroidTVOptimizerDialog(const QString &projectPath, QWidget *parent = nullptr);
    
private slots:
    void analyzeForTV();
    void addLeanbackSupport();
    void addBannerIcon();
    void optimizeForTV();
    
private:
    QString m_ProjectPath;
    QListWidget *m_CheckList;
    QTextBrowser *m_Recommendations;
};

// ADB Logcat Viewer
class LogcatViewerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LogcatViewerDialog(const QString &packageName, QWidget *parent = nullptr);
    
private slots:
    void startLogcat();
    void stopLogcat();
    void clearLog();
    void filterChanged();
    
private:
    QString m_PackageName;
    QPlainTextEdit *m_LogView;
    QLineEdit *m_FilterInput;
    QComboBox *m_LevelCombo;
    QProcess *m_LogcatProcess;
};

// Certificate Info Viewer
class CertificateInfoDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CertificateInfoDialog(const QString &apkPath, QWidget *parent = nullptr);
    
private:
    void extractCertInfo();
    
    QString m_ApkPath;
    QTableWidget *m_CertTable;
};

// APK Comparison Tool
class ApkCompareDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ApkCompareDialog(QWidget *parent = nullptr);
    
private slots:
    void selectApk1();
    void selectApk2();
    void compare();
    
private:
    QString m_Apk1Path;
    QString m_Apk2Path;
    QTreeWidget *m_DiffTree;
};

// Obfuscation Detector
class ObfuscationDetectorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ObfuscationDetectorDialog(const QString &projectPath, QWidget *parent = nullptr);
    
private:
    void detectObfuscation();
    
    QString m_ProjectPath;
    QTableWidget *m_Results;
    QProgressBar *m_ObfuscationLevel;
};

// SSL Pinning Analyzer & Unpinner
class SSLPinningDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SSLPinningDialog(const QString &projectPath, QWidget *parent = nullptr);
    
private slots:
    void analyzePinning();
    void unpinAll();
    void unpinSelected();
    void addCustomCert();
    void exportCertTemplate();
    
private:
    struct PinningLocation {
        QString filePath;
        int lineNumber;
        QString pinType; // "certificate", "publicKey", "hash", "okhttp", "trustmanager"
        QString originalCode;
        QString description;
        bool canUnpin;
    };
    
    void searchForPinning();
    void searchSmaliFiles();
    void searchJavaFiles();
    void searchNetworkConfig();
    bool unpinLocation(const PinningLocation &location);
    QString generateTrustAllManager();
    QString generateNetworkSecurityConfig();
    void createFridaScript();
    
    QString m_ProjectPath;
    QList<PinningLocation> m_PinningLocations;
    QTableWidget *m_ResultsTable;
    QTextBrowser *m_DetailsView;
    QPushButton *m_UnpinAllBtn;
    QPushButton *m_UnpinSelectedBtn;
    QPushButton *m_AddCertBtn;
    QProgressBar *m_Progress;
};

// Certificate Injector
class CertificateInjectorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CertificateInjectorDialog(const QString &projectPath, QWidget *parent = nullptr);
    
private slots:
    void selectCertificate();
    void injectCertificate();
    void generateSelfSigned();
    void previewChanges();
    
private:
    void modifyNetworkSecurityConfig();
    void addCertToResources();
    void patchTrustManager();
    
    QString m_ProjectPath;
    QString m_CertPath;
    QLineEdit *m_CertPathEdit;
    QTextEdit *m_PreviewEdit;
    QCheckBox *m_ModifyConfigCheck;
    QCheckBox *m_PatchTrustManagerCheck;
    QCheckBox *m_CreateFridaScriptCheck;
};

#endif // APKANALYSISTOOLS_H
