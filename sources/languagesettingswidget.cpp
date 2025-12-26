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
    {"en", "English"},
    {"es", "Español"},
    {"pt", "Português"},
    {"fr", "Français"},
    {"de", "Deutsch"},
    {"it", "Italiano"},
    {"ja", "日本語"},
    {"ko", "한국어"},
    {"zh", "中文"},
    {"ru", "Русский"},
    {"ar", "العربية"}
};

// English translations (base)
static const QMap<QString, QString> EN_TRANSLATIONS = {
    {"app_name", "APK Studio"},
    {"file", "File"},
    {"edit", "Edit"},
    {"view", "View"},
    {"build", "Build"},
    {"tools", "Tools"},
    {"help", "Help"},
    {"open_apk", "Open APK..."},
    {"open_folder", "Open Folder..."},
    {"save", "Save"},
    {"save_all", "Save All"},
    {"close", "Close"},
    {"close_all", "Close All"},
    {"exit", "Exit"},
    {"undo", "Undo"},
    {"redo", "Redo"},
    {"cut", "Cut"},
    {"copy", "Copy"},
    {"paste", "Paste"},
    {"find", "Find..."},
    {"replace", "Replace..."},
    {"goto_line", "Go to Line..."},
    {"settings", "Settings"},
    {"project_explorer", "Project Explorer"},
    {"terminal", "Terminal"},
    {"ai_assistant", "AI Assistant"},
    {"build_debug", "Build (Debug)"},
    {"build_release", "Build (Release)"},
    {"sign_apk", "Sign APK"},
    {"install", "Install to Device"},
    {"decompile", "Decompile"},
    {"recompile", "Recompile"},
    {"antisplit", "AntiSplit (Merge APKs)"},
    {"ssl_unpinner", "SSL Unpinner"},
    {"certificate_injector", "Certificate Injector"},
    {"permission_analyzer", "Permission Analyzer"},
    {"string_search", "String Search"},
    {"apk_info", "APK Info"},
    {"about", "About"},
    {"documentation", "Documentation"},
    {"language", "Language"},
    {"appearance", "Appearance"},
    {"binaries", "Binaries"},
    {"signing", "Signing"},
    {"ai_settings", "AI Assistant"},
    {"analyze_project", "Analyze Project"},
    {"clear", "Clear"},
    {"send", "Send"},
    {"no_project", "No project loaded"},
    {"analyzing", "Analyzing..."},
    {"analysis_complete", "Analysis complete"},
    {"error", "Error"},
    {"success", "Success"},
    {"warning", "Warning"},
    {"cancel", "Cancel"},
    {"ok", "OK"},
    {"apply", "Apply"},
    {"browse", "Browse..."},
    {"select_language", "Select Language"},
    {"restart_required", "Restart Required"},
    {"restart_message", "Please restart the application for language changes to take effect."},
    {"add_custom_language", "Add Custom Language"},
    {"export_translations", "Export Translations"},
    {"import_translations", "Import Translations"},
    {"edit_translations", "Edit Translations"},
    {"auto_open_last_project", "Auto-open last project on startup"}
};

