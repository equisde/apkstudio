#include "securitysettingswidget.h"
#include <QSettings>
#include <QFormLayout>
#include <QLabel>

SecuritySettingsWidget::SecuritySettingsWidget(QWidget *parent) : QWidget(parent)
{
    auto layout = new QVBoxLayout(this);
    QSettings settings;

    auto group = new QGroupBox(tr("Auto-Security Engine"));
    auto form = new QFormLayout(group);

    m_CheckAutoSSLUnpin = new QCheckBox(tr("Automatic SSL Unpinning during decompile"));
    m_CheckAutoSSLUnpin->setChecked(settings.value("sec_auto_unpin", true).toBool());
    form->addRow(m_CheckAutoSSLUnpin);

    m_CheckDetectTampering = new QCheckBox(tr("Scan for Anti-Tamper logic on open"));
    m_CheckDetectTampering->setChecked(settings.value("sec_scan_tamper", true).toBool());
    form->addRow(m_CheckDetectTampering);

    m_CheckHideRoot = new QCheckBox(tr("Auto-inject Root Detection Bypass"));
    m_CheckHideRoot->setChecked(settings.value("sec_hide_root", false).toBool());
    form->addRow(m_CheckHideRoot);

    m_EditCustomCertPath = new QLineEdit();
    m_EditCustomCertPath->setPlaceholderText(tr("/path/to/custom/cert.pem"));
    m_EditCustomCertPath->setText(settings.value("sec_custom_cert").toString());
    form->addRow(tr("Custom SSL Certificate:"), m_EditCustomCertPath);

    layout->addWidget(group);
    layout->addStretch();
}

void SecuritySettingsWidget::save()
{
    QSettings settings;
    settings.setValue("sec_auto_unpin", m_CheckAutoSSLUnpin->isChecked());
    settings.setValue("sec_scan_tamper", m_CheckDetectTampering->isChecked());
    settings.setValue("sec_hide_root", m_CheckHideRoot->isChecked());
    settings.setValue("sec_custom_cert", m_EditCustomCertPath->text());
}
