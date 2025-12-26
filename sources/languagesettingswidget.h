#ifndef LANGUAGESETTINGSWIDGET_H
#define LANGUAGESETTINGSWIDGET_H

#include <QComboBox>
#include <QLabel>
#include <QMap>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>

class LanguageSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LanguageSettingsWidget(QWidget *parent = nullptr);
    void save();
    
    static QMap<QString, QString> getSupportedLanguages();
    static QString getCurrentLanguage();
    static void setCurrentLanguage(const QString &langCode);
    static QString translate(const QString &key);
    static void loadTranslations(const QString &langCode);
    
signals:
    void languageChanged(const QString &langCode);
    
private slots:
    void onLanguageSelected(int index);
    void onAddCustomLanguage();
    void onEditTranslation();
    void onExportTranslations();
    void onImportTranslations();
    
private:
    void populateLanguages();
    void saveCustomTranslation(const QString &langCode, const QString &key, const QString &value);
    
    QComboBox *m_LanguageCombo;
    QLabel *m_InfoLabel;
    QTableWidget *m_TranslationsTable;
    QPushButton *m_AddLanguageBtn;
    QPushButton *m_EditBtn;
    QPushButton *m_ExportBtn;
    QPushButton *m_ImportBtn;
    
    static QMap<QString, QMap<QString, QString>> s_Translations;
    static QString s_CurrentLanguage;
};

#endif // LANGUAGESETTINGSWIDGET_H