// Spanish translations
static const QMap<QString, QString> ES_TRANSLATIONS = {
    {"app_name", "APK Studio"},
    {"file", "Archivo"},
    {"edit", "Editar"},
    {"view", "Ver"},
    {"build", "Compilar"},
    {"tools", "Herramientas"},
    {"help", "Ayuda"},
    {"open_apk", "Abrir APK..."},
    {"open_folder", "Abrir Carpeta..."},
    {"save", "Guardar"},
    {"save_all", "Guardar Todo"},
    {"close", "Cerrar"},
    {"close_all", "Cerrar Todo"},
    {"exit", "Salir"},
    {"undo", "Deshacer"},
    {"redo", "Rehacer"},
    {"cut", "Cortar"},
    {"copy", "Copiar"},
    {"paste", "Pegar"},
    {"find", "Buscar..."},
    {"replace", "Reemplazar..."},
    {"goto_line", "Ir a Línea..."},
    {"settings", "Configuración"},
    {"project_explorer", "Explorador de Proyecto"},
    {"terminal", "Terminal"},
    {"ai_assistant", "Asistente IA"},
    {"build_debug", "Compilar (Debug)"},
    {"build_release", "Compilar (Release)"},
    {"sign_apk", "Firmar APK"},
    {"install", "Instalar en Dispositivo"},
    {"decompile", "Decompilar"},
    {"recompile", "Recompilar"},
    {"antisplit", "AntiSplit (Unir APKs)"},
    {"ssl_unpinner", "Desbloquear SSL"},
    {"certificate_injector", "Inyector de Certificados"},
    {"permission_analyzer", "Analizador de Permisos"},
    {"string_search", "Buscar Cadenas"},
    {"apk_info", "Información APK"},
    {"about", "Acerca de"},
    {"documentation", "Documentación"},
    {"language", "Idioma"},
    {"appearance", "Apariencia"},
    {"binaries", "Binarios"},
    {"signing", "Firma"},
    {"ai_settings", "Asistente IA"},
    {"analyze_project", "Analizar Proyecto"},
    {"clear", "Limpiar"},
    {"send", "Enviar"},
    {"no_project", "Ningún proyecto cargado"},
    {"analyzing", "Analizando..."},
    {"analysis_complete", "Análisis completo"},
    {"error", "Error"},
    {"success", "Éxito"},
    {"warning", "Advertencia"},
    {"cancel", "Cancelar"},
    {"ok", "Aceptar"},
    {"apply", "Aplicar"},
    {"browse", "Explorar..."},
    {"select_language", "Seleccionar Idioma"},
    {"restart_required", "Reinicio Requerido"},
    {"restart_message", "Por favor reinicie la aplicación para que los cambios de idioma surtan efecto."},
    {"add_custom_language", "Agregar Idioma Personalizado"},
    {"export_translations", "Exportar Traducciones"},
    {"import_translations", "Importar Traducciones"},
    {"edit_translations", "Editar Traducciones"},
    {"auto_open_last_project", "Abrir último proyecto automáticamente al iniciar"}
};

// Portuguese translations
static const QMap<QString, QString> PT_TRANSLATIONS = {
    {"app_name", "APK Studio"},
    {"file", "Arquivo"},
    {"edit", "Editar"},
    {"view", "Visualizar"},
    {"build", "Compilar"},
    {"tools", "Ferramentas"},
    {"help", "Ajuda"},
    {"open_apk", "Abrir APK..."},
    {"open_folder", "Abrir Pasta..."},
    {"save", "Salvar"},
    {"save_all", "Salvar Tudo"},
    {"close", "Fechar"},
    {"close_all", "Fechar Tudo"},
    {"exit", "Sair"},
    {"undo", "Desfazer"},
    {"redo", "Refazer"},
    {"cut", "Cortar"},
    {"copy", "Copiar"},
    {"paste", "Colar"},
    {"find", "Buscar..."},
    {"replace", "Substituir..."},
    {"goto_line", "Ir para Linha..."},
    {"settings", "Configurações"},
    {"project_explorer", "Explorador de Projeto"},
    {"terminal", "Terminal"},
    {"ai_assistant", "Assistente IA"},
    {"build_debug", "Compilar (Debug)"},
    {"build_release", "Compilar (Release)"},
    {"sign_apk", "Assinar APK"},
    {"install", "Instalar no Dispositivo"},
    {"decompile", "Decompilar"},
    {"recompile", "Recompilar"},
    {"antisplit", "AntiSplit (Juntar APKs)"},
    {"ssl_unpinner", "Desbloquear SSL"},
    {"certificate_injector", "Injetor de Certificados"},
    {"permission_analyzer", "Analisador de Permissões"},
    {"string_search", "Buscar Strings"},
    {"apk_info", "Informações do APK"},
    {"about", "Sobre"},
    {"documentation", "Documentação"},
    {"language", "Idioma"},
    {"appearance", "Aparência"},
    {"binaries", "Binários"},
    {"signing", "Assinatura"},
    {"ai_settings", "Assistente IA"},
    {"analyze_project", "Analisar Projeto"},
    {"clear", "Limpar"},
    {"send", "Enviar"},
    {"no_project", "Nenhum projeto carregado"},
    {"analyzing", "Analisando..."},
    {"analysis_complete", "Análise completa"},
    {"error", "Erro"},
    {"success", "Sucesso"},
    {"warning", "Aviso"},
    {"cancel", "Cancelar"},
    {"ok", "OK"},
    {"apply", "Aplicar"},
    {"browse", "Procurar..."},
    {"select_language", "Selecionar Idioma"},
    {"restart_required", "Reinício Necessário"},
    {"restart_message", "Por favor reinicie o aplicativo para que as alterações de idioma tenham efeito."},
    {"add_custom_language", "Adicionar Idioma Personalizado"},
    {"export_translations", "Exportar Traduções"},
    {"import_translations", "Importar Traduções"},
    {"edit_translations", "Editar Traduções"},
    {"auto_open_last_project", "Abrir último projeto automaticamente ao iniciar"}
};

