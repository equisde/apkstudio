#include "binarysettingswidget.h"
#include "tooldownloaddialog.h"
#include "tooldownloadworker.h"
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QSettings>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

BinarySettingsWidget::BinarySettingsWidget(QWidget *parent) : QWidget(parent)
{
    auto mainLayout = new QVBoxLayout(this);
    QSettings settings;

    auto group = new QGroupBox(tr("🛠️ Toolchain Binaries (Managed by AI)"));
    auto form = new QFormLayout(group);

    auto addToolRow = [&](const QString &label, const QString &settingKey, const QString &placeholder) {
        auto layout = new QHBoxLayout();
        auto edit = new QLineEdit(settings.value(settingKey).toString());
        edit->setPlaceholderText(placeholder);
        layout->addWidget(edit);
        
        auto btn = new QPushButton(tr("Browse"));
        connect(btn, &QPushButton::clicked, [this, edit, settingKey]() {
            QString path = QFileDialog::getOpenFileName(this, tr("Select Binary"));
            if (!path.isEmpty()) {
                edit->setText(path);
                QSettings().setValue(settingKey, path);
            }
        });
        layout->addWidget(btn);
        form->addRow(label, layout);
    };

    addToolRow("Java Executable:", "java_exe", "java.exe");
    addToolRow("Apktool Jar:", "apktool_jar", "apktool.jar");
    addToolRow("JADX Executable:", "jadx_exe", "jadx.bat");
    addToolRow("ILSpyCmd:", "ilspy_cmd", "ilspycmd.exe");
    addToolRow("Mono Compiler (mcs):", "mono_mcs_exe", "mcs.exe");
    addToolRow("Il2CppDumper:", "il2cpp_dumper_exe", "Il2CppDumper.exe");
    addToolRow("Uber APK Signer:", "uas_jar", "uber-apk-signer.jar");

    mainLayout->addWidget(group);

    // BOTÓN DE DESCARGA MASIVA
    auto btnDownloadAll = new QPushButton(tr("🚀 Download & Configure All Missing Tools"));
    btnDownloadAll->setStyleSheet("background-color: #238636; color: white; font-weight: bold; padding: 15px; border-radius: 8px;");
    connect(btnDownloadAll, &QPushButton::clicked, this, &BinarySettingsWidget::downloadAllTools);
    mainLayout->addWidget(btnDownloadAll);

    mainLayout->addStretch();
}

void BinarySettingsWidget::downloadAllTools()
{
    // Usar el diálogo de descarga existente para bajar todo en secuencia
    QList<ToolDownloadWorker::ToolType> tools = {
        ToolDownloadWorker::Java,
        ToolDownloadWorker::Apktool,
        ToolDownloadWorker::Jadx,
        ToolDownloadWorker::ILSpyCmd,
        ToolDownloadWorker::Mono,
        ToolDownloadWorker::UberApkSigner
    };
    
    auto dialog = new ToolDownloadDialog(tools, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void BinarySettingsWidget::save() {
    // Los cambios se guardan al seleccionar o mediante el botón de la ventana de ajustes
}