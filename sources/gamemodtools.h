#ifndef GAMEMODTOOLS_H
#define GAMEMODTOOLS_H

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QNetworkAccessManager>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextBrowser>
#include <QTreeWidget>

// Unity Game Analyzer & Modder
class UnityGameDialog : public QDialog
{
    Q_OBJECT
public:
    explicit UnityGameDialog(const QString &projectPath, QWidget *parent = nullptr);
    
private slots:
    void detectUnityVersion();
    void analyzeAssets();
    void extractIl2cppMetadata();
    void decompileIl2cpp();
    void modifyGameValues();
    void dumpAssembly();
    void patchAssembly();
    void aiAnalyzeGame();
    void aiSuggestMods();
    
private:
    struct UnityAsset {
        QString name;
        QString type;
        QString path;
        qint64 size;
        bool extractable;
    };
    
    struct Il2CppClass {
        QString name;
        QString namespaceName;
        QStringList methods;
        QStringList fields;
    };
    
    bool isUnityGame();
    bool hasIl2cpp();
    bool hasMono();
    void scanAssetBundles();
    void parseGlobalMetadata();
    void findGameValues();
    QString getUnityVersion();
    void askAI(const QString &prompt, std::function<void(const QString&)> callback);
    
    QString m_ProjectPath;
    QString m_UnityVersion;
    bool m_IsIl2cpp;
    QList<UnityAsset> m_Assets;
    QList<Il2CppClass> m_Classes;
    
    QLabel *m_VersionLabel;
    QLabel *m_TypeLabel;
    QTreeWidget *m_AssetsTree;
    QTableWidget *m_ClassesTable;
    QTextBrowser *m_DetailsView;
    QProgressBar *m_Progress;
    QPlainTextEdit *m_LogView;
    QPushButton *m_DecompileBtn;
    QPushButton *m_ModifyBtn;
    QPushButton *m_AiAnalyzeBtn;
    QNetworkAccessManager *m_NetworkManager;
};

// Flutter App Analyzer & Decompiler
class FlutterAnalyzerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FlutterAnalyzerDialog(const QString &projectPath, QWidget *parent = nullptr);
    
private slots:
    void detectFlutter();
    void analyzeLibflutter();
    void extractDartSnapshot();
    void decompileSnapshot();
    void findWidgets();
    void extractAssets();
    void aiAnalyzeFlutter();
    void aiSuggestPatches();
    
private:
    struct DartFunction {
        QString name;
        QString className;
        quint64 offset;
        qint64 size;
    };
    
    bool isFlutterApp();
    QString getFlutterVersion();
    void parseDartSnapshot();
    void findLibflutter();
    void extractFlutterAssets();
    void askAI(const QString &prompt, std::function<void(const QString&)> callback);
    
    QString m_ProjectPath;
    QString m_FlutterVersion;
    QString m_DartVersion;
    QString m_LibflutterPath;
    QList<DartFunction> m_Functions;
    
    QLabel *m_FlutterVersionLabel;
    QLabel *m_DartVersionLabel;
    QTreeWidget *m_AssetsTree;
    QTableWidget *m_FunctionsTable;
    QTextBrowser *m_DetailsView;
    QProgressBar *m_Progress;
    QPlainTextEdit *m_LogView;
    QNetworkAccessManager *m_NetworkManager;
};

// Native Library Analyzer (for .so files)
class NativeLibAnalyzerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit NativeLibAnalyzerDialog(const QString &libPath, QWidget *parent = nullptr);
    
private slots:
    void analyzeLib();
    void extractSymbols();
    void findStrings();
    void hexDump();
    void disassemble();
    void patchBytes();
    void aiAnalyzeLib();
    
private:
    struct Symbol {
        QString name;
        QString type;
        quint64 address;
        qint64 size;
        bool exported;
    };
    
    void parseElfHeader();
    void parseSymbolTable();
    void findInterestingPatterns();
    void askAI(const QString &prompt, std::function<void(const QString&)> callback);
    
    QString m_LibPath;
    QByteArray m_LibData;
    QList<Symbol> m_Symbols;
    QStringList m_Strings;
    
    QTableWidget *m_SymbolsTable;
    QTableWidget *m_StringsTable;
    QPlainTextEdit *m_HexView;
    QPlainTextEdit *m_DisasmView;
    QTextBrowser *m_DetailsView;
    QProgressBar *m_Progress;
    QNetworkAccessManager *m_NetworkManager;
};

