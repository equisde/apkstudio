#ifndef SECURITYSETTINGSWIDGET_H
#define SECURITYSETTINGSWIDGET_H

#include <QWidget>
#include <QCheckBox>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QGroupBox>

class SecuritySettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SecuritySettingsWidget(QWidget *parent = nullptr);
    void save();

private:
    QCheckBox *m_CheckAutoSSLUnpin;
    QCheckBox *m_CheckDetectTampering;
    QCheckBox *m_CheckHideRoot;
    QLineEdit *m_EditCustomCertPath;
};

#endif // SECURITYSETTINGSWIDGET_H
