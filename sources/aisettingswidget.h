#ifndef AISETTINGSWIDGET_H
#define AISETTINGSWIDGET_H

#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QTextEdit>
#include <QWidget>

struct EnvironmentStatus {
    bool nvmAvailable = false;
    bool nodeAvailable = false;
    bool npmAvailable = false;
    bool geminiCliAvailable = false;
    bool copilotCliAvailable = false;
    bool ghAvailable = false;
    QString nvmPath;
    QString nodeVersion;
    QString npmVersion;
    QString geminiVersion;
    QString copilotVersion;
    QString recommendedNodeVersion = "v22.0.0"; // LTS recommended for CLI agents
};

class AISettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AISettingsWidget(QWidget *parent = nullptr);
    void save();
    
private slots:
    void checkCliStatus();
    void installCliAgent();
    void installNodeViaWeb();
    void installNvm();
    void onProviderChanged(int index);
    void runSmartDetection();
    void showDetailedStatus();
    
private:
    // Detection methods
    EnvironmentStatus detectFullEnvironment();
    QString detectNvmPath();
    QString detectNodePath();
    bool checkCommandAvailable(const QString &command, const QStringList &args, QString &output);
    bool installPackageWithNpm(const QString &package, bool global = true);
    void updateStatusDisplay(const EnvironmentStatus &status);
    QString getInstallCommand(const QString &provider);
    QString getCliCommand(const QString &provider);
    
    // UI Elements
    QComboBox *m_ComboProvider;
    QLineEdit *m_EditApiKey;
    QLineEdit *m_EditModel;
    QCheckBox *m_CheckAutoAnalyze;
    QCheckBox *m_CheckAutoFix;
    QCheckBox *m_CheckEnabled;
    QCheckBox *m_CheckUseCliAgent;
    QComboBox *m_ComboSlang;
    QLabel *m_CliStatusLabel;
    QTextEdit *m_DetailedStatus;
    QPushButton *m_InstallCliButton;
    QPushButton *m_CheckCliButton;
    QPushButton *m_InstallNodeButton;
    QPushButton *m_InstallNvmButton;
    QPushButton *m_SmartDetectButton;
    QProgressBar *m_ProgressBar;
    
    // State
    EnvironmentStatus m_EnvStatus;
    bool m_IsInstalling;
};

#endif // AISETTINGSWIDGET_H