// French translations
static const QMap<QString, QString> FR_TRANSLATIONS = {
    {"app_name", "APK Studio"},
    {"file", "Fichier"},
    {"edit", "Édition"},
    {"view", "Affichage"},
    {"build", "Compiler"},
    {"tools", "Outils"},
    {"help", "Aide"},
    {"open_apk", "Ouvrir APK..."},
    {"open_folder", "Ouvrir Dossier..."},
    {"save", "Enregistrer"},
    {"save_all", "Tout Enregistrer"},
    {"close", "Fermer"},
    {"close_all", "Tout Fermer"},
    {"exit", "Quitter"},
    {"undo", "Annuler"},
    {"redo", "Rétablir"},
    {"cut", "Couper"},
    {"copy", "Copier"},
    {"paste", "Coller"},
    {"find", "Rechercher..."},
    {"replace", "Remplacer..."},
    {"goto_line", "Aller à la Ligne..."},
    {"settings", "Paramètres"},
    {"project_explorer", "Explorateur de Projet"},
    {"terminal", "Terminal"},
    {"ai_assistant", "Assistant IA"},
    {"build_debug", "Compiler (Debug)"},
    {"build_release", "Compiler (Release)"},
    {"sign_apk", "Signer APK"},
    {"install", "Installer sur Appareil"},
    {"decompile", "Décompiler"},
    {"recompile", "Recompiler"},
    {"antisplit", "AntiSplit (Fusionner APKs)"},
    {"ssl_unpinner", "Débloquer SSL"},
    {"certificate_injector", "Injecteur de Certificats"},
    {"permission_analyzer", "Analyseur de Permissions"},
    {"string_search", "Rechercher Chaînes"},
    {"apk_info", "Info APK"},
    {"about", "À propos"},
    {"documentation", "Documentation"},
    {"language", "Langue"},
    {"appearance", "Apparence"},
    {"binaries", "Binaires"},
    {"signing", "Signature"},
    {"ai_settings", "Assistant IA"},
    {"analyze_project", "Analyser Projet"},
    {"clear", "Effacer"},
    {"send", "Envoyer"},
    {"no_project", "Aucun projet chargé"},
    {"analyzing", "Analyse en cours..."},
    {"analysis_complete", "Analyse terminée"},
    {"error", "Erreur"},
    {"success", "Succès"},
    {"warning", "Avertissement"},
    {"cancel", "Annuler"},
    {"ok", "OK"},
    {"apply", "Appliquer"},
    {"browse", "Parcourir..."},
    {"select_language", "Sélectionner Langue"},
    {"restart_required", "Redémarrage Requis"},
    {"restart_message", "Veuillez redémarrer l'application pour que les changements de langue prennent effet."},
    {"add_custom_language", "Ajouter Langue Personnalisée"},
    {"export_translations", "Exporter Traductions"},
    {"import_translations", "Importer Traductions"},
    {"edit_translations", "Modifier Traductions"},
    {"auto_open_last_project", "Ouvrir automatiquement le dernier projet au démarrage"}
};

