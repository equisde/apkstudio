#ifndef ANTISPLITDIALOG_H
#define ANTISPLITDIALOG_H

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>

class AntiSplitDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AntiSplitDialog(QWidget *parent = nullptr);
    QStringList inputFiles() const;
    QString outputFile() const;
    bool signApk() const;
    QString targetArchitecture() const;
private:
    QDialogButtonBox *m_ButtonBox;
    QListWidget *m_ListFiles;
    QLineEdit *m_EditOutput;
    QCheckBox *m_CheckSign;
    QComboBox *m_ComboArch;
    QPushButton *m_BtnAddApk;
    QPushButton *m_BtnAddXapk;
    QPushButton *m_BtnRemove;
    QPushButton *m_BtnClear;
    QWidget *buildButtonBox();
    QLayout *buildForm();
private slots:
    void handleAddApk();
    void handleAddXapk();
    void handleRemoveSelected();
    void handleClearAll();
    void handleBrowseOutput();
    void updateButtons();
};

#endif // ANTISPLITDIALOG_H