// Game Value Editor (for modifying game values)
class GameValueEditorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit GameValueEditorDialog(const QString &projectPath, QWidget *parent = nullptr);
    
private slots:
    void scanForValues();
    void modifyValue();
    void saveChanges();
    void revertChanges();
    void aiSuggestValues();
    
private:
    struct GameValue {
        QString name;
        QString file;
        int line;
        QString originalValue;
        QString currentValue;
        QString type; // int, float, string, boolean
        QString context;
    };
    
    void scanSmaliForValues();
    void scanXmlForValues();
    void scanJsonForValues();
    void scanAssetsForValues();
    void askAI(const QString &prompt, std::function<void(const QString&)> callback);
    
    QString m_ProjectPath;
    QList<GameValue> m_Values;
    
    QTableWidget *m_ValuesTable;
    QLineEdit *m_SearchInput;
    QComboBox *m_TypeFilter;
    QTextBrowser *m_PreviewView;
    QPushButton *m_ModifyBtn;
    QPushButton *m_SaveBtn;
    QNetworkAccessManager *m_NetworkManager;
};

// Cocos2d-x Game Analyzer
class Cocos2dxAnalyzerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit Cocos2dxAnalyzerDialog(const QString &projectPath, QWidget *parent = nullptr);
    
private slots:
    void detectCocos();
    void analyzeScripts();
    void extractResources();
    void decryptLua();
    void modifyLua();
    void aiAnalyzeCocos();
    
private:
    bool isCocosGame();
    QString getCocosVersion();
    void findLuaScripts();
    void findJsScripts();
    void decryptLuaScript(const QString &path);
    void askAI(const QString &prompt, std::function<void(const QString&)> callback);
    
    QString m_ProjectPath;
    QString m_CocosVersion;
    bool m_UsesLua;
    bool m_UsesJs;
    QStringList m_ScriptFiles;
    
    QLabel *m_VersionLabel;
    QTreeWidget *m_ResourcesTree;
    QTableWidget *m_ScriptsTable;
    QPlainTextEdit *m_ScriptView;
    QProgressBar *m_Progress;
    QNetworkAccessManager *m_NetworkManager;
};

// Unreal Engine Game Analyzer
class UnrealAnalyzerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit UnrealAnalyzerDialog(const QString &projectPath, QWidget *parent = nullptr);
    
private slots:
    void detectUnreal();
    void analyzePakFiles();
    void extractAssets();
    void findBlueprints();
    void modifyConfig();
    void aiAnalyzeUnreal();
    
private:
    bool isUnrealGame();
    QString getUnrealVersion();
    void parsePakFile(const QString &path);
    void findConfigFiles();
    void askAI(const QString &prompt, std::function<void(const QString&)> callback);
    
    QString m_ProjectPath;
    QString m_UnrealVersion;
    QStringList m_PakFiles;
    
    QLabel *m_VersionLabel;
    QTreeWidget *m_AssetsTree;
    QTableWidget *m_ConfigTable;
    QTextBrowser *m_DetailsView;
    QProgressBar *m_Progress;
    QNetworkAccessManager *m_NetworkManager;
};

// React Native Analyzer
class ReactNativeAnalyzerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ReactNativeAnalyzerDialog(const QString &projectPath, QWidget *parent = nullptr);
    
private slots:
    void detectReactNative();
    void analyzeBundle();
    void extractComponents();
    void modifyBundle();
    void aiAnalyzeRN();
    
private:
    bool isReactNativeApp();
    QString getRNVersion();
    void parseJsBundle();
    void findComponents();
    void askAI(const QString &prompt, std::function<void(const QString&)> callback);
    
    QString m_ProjectPath;
    QString m_RNVersion;
    QString m_BundlePath;
    QStringList m_Components;
    
    QLabel *m_VersionLabel;
    QTreeWidget *m_ComponentsTree;
    QPlainTextEdit *m_BundleView;
    QProgressBar *m_Progress;
    QNetworkAccessManager *m_NetworkManager;
};

// Game Engine Auto-Detector
class GameEngineDetector
{
public:
    enum Engine {
        Unknown,
        Unity,
        UnrealEngine,
        Cocos2dx,
        Flutter,
        ReactNative,
        Godot,
        LibGDX,
        Cordova,
        Xamarin,
        NativeAndroid
    };
    
    static Engine detectEngine(const QString &projectPath);
    static QString engineName(Engine engine);
    static QStringList getEngineFiles(Engine engine);
    static bool supportsDecompilation(Engine engine);
};

#endif // GAMEMODTOOLS_H