// German translations
static const QMap<QString, QString> DE_TRANSLATIONS = {
    {"app_name", "APK Studio"},
    {"file", "Datei"},
    {"edit", "Bearbeiten"},
    {"view", "Ansicht"},
    {"build", "Erstellen"},
    {"tools", "Werkzeuge"},
    {"help", "Hilfe"},
    {"open_apk", "APK Öffnen..."},
    {"open_folder", "Ordner Öffnen..."},
    {"save", "Speichern"},
    {"save_all", "Alle Speichern"},
    {"close", "Schließen"},
    {"close_all", "Alle Schließen"},
    {"exit", "Beenden"},
    {"undo", "Rückgängig"},
    {"redo", "Wiederholen"},
    {"cut", "Ausschneiden"},
    {"copy", "Kopieren"},
    {"paste", "Einfügen"},
    {"find", "Suchen..."},
    {"replace", "Ersetzen..."},
    {"goto_line", "Gehe zu Zeile..."},
    {"settings", "Einstellungen"},
    {"project_explorer", "Projekt-Explorer"},
    {"terminal", "Terminal"},
    {"ai_assistant", "KI-Assistent"},
    {"build_debug", "Erstellen (Debug)"},
    {"build_release", "Erstellen (Release)"},
    {"sign_apk", "APK Signieren"},
    {"install", "Auf Gerät Installieren"},
    {"decompile", "Dekompilieren"},
    {"recompile", "Rekompilieren"},
    {"antisplit", "AntiSplit (APKs Zusammenführen)"},
    {"ssl_unpinner", "SSL Entsperren"},
    {"certificate_injector", "Zertifikat-Injektor"},
    {"permission_analyzer", "Berechtigungs-Analysator"},
    {"string_search", "String-Suche"},
    {"apk_info", "APK-Info"},
    {"about", "Über"},
    {"documentation", "Dokumentation"},
    {"language", "Sprache"},
    {"appearance", "Erscheinungsbild"},
    {"binaries", "Binärdateien"},
    {"signing", "Signierung"},
    {"ai_settings", "KI-Assistent"},
    {"analyze_project", "Projekt Analysieren"},
    {"clear", "Löschen"},
    {"send", "Senden"},
    {"no_project", "Kein Projekt geladen"},
    {"analyzing", "Analysiere..."},
    {"analysis_complete", "Analyse abgeschlossen"},
    {"error", "Fehler"},
    {"success", "Erfolg"},
    {"warning", "Warnung"},
    {"cancel", "Abbrechen"},
    {"ok", "OK"},
    {"apply", "Anwenden"},
    {"browse", "Durchsuchen..."},
    {"select_language", "Sprache Auswählen"},
    {"restart_required", "Neustart Erforderlich"},
    {"restart_message", "Bitte starten Sie die Anwendung neu, damit die Sprachänderungen wirksam werden."},
    {"add_custom_language", "Benutzerdefinierte Sprache Hinzufügen"},
    {"export_translations", "Übersetzungen Exportieren"},
    {"import_translations", "Übersetzungen Importieren"},
    {"edit_translations", "Übersetzungen Bearbeiten"},
    {"auto_open_last_project", "Letztes Projekt beim Start automatisch öffnen"}
};

