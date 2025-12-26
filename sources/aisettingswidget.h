#ifndef AISETTINGSWIDGET_H
#define AISETTINGSWIDGET_H

#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QWidget>

class AISettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AISettingsWidget(QWidget *parent = nullptr);
    void save();
private:
    QComboBox *m_ComboProvider;
    QLineEdit *m_EditApiKey;
    QLineEdit *m_EditModel;
    QCheckBox *m_CheckAutoAnalyze;
    QCheckBox *m_CheckEnabled;
};

#endif // AISETTINGSWIDGET_H
