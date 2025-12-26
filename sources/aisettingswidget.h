#ifndef AISETTINGSWIDGET_H
#define AISETTINGSWIDGET_H

#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QWidget>

class AISettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AISettingsWidget(QWidget *parent = nullptr);
    void save();
    
private slots:
    void checkCliStatus();
    void installCliAgent();
    
private:
    QComboBox *m_ComboProvider;
    QLineEdit *m_EditApiKey;
    QLineEdit *m_EditModel;
    QCheckBox *m_CheckAutoAnalyze;
    QCheckBox *m_CheckEnabled;
    QCheckBox *m_CheckUseCliAgent;
    QLabel *m_CliStatusLabel;
    QPushButton *m_InstallCliButton;
    QPushButton *m_CheckCliButton;
};

#endif // AISETTINGSWIDGET_H