// Italian translations
static const QMap<QString, QString> IT_TRANSLATIONS = {
    {"app_name", "APK Studio"},
    {"file", "File"},
    {"edit", "Modifica"},
    {"view", "Visualizza"},
    {"build", "Compila"},
    {"tools", "Strumenti"},
    {"help", "Aiuto"},
    {"open_apk", "Apri APK..."},
    {"open_folder", "Apri Cartella..."},
    {"save", "Salva"},
    {"save_all", "Salva Tutto"},
    {"close", "Chiudi"},
    {"close_all", "Chiudi Tutto"},
    {"exit", "Esci"},
    {"undo", "Annulla"},
    {"redo", "Ripeti"},
    {"cut", "Taglia"},
    {"copy", "Copia"},
    {"paste", "Incolla"},
    {"find", "Trova..."},
    {"replace", "Sostituisci..."},
    {"goto_line", "Vai alla Riga..."},
    {"settings", "Impostazioni"},
    {"project_explorer", "Esplora Progetto"},
    {"terminal", "Terminale"},
    {"ai_assistant", "Assistente IA"},
    {"build_debug", "Compila (Debug)"},
    {"build_release", "Compila (Release)"},
    {"sign_apk", "Firma APK"},
    {"install", "Installa su Dispositivo"},
    {"decompile", "Decompila"},
    {"recompile", "Ricompila"},
    {"antisplit", "AntiSplit (Unisci APK)"},
    {"ssl_unpinner", "Sblocca SSL"},
    {"certificate_injector", "Iniettore Certificati"},
    {"permission_analyzer", "Analizzatore Permessi"},
    {"string_search", "Cerca Stringhe"},
    {"apk_info", "Info APK"},
    {"about", "Informazioni"},
    {"documentation", "Documentazione"},
    {"language", "Lingua"},
    {"appearance", "Aspetto"},
    {"binaries", "Binari"},
    {"signing", "Firma"},
    {"ai_settings", "Assistente IA"},
    {"analyze_project", "Analizza Progetto"},
    {"clear", "Pulisci"},
    {"send", "Invia"},
    {"no_project", "Nessun progetto caricato"},
    {"analyzing", "Analisi in corso..."},
    {"analysis_complete", "Analisi completata"},
    {"error", "Errore"},
    {"success", "Successo"},
    {"warning", "Avviso"},
    {"cancel", "Annulla"},
    {"ok", "OK"},
    {"apply", "Applica"},
    {"browse", "Sfoglia..."},
    {"select_language", "Seleziona Lingua"},
    {"restart_required", "Riavvio Necessario"},
    {"restart_message", "Riavvia l'applicazione per applicare le modifiche alla lingua."},
    {"add_custom_language", "Aggiungi Lingua Personalizzata"},
    {"export_translations", "Esporta Traduzioni"},
    {"import_translations", "Importa Traduzioni"},
    {"edit_translations", "Modifica Traduzioni"},
    {"auto_open_last_project", "Apri automaticamente l'ultimo progetto all'avvio"}
};

