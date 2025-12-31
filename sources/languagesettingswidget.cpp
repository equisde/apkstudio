#include <QApplication>
#include <QDateTime>
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
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTableWidget>
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
    
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("Language"));
    msgBox.setText(tr("Application language changed. You need to restart APK Studio for changes to take effect."));
    QPushButton *restartBtn = msgBox.addButton(tr("Restart"), QMessageBox::AcceptRole);
    msgBox.addButton(QMessageBox::Ok);
    msgBox.exec();
    
    if (msgBox.clickedButton() == (QAbstractButton *)restartBtn) {
        qApp->exit(60600); // CODE_RESTART
    }
}

void LanguageSettingsWidget::save()
{
    QSettings settings;
    QString langCode = m_LanguageCombo->currentData().toString();
    settings.setValue("language", langCode);
    settings.sync();
    s_CurrentLanguage = langCode;
}

void LanguageSettingsWidget::onAddCustomLanguage()
{
    QString langCode = QInputDialog::getText(this, tr("Add Custom Language"),
        tr("Enter language code (e.g., 'fr', 'de', 'ja'):"));
    
    if (langCode.isEmpty()) return;
    
    if (s_Translations.contains(langCode)) {
        QMessageBox::warning(this, tr("Language Exists"),
            tr("Language '%1' already exists.").arg(langCode));
        return;
    }
    
    QString langName = QInputDialog::getText(this, tr("Language Name"),
        tr("Enter display name for '%1':").arg(langCode));
    
    if (langName.isEmpty()) langName = langCode.toUpper();
    
    // Create empty translation map based on English
    s_Translations[langCode] = EN_TRANSLATIONS;
    
    // Save to file
    QString customPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/translations";
    QDir().mkpath(customPath);
    
    QFile file(customPath + "/" + langCode + ".json");
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject obj;
        obj["code"] = langCode;
        obj["name"] = langName;
        QJsonObject translations;
        for (auto it = EN_TRANSLATIONS.constBegin(); it != EN_TRANSLATIONS.constEnd(); ++it) {
            translations[it.key()] = it.value();
        }
        obj["translations"] = translations;
        file.write(QJsonDocument(obj).toJson());
        file.close();
    }
    
    // Add to combo
    m_LanguageCombo->addItem(langName, langCode);
    m_LanguageCombo->setCurrentIndex(m_LanguageCombo->count() - 1);
    
    QMessageBox::information(this, tr("Language Added"),
        tr("Language '%1' added. You can now edit its translations.").arg(langName));
}

void LanguageSettingsWidget::onEditTranslation()
{
    QString langCode = m_LanguageCombo->currentData().toString();
    
    if (!s_Translations.contains(langCode)) {
        QMessageBox::warning(this, tr("Edit Translation"),
            tr("No translations found for '%1'.").arg(langCode));
        return;
    }
    
    // Create a simple editor dialog
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Edit Translations - %1").arg(langCode));
    dialog.setMinimumSize(600, 400);
    
    auto layout = new QVBoxLayout(&dialog);
    auto table = new QTableWidget(&dialog);
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels({tr("Key"), tr("Translation")});
    table->horizontalHeader()->setStretchLastSection(true);
    
    const auto &trans = s_Translations[langCode];
    table->setRowCount(trans.size());
    
    int row = 0;
    for (auto it = trans.constBegin(); it != trans.constEnd(); ++it, ++row) {
        auto keyItem = new QTableWidgetItem(it.key());
        keyItem->setFlags(keyItem->flags() & ~Qt::ItemIsEditable);
        table->setItem(row, 0, keyItem);
        table->setItem(row, 1, new QTableWidgetItem(it.value()));
    }
    
    layout->addWidget(table);
    
    auto btnBox = new QHBoxLayout();
    auto saveBtn = new QPushButton(tr("Save"), &dialog);
    auto cancelBtn = new QPushButton(tr("Cancel"), &dialog);
    btnBox->addStretch();
    btnBox->addWidget(saveBtn);
    btnBox->addWidget(cancelBtn);
    layout->addLayout(btnBox);
    
    connect(saveBtn, &QPushButton::clicked, &dialog, [&]() {
        for (int r = 0; r < table->rowCount(); ++r) {
            QString key = table->item(r, 0)->text();
            QString value = table->item(r, 1)->text();
            saveCustomTranslation(langCode, key, value);
        }
        dialog.accept();
    });
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    dialog.exec();
}

void LanguageSettingsWidget::onExportTranslations()
{
    QString langCode = m_LanguageCombo->currentData().toString();
    
    QString filePath = QFileDialog::getSaveFileName(this, tr("Export Translations"),
        QDir::homePath() + "/" + langCode + "_translations.json",
        "JSON Files (*.json)");
    
    if (filePath.isEmpty()) return;
    
    if (!s_Translations.contains(langCode)) {
        QMessageBox::warning(this, tr("Export"), tr("No translations to export."));
        return;
    }
    
    QJsonObject obj;
    obj["code"] = langCode;
    obj["exported_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QJsonObject translations;
    const auto &trans = s_Translations[langCode];
    for (auto it = trans.constBegin(); it != trans.constEnd(); ++it) {
        translations[it.key()] = it.value();
    }
    obj["translations"] = translations;
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        file.close();
        QMessageBox::information(this, tr("Export Complete"),
            tr("Translations exported to:\n%1").arg(filePath));
    } else {
        QMessageBox::critical(this, tr("Export Failed"),
            tr("Could not write to file: %1").arg(filePath));
    }
}

void LanguageSettingsWidget::onImportTranslations()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Import Translations"),
        QDir::homePath(), "JSON Files (*.json)");
    
    if (filePath.isEmpty()) return;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Import Failed"),
            tr("Could not read file: %1").arg(filePath));
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (!doc.isObject()) {
        QMessageBox::critical(this, tr("Import Failed"), tr("Invalid JSON format."));
        return;
    }
    
    QJsonObject obj = doc.object();
    QString langCode = obj["code"].toString();
    
    if (langCode.isEmpty()) {
        QMessageBox::critical(this, tr("Import Failed"), tr("No language code found in file."));
        return;
    }
    
    QJsonObject translations = obj["translations"].toObject();
    QMap<QString, QString> transMap;
    
    for (auto it = translations.constBegin(); it != translations.constEnd(); ++it) {
        transMap[it.key()] = it.value().toString();
    }
    
    s_Translations[langCode] = transMap;
    
    // Check if language exists in combo, if not add it
    int idx = m_LanguageCombo->findData(langCode);
    if (idx < 0) {
        QString langName = obj.value("name").toString(langCode.toUpper());
        m_LanguageCombo->addItem(langName, langCode);
    }
    
    QMessageBox::information(this, tr("Import Complete"),
        tr("Imported %1 translations for '%2'.").arg(transMap.size()).arg(langCode));
}

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

void LanguageSettingsWidget::saveCustomTranslation(const QString &langCode, const QString &key, const QString &value)
{
    if (!s_Translations.contains(langCode)) {
        s_Translations[langCode] = QMap<QString, QString>();
    }
    s_Translations[langCode][key] = value;
    
    // Persist to disk
    QString customPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/translations";
    QDir().mkpath(customPath);
    
    QFile file(customPath + "/" + langCode + ".json");
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject obj;
        obj["code"] = langCode;
        QJsonObject translations;
        const auto &trans = s_Translations[langCode];
        for (auto it = trans.constBegin(); it != trans.constEnd(); ++it) {
            translations[it.key()] = it.value();
        }
        obj["translations"] = translations;
        file.write(QJsonDocument(obj).toJson());
        file.close();
    }
}