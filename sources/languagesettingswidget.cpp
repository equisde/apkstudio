#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>
#include <QVBoxLayout>
#include "languagesettingswidget.h"

// Static members
QMap<QString, QMap<QString, QString>> LanguageSettingsWidget::s_Translations;
QString LanguageSettingsWidget::s_CurrentLanguage = "en";

// Built-in translations
static const QMap<QString, QString> LANGUAGE_NAMES = {
    {"en", "English (Technical)"},
    {"es", "Español (Coloquial)"},
    {"cl", "Chileno (Flaite/Vio)"},
    {"pt", "Português (Gíria)"},
    {"hack", "Hackerman (Leet)"},
    {"fr", "Français"},
    {"de", "Deutsch"},
    {"it", "Italiano"}
};

// English translations (base)
static const QMap<QString, QString> EN_TRANSLATIONS = {
    {"app_name", "APK Studio Pro"},
    {"open_apk", "Feed me an APK"},
    {"decompile", "Dissect the beast"},
    {"recompile", "Sew it back"},
    {"ready", "Ready for action"},
    {"settings", "My Configs"},
    {"ai_assistant", "AI Oracle"},
    {"ssl_unpinner", "SSL Killer"},
    {"project_explorer", "The Snitch"}
};

// Chilean translations (Flaite)
static const QMap<QString, QString> CL_TRANSLATIONS = {
    {"app_name", "APK Studio pal' Corte"},
    {"open_apk", "Suelta el APK po ctm"},
    {"settings", "Mis weás"},
    {"decompile", "Destripando el bicho"},
    {"recompile", "Armando el cuento"},
    {"ready", "Tamo' ready weon"},
    {"ai_assistant", "El Vio de la IA"},
    {"ssl_unpinner", "Bypass de los Pacos"},
    {"project_explorer", "El Sapo"},
    {"sign_apk", "Ponerle el sello"},
    {"install", "Meter pal' celu"}
};

// Spanish translations (Colloquial)
static const QMap<QString, QString> ES_TRANSLATIONS = {
    {"app_name", "APK Studio Pro"},
    {"open_apk", "Suéltame ese APK"},
    {"settings", "Mis Chanchullos"},
    {"decompile", "Abrir en canal"},
    {"recompile", "Volver a coser"},
    {"ready", "Al pie del cañón"},
    {"ai_assistant", "El Oráculo IA"},
    {"ssl_unpinner", "Romper Candados SSL"},
    {"project_explorer", "El Chivato"},
    {"terminal", "La Consola"},
    {"sign_apk", "Ponerle el Sello"},
    {"install", "Meter en el Móvil"}
};

// Portuguese translations (Colloquial)
static const QMap<QString, QString> PT_TRANSLATIONS = {
    {"app_name", "APK Studio Pro"},
    {"open_apk", "Manda o APK pra cá"},
    {"settings", "Minhas Gambiarras"},
    {"decompile", "Desmontar o Brinquedo"},
    {"recompile", "Montar de Novo"},
    {"ready", "Na atividade, mestre"},
    {"ai_assistant", "Mestre da IA"},
    {"ssl_unpinner", "Bypass de SSL"},
    {"project_explorer", "X9"},
    {"sign_apk", "Dar a Assinatura"},
    {"install", "Instalar no Aparelho"}
};

// Hackerman (Leet)
static const QMap<QString, QString> HACK_TRANSLATIONS = {
    {"app_name", "4PK 57UD10 PR0"},
    {"open_apk", "L04D 4PK"},
    {"settings", "C0NF1G"},
    {"decompile", "D3C0MP1L3"},
    {"recompile", "R3C0MP1L3"},
    {"ready", "PWN3D"},
    {"ai_assistant", "AI_G0D"},
    {"ssl_unpinner", "SSL_KILL3R"}
};

LanguageSettingsWidget::LanguageSettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    // Initialize translations
    s_Translations["en"] = EN_TRANSLATIONS;
    s_Translations["cl"] = CL_TRANSLATIONS;
    s_Translations["es"] = ES_TRANSLATIONS;
    s_Translations["pt"] = PT_TRANSLATIONS;
    s_Translations["hack"] = HACK_TRANSLATIONS;
    
    // Language selection group
    auto langGroup = new QGroupBox(tr("Language Selection"), this);
    auto langLayout = new QVBoxLayout(langGroup);
    
    auto selectLayout = new QHBoxLayout();
    selectLayout->addWidget(new QLabel(tr("Application Language:"), this));
    
    m_LanguageCombo = new QComboBox(this);
    populateLanguages();
    connect(m_LanguageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &LanguageSettingsWidget::onLanguageSelected);
    selectLayout->addWidget(m_LanguageCombo, 1);
    
    langLayout->addLayout(selectLayout);
    
    m_InfoLabel = new QLabel(this);
    m_InfoLabel->setStyleSheet("color: #808080; font-size: 11px;");
    m_InfoLabel->setWordWrap(true);
    langLayout->addWidget(m_InfoLabel);
    
    layout->addWidget(langGroup);
    layout->addStretch();
    
    // Load current language
    QSettings settings;
    s_CurrentLanguage = settings.value("language", "en").toString();
    int index = m_LanguageCombo->findData(s_CurrentLanguage);
    if (index >= 0) {
        m_LanguageCombo->setCurrentIndex(index);
    }
}

void LanguageSettingsWidget::populateLanguages()
{
    m_LanguageCombo->clear();
    for (auto it = LANGUAGE_NAMES.constBegin(); it != LANGUAGE_NAMES.constEnd(); ++it) {
        m_LanguageCombo->addItem(it.value(), it.key());
    }
}

void LanguageSettingsWidget::onLanguageSelected(int index)
{
    QString langCode = m_LanguageCombo->itemData(index).toString();
    QSettings settings;
    if (settings.value("language", "en").toString() == langCode) {
        return;
    }

    settings.setValue("language", langCode);
    settings.sync();
    s_CurrentLanguage = langCode;
    
    int btn = QMessageBox::information(this,
                                       tr("Language"),
                                       tr("Application language changed. You need to restart APK Studio for changes to take effect."),
                                       tr("Restart"),
                                       tr("OK"));
    if (btn == 0) {
        qApp->exit(60600); // CODE_RESTART
    }
}

void LanguageSettingsWidget::save() {}
void LanguageSettingsWidget::onAddCustomLanguage() {}
void LanguageSettingsWidget::onEditTranslation() {}
void LanguageSettingsWidget::onExportTranslations() {}
void LanguageSettingsWidget::onImportTranslations() {}

QMap<QString, QString> LanguageSettingsWidget::getSupportedLanguages() { return LANGUAGE_NAMES; }
QString LanguageSettingsWidget::getCurrentLanguage() { return s_CurrentLanguage; }
void LanguageSettingsWidget::setCurrentLanguage(const QString &langCode) { s_CurrentLanguage = langCode; }

QString LanguageSettingsWidget::translate(const QString &key)
{
    if (s_Translations.contains(s_CurrentLanguage)) {
        const auto &trans = s_Translations[s_CurrentLanguage];
        if (trans.contains(key)) return trans[key];
    }
    return EN_TRANSLATIONS.value(key, key);
}

void LanguageSettingsWidget::loadTranslations(const QString &langCode) { s_CurrentLanguage = langCode; }
void LanguageSettingsWidget::saveCustomTranslation(const QString &, const QString &, const QString &) {}