// Japanese translations
static const QMap<QString, QString> JA_TRANSLATIONS = {
    {"app_name", "APK Studio"},
    {"file", "ファイル"},
    {"edit", "編集"},
    {"view", "表示"},
    {"build", "ビルド"},
    {"tools", "ツール"},
    {"help", "ヘルプ"},
    {"open_apk", "APKを開く..."},
    {"open_folder", "フォルダを開く..."},
    {"save", "保存"},
    {"save_all", "すべて保存"},
    {"close", "閉じる"},
    {"close_all", "すべて閉じる"},
    {"exit", "終了"},
    {"undo", "元に戻す"},
    {"redo", "やり直し"},
    {"cut", "切り取り"},
    {"copy", "コピー"},
    {"paste", "貼り付け"},
    {"find", "検索..."},
    {"replace", "置換..."},
    {"goto_line", "行へ移動..."},
    {"settings", "設定"},
    {"project_explorer", "プロジェクトエクスプローラー"},
    {"terminal", "ターミナル"},
    {"ai_assistant", "AIアシスタント"},
    {"build_debug", "ビルド (デバッグ)"},
    {"build_release", "ビルド (リリース)"},
    {"sign_apk", "APK署名"},
    {"install", "デバイスにインストール"},
    {"decompile", "逆コンパイル"},
    {"recompile", "再コンパイル"},
    {"antisplit", "AntiSplit (APK統合)"},
    {"ssl_unpinner", "SSLピン留め解除"},
    {"certificate_injector", "証明書インジェクター"},
    {"permission_analyzer", "権限アナライザー"},
    {"string_search", "文字列検索"},
    {"apk_info", "APK情報"},
    {"about", "について"},
    {"documentation", "ドキュメント"},
    {"language", "言語"},
    {"appearance", "外観"},
    {"binaries", "バイナリ"},
    {"signing", "署名"},
    {"ai_settings", "AIアシスタント"},
    {"analyze_project", "プロジェクト分析"},
    {"clear", "クリア"},
    {"send", "送信"},
    {"no_project", "プロジェクトが読み込まれていません"},
    {"analyzing", "分析中..."},
    {"analysis_complete", "分析完了"},
    {"error", "エラー"},
    {"success", "成功"},
    {"warning", "警告"},
    {"cancel", "キャンセル"},
    {"ok", "OK"},
    {"apply", "適用"},
    {"browse", "参照..."},
    {"select_language", "言語を選択"},
    {"restart_required", "再起動が必要です"},
    {"restart_message", "言語の変更を適用するにはアプリケーションを再起動してください。"},
    {"add_custom_language", "カスタム言語を追加"},
    {"export_translations", "翻訳をエクスポート"},
    {"import_translations", "翻訳をインポート"},
    {"edit_translations", "翻訳を編集"},
    {"auto_open_last_project", "起動時に最後のプロジェクトを自動的に開く"}
};

// Korean translations
static const QMap<QString, QString> KO_TRANSLATIONS = {
    {"app_name", "APK Studio"},
    {"file", "파일"},
    {"edit", "편집"},
    {"view", "보기"},
    {"build", "빌드"},
    {"tools", "도구"},
    {"help", "도움말"},
    {"open_apk", "APK 열기..."},
    {"open_folder", "폴더 열기..."},
    {"save", "저장"},
    {"save_all", "모두 저장"},
    {"close", "닫기"},
    {"close_all", "모두 닫기"},
    {"exit", "종료"},
    {"undo", "실행 취소"},
    {"redo", "다시 실행"},
    {"cut", "잘라내기"},
    {"copy", "복사"},
    {"paste", "붙여넣기"},
    {"find", "찾기..."},
    {"replace", "바꾸기..."},
    {"goto_line", "줄 이동..."},
    {"settings", "설정"},
    {"project_explorer", "프로젝트 탐색기"},
    {"terminal", "터미널"},
    {"ai_assistant", "AI 어시스턴트"},
    {"build_debug", "빌드 (디버그)"},
    {"build_release", "빌드 (릴리스)"},
    {"sign_apk", "APK 서명"},
    {"install", "기기에 설치"},
    {"decompile", "디컴파일"},
    {"recompile", "리컴파일"},
    {"antisplit", "AntiSplit (APK 병합)"},
    {"ssl_unpinner", "SSL 핀닝 해제"},
    {"certificate_injector", "인증서 주입기"},
    {"permission_analyzer", "권한 분석기"},
    {"string_search", "문자열 검색"},
    {"apk_info", "APK 정보"},
    {"about", "정보"},
    {"documentation", "문서"},
    {"language", "언어"},
    {"appearance", "모양"},
    {"binaries", "바이너리"},
    {"signing", "서명"},
    {"ai_settings", "AI 어시스턴트"},
    {"analyze_project", "프로젝트 분석"},
    {"clear", "지우기"},
    {"send", "보내기"},
    {"no_project", "로드된 프로젝트 없음"},
    {"analyzing", "분석 중..."},
    {"analysis_complete", "분석 완료"},
    {"error", "오류"},
    {"success", "성공"},
    {"warning", "경고"},
    {"cancel", "취소"},
    {"ok", "확인"},
    {"apply", "적용"},
    {"browse", "찾아보기..."},
    {"select_language", "언어 선택"},
    {"restart_required", "재시작 필요"},
    {"restart_message", "언어 변경을 적용하려면 응용 프로그램을 다시 시작하십시오."},
    {"add_custom_language", "사용자 정의 언어 추가"},
    {"export_translations", "번역 내보내기"},
    {"import_translations", "번역 가져오기"},
    {"edit_translations", "번역 편집"},
    {"auto_open_last_project", "시작 시 마지막 프로젝트 자동 열기"}
};

LanguageSettingsWidget::LanguageSettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    // Initialize translations
    s_Translations["en"] = EN_TRANSLATIONS;
    s_Translations["es"] = ES_TRANSLATIONS;
    s_Translations["pt"] = PT_TRANSLATIONS;
    s_Translations["fr"] = FR_TRANSLATIONS;
    s_Translations["de"] = DE_TRANSLATIONS;
    s_Translations["it"] = IT_TRANSLATIONS;
    s_Translations["ja"] = JA_TRANSLATIONS;
    s_Translations["ko"] = KO_TRANSLATIONS;
    
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
    
    // Custom languages group
    auto customGroup = new QGroupBox(tr("Custom Languages"), this);
    auto customLayout = new QVBoxLayout(customGroup);
    
    auto btnLayout = new QHBoxLayout();
    
    m_AddLanguageBtn = new QPushButton(tr("Add Language..."), this);
    connect(m_AddLanguageBtn, &QPushButton::clicked, this, &LanguageSettingsWidget::onAddCustomLanguage);
    btnLayout->addWidget(m_AddLanguageBtn);
    
    m_ImportBtn = new QPushButton(tr("Import..."), this);
    connect(m_ImportBtn, &QPushButton::clicked, this, &LanguageSettingsWidget::onImportTranslations);
    btnLayout->addWidget(m_ImportBtn);
    
    m_ExportBtn = new QPushButton(tr("Export..."), this);
    connect(m_ExportBtn, &QPushButton::clicked, this, &LanguageSettingsWidget::onExportTranslations);
    btnLayout->addWidget(m_ExportBtn);
    
    btnLayout->addStretch();
    customLayout->addLayout(btnLayout);
    
    layout->addWidget(customGroup);
    
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
    
    // Add any custom languages from settings
    QSettings settings;
    QStringList customLangs = settings.value("custom_languages").toStringList();
    for (const QString &lang : customLangs) {
        if (!LANGUAGE_NAMES.contains(lang)) {
            m_LanguageCombo->addItem(lang + " (Custom)", lang);
        }
    }
}

void LanguageSettingsWidget::onLanguageSelected(int index)
{
    QString langCode = m_LanguageCombo->itemData(index).toString();
    
    QSettings settings;
    settings.setValue("language", langCode);
    settings.sync();
    
    s_CurrentLanguage = langCode;
    
    m_InfoLabel->setText(tr("Language changed to %1. Please restart the application for changes to take effect.")
                         .arg(m_LanguageCombo->currentText()));
    
    emit languageChanged(langCode);
}

void LanguageSettingsWidget::onAddCustomLanguage()
{
    QString langCode = QInputDialog::getText(this, tr("Add Custom Language"),
        tr("Enter language code (e.g., 'pl' for Polish):"));
    
    if (langCode.isEmpty()) return;
    
    QString langName = QInputDialog::getText(this, tr("Add Custom Language"),
        tr("Enter language name:"));
    
    if (langName.isEmpty()) return;
    
    // Save to settings
    QSettings settings;
    QStringList customLangs = settings.value("custom_languages").toStringList();
    if (!customLangs.contains(langCode)) {
        customLangs.append(langCode);
        settings.setValue("custom_languages", customLangs);
        settings.setValue("custom_lang_name_" + langCode, langName);
    }
    
    // Start with English translations as base
    s_Translations[langCode] = EN_TRANSLATIONS;
    
    populateLanguages();
    
    QMessageBox::information(this, tr("Language Added"),
        tr("Language '%1' has been added. You can export and edit the translations, then import them back.").arg(langName));
}

void LanguageSettingsWidget::onEditTranslation()
{
    // This would open a dialog to edit translations
}

void LanguageSettingsWidget::onExportTranslations()
{
    QString langCode = m_LanguageCombo->currentData().toString();
    
    QString filePath = QFileDialog::getSaveFileName(this, tr("Export Translations"),
        QString("translations_%1.json").arg(langCode), tr("JSON Files (*.json)"));
    
    if (filePath.isEmpty()) return;
    
    QJsonObject json;
    const auto &trans = s_Translations.value(langCode, EN_TRANSLATIONS);
    for (auto it = trans.constBegin(); it != trans.constEnd(); ++it) {
        json[it.key()] = it.value();
    }
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
        file.close();
        QMessageBox::information(this, tr("Export Complete"),
            tr("Translations exported to %1").arg(filePath));
    }
}

void LanguageSettingsWidget::onImportTranslations()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Import Translations"),
        QString(), tr("JSON Files (*.json)"));
    
    if (filePath.isEmpty()) return;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Error"), tr("Could not open file."));
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (!doc.isObject()) {
        QMessageBox::warning(this, tr("Error"), tr("Invalid JSON format."));
        return;
    }
    
    QString langCode = m_LanguageCombo->currentData().toString();
    QJsonObject json = doc.object();
    
    QMap<QString, QString> trans;
    for (auto it = json.constBegin(); it != json.constEnd(); ++it) {
        trans[it.key()] = it.value().toString();
    }
    
    s_Translations[langCode] = trans;
    
    // Save to persistent storage
    QSettings settings;
    settings.beginGroup("translations_" + langCode);
    for (auto it = trans.constBegin(); it != trans.constEnd(); ++it) {
        settings.setValue(it.key(), it.value());
    }
    settings.endGroup();
    
    QMessageBox::information(this, tr("Import Complete"),
        tr("Translations imported successfully."));
}

QMap<QString, QString> LanguageSettingsWidget::getSupportedLanguages()
{
    QMap<QString, QString> langs = LANGUAGE_NAMES;
    
    // Add custom languages
    QSettings settings;
    QStringList customLangs = settings.value("custom_languages").toStringList();
    for (const QString &lang : customLangs) {
        QString name = settings.value("custom_lang_name_" + lang, lang).toString();
        langs[lang] = name;
    }
    
    return langs;
}

QString LanguageSettingsWidget::getCurrentLanguage()
{
    return s_CurrentLanguage;
}

void LanguageSettingsWidget::setCurrentLanguage(const QString &langCode)
{
    s_CurrentLanguage = langCode;
    QSettings settings;
    settings.setValue("language", langCode);
}

QString LanguageSettingsWidget::translate(const QString &key)
{
    if (s_Translations.contains(s_CurrentLanguage)) {
        const auto &trans = s_Translations[s_CurrentLanguage];
        if (trans.contains(key)) {
            return trans[key];
        }
    }
    
    // Fallback to English
    if (EN_TRANSLATIONS.contains(key)) {
        return EN_TRANSLATIONS[key];
    }
    
    return key;
}

void LanguageSettingsWidget::loadTranslations(const QString &langCode)
{
    s_CurrentLanguage = langCode;
    
    // Load custom translations from settings if available
    QSettings settings;
    settings.beginGroup("translations_" + langCode);
    QStringList keys = settings.childKeys();
    if (!keys.isEmpty()) {
        QMap<QString, QString> trans;
        for (const QString &key : keys) {
            trans[key] = settings.value(key).toString();
        }
        s_Translations[langCode] = trans;
    }
    settings.endGroup();
}

void LanguageSettingsWidget::saveCustomTranslation(const QString &langCode, const QString &key, const QString &value)
{
    s_Translations[langCode][key] = value;
    
    QSettings settings;
    settings.beginGroup("translations_" + langCode);
    settings.setValue(key, value);
    settings.endGroup();
}
