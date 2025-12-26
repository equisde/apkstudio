#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QSplitter>
#include <QTabWidget>
#include <QTextStream>
#include <QVBoxLayout>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include "apkanalysistools.h"

// ==================== APK Info Dialog ====================
ApkInfoDialog::ApkInfoDialog(const QString &projectPath, QWidget *parent)
    : QDialog(parent), m_ProjectPath(projectPath)
{
    setWindowTitle(tr("APK Information"));
    setMinimumSize(700, 500);
    
    auto layout = new QVBoxLayout(this);
    
    auto tabs = new QTabWidget(this);
    
    // General Info Tab
    auto infoWidget = new QWidget(this);
    auto infoLayout = new QVBoxLayout(infoWidget);
    m_InfoTable = new QTableWidget(this);
    m_InfoTable->setColumnCount(2);
    m_InfoTable->setHorizontalHeaderLabels({tr("Property"), tr("Value")});
    m_InfoTable->horizontalHeader()->setStretchLastSection(true);
    m_InfoTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    infoLayout->addWidget(m_InfoTable);
    tabs->addTab(infoWidget, tr("General"));
    
    // Permissions Tab
    auto permWidget = new QWidget(this);
    auto permLayout = new QVBoxLayout(permWidget);
    m_PermissionsList = new QListWidget(this);
    permLayout->addWidget(m_PermissionsList);
    tabs->addTab(permWidget, tr("Permissions"));
    
    // Components Tab
    auto compWidget = new QWidget(this);
    auto compLayout = new QVBoxLayout(compWidget);
    m_ComponentsTree = new QTreeWidget(this);
    m_ComponentsTree->setHeaderLabels({tr("Component"), tr("Type"), tr("Exported")});
    compLayout->addWidget(m_ComponentsTree);
    tabs->addTab(compWidget, tr("Components"));
    
    layout->addWidget(tabs);
    
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
    
    loadApkInfo();
}

void ApkInfoDialog::loadApkInfo()
{
    parseManifest();
}

void ApkInfoDialog::parseManifest()
{
    QString manifestPath = m_ProjectPath + "/AndroidManifest.xml";
    QFile file(manifestPath);
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QXmlStreamReader xml(&file);
    
    QString packageName, versionName, versionCode, minSdk, targetSdk;
    QStringList permissions;
    QList<QTreeWidgetItem*> activities, services, receivers, providers;
    
    while (!xml.atEnd()) {
        xml.readNext();
        
        if (xml.isStartElement()) {
            QString name = xml.name().toString();
            
            if (name == "manifest") {
                packageName = xml.attributes().value("package").toString();
                versionName = xml.attributes().value("android:versionName").toString();
                versionCode = xml.attributes().value("android:versionCode").toString();
            }
            else if (name == "uses-sdk") {
                minSdk = xml.attributes().value("android:minSdkVersion").toString();
                targetSdk = xml.attributes().value("android:targetSdkVersion").toString();
            }
            else if (name == "uses-permission") {
                QString perm = xml.attributes().value("android:name").toString();
                permissions.append(perm.replace("android.permission.", ""));
            }
            else if (name == "activity") {
                QString actName = xml.attributes().value("android:name").toString();
                QString exported = xml.attributes().value("android:exported").toString();
                auto item = new QTreeWidgetItem({actName, "Activity", exported});
                activities.append(item);
            }
            else if (name == "service") {
                QString srvName = xml.attributes().value("android:name").toString();
                QString exported = xml.attributes().value("android:exported").toString();
                auto item = new QTreeWidgetItem({srvName, "Service", exported});
                services.append(item);
            }
            else if (name == "receiver") {
                QString rcvName = xml.attributes().value("android:name").toString();
                QString exported = xml.attributes().value("android:exported").toString();
                auto item = new QTreeWidgetItem({rcvName, "Receiver", exported});
                receivers.append(item);
            }
            else if (name == "provider") {
                QString prvName = xml.attributes().value("android:name").toString();
                QString exported = xml.attributes().value("android:exported").toString();
                auto item = new QTreeWidgetItem({prvName, "Provider", exported});
                providers.append(item);
            }
        }
    }
    
    // Populate info table
    QStringList props = {"Package Name", "Version Name", "Version Code", "Min SDK", "Target SDK"};
    QStringList vals = {packageName, versionName, versionCode, minSdk, targetSdk};
    m_InfoTable->setRowCount(props.size());
    for (int i = 0; i < props.size(); ++i) {
        m_InfoTable->setItem(i, 0, new QTableWidgetItem(props[i]));
        m_InfoTable->setItem(i, 1, new QTableWidgetItem(vals[i]));
    }
    
    // Populate permissions
    for (const QString &perm : permissions) {
        m_PermissionsList->addItem(perm);
    }
    
    // Populate components
    if (!activities.isEmpty()) {
        auto actRoot = new QTreeWidgetItem({tr("Activities (%1)").arg(activities.size())});
        for (auto item : activities) actRoot->addChild(item);
        m_ComponentsTree->addTopLevelItem(actRoot);
        actRoot->setExpanded(true);
    }
    if (!services.isEmpty()) {
        auto srvRoot = new QTreeWidgetItem({tr("Services (%1)").arg(services.size())});
        for (auto item : services) srvRoot->addChild(item);
        m_ComponentsTree->addTopLevelItem(srvRoot);
    }
    if (!receivers.isEmpty()) {
        auto rcvRoot = new QTreeWidgetItem({tr("Receivers (%1)").arg(receivers.size())});
        for (auto item : receivers) rcvRoot->addChild(item);
        m_ComponentsTree->addTopLevelItem(rcvRoot);
    }
    if (!providers.isEmpty()) {
        auto prvRoot = new QTreeWidgetItem({tr("Providers (%1)").arg(providers.size())});
        for (auto item : providers) prvRoot->addChild(item);
        m_ComponentsTree->addTopLevelItem(prvRoot);
    }
    
    m_ComponentsTree->expandAll();
}

// ==================== Permission Analyzer ====================
PermissionAnalyzerDialog::PermissionAnalyzerDialog(const QString &projectPath, QWidget *parent)
    : QDialog(parent), m_ProjectPath(projectPath)
{
    setWindowTitle(tr("Permission Analyzer"));
    setMinimumSize(800, 500);
    
    auto layout = new QVBoxLayout(this);
    
    auto splitter = new QSplitter(Qt::Horizontal, this);
    
    m_PermTable = new QTableWidget(this);
    m_PermTable->setColumnCount(4);
    m_PermTable->setHorizontalHeaderLabels({tr("Permission"), tr("Risk"), tr("Category"), tr("Description")});
    m_PermTable->horizontalHeader()->setStretchLastSection(true);
    m_PermTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_PermTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    splitter->addWidget(m_PermTable);
    
    m_Details = new QTextBrowser(this);
    m_Details->setMinimumWidth(250);
    splitter->addWidget(m_Details);
    
    layout->addWidget(splitter);
    
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
    
    analyzePermissions();
}

void PermissionAnalyzerDialog::analyzePermissions()
{
    QString manifestPath = m_ProjectPath + "/AndroidManifest.xml";
    QFile file(manifestPath);
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QXmlStreamReader xml(&file);
    QStringList permissions;
    
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name().toString() == "uses-permission") {
            QString perm = xml.attributes().value("android:name").toString();
            permissions.append(perm);
        }
    }
    
    m_PermTable->setRowCount(permissions.size());
    for (int i = 0; i < permissions.size(); ++i) {
        PermissionInfo info = getPermissionInfo(permissions[i]);
        
        m_PermTable->setItem(i, 0, new QTableWidgetItem(info.name));
        
        auto riskItem = new QTableWidgetItem(info.riskLevel);
        if (info.riskLevel == "DANGEROUS") riskItem->setBackground(QColor(255, 100, 100));
        else if (info.riskLevel == "HIGH") riskItem->setBackground(QColor(255, 165, 0));
        else if (info.riskLevel == "MEDIUM") riskItem->setBackground(QColor(255, 255, 100));
        m_PermTable->setItem(i, 1, riskItem);
        
        m_PermTable->setItem(i, 2, new QTableWidgetItem(info.category));
        m_PermTable->setItem(i, 3, new QTableWidgetItem(info.description));
    }
    
    m_PermTable->resizeColumnsToContents();
}

PermissionAnalyzerDialog::PermissionInfo PermissionAnalyzerDialog::getPermissionInfo(const QString &permission)
{
    PermissionInfo info;
    info.name = permission.mid(permission.lastIndexOf('.') + 1);
    
    // Database of common permissions
    static QMap<QString, QStringList> permDb = {
        {"INTERNET", {"LOW", "Network", "Allows app to open network sockets"}},
        {"ACCESS_NETWORK_STATE", {"LOW", "Network", "View network connectivity state"}},
        {"ACCESS_WIFI_STATE", {"LOW", "Network", "View WiFi connectivity state"}},
        {"READ_EXTERNAL_STORAGE", {"MEDIUM", "Storage", "Read from external storage"}},
        {"WRITE_EXTERNAL_STORAGE", {"MEDIUM", "Storage", "Write to external storage"}},
        {"READ_CONTACTS", {"HIGH", "Privacy", "Read user's contacts data"}},
        {"WRITE_CONTACTS", {"HIGH", "Privacy", "Write to contacts data"}},
        {"READ_CALL_LOG", {"DANGEROUS", "Privacy", "Read user's call log"}},
        {"READ_SMS", {"DANGEROUS", "Privacy", "Read SMS messages"}},
        {"SEND_SMS", {"DANGEROUS", "Privacy", "Send SMS messages"}},
        {"CAMERA", {"HIGH", "Hardware", "Access device camera"}},
        {"RECORD_AUDIO", {"HIGH", "Hardware", "Record audio from microphone"}},
        {"ACCESS_FINE_LOCATION", {"HIGH", "Location", "Access precise GPS location"}},
        {"ACCESS_COARSE_LOCATION", {"MEDIUM", "Location", "Access approximate location"}},
        {"ACCESS_BACKGROUND_LOCATION", {"DANGEROUS", "Location", "Access location in background"}},
        {"READ_PHONE_STATE", {"MEDIUM", "Phone", "Read phone state and identity"}},
        {"CALL_PHONE", {"HIGH", "Phone", "Directly call phone numbers"}},
        {"RECEIVE_BOOT_COMPLETED", {"LOW", "System", "Run at startup"}},
        {"VIBRATE", {"LOW", "Hardware", "Control vibration"}},
        {"WAKE_LOCK", {"LOW", "System", "Prevent phone from sleeping"}},
        {"FOREGROUND_SERVICE", {"LOW", "System", "Run foreground service"}},
        {"REQUEST_INSTALL_PACKAGES", {"DANGEROUS", "System", "Install applications"}},
        {"SYSTEM_ALERT_WINDOW", {"DANGEROUS", "System", "Draw over other apps"}},
        {"GET_ACCOUNTS", {"MEDIUM", "Privacy", "Get accounts on device"}},
        {"BLUETOOTH", {"MEDIUM", "Hardware", "Connect to Bluetooth devices"}},
        {"BLUETOOTH_ADMIN", {"MEDIUM", "Hardware", "Manage Bluetooth connections"}},
    };
    
    if (permDb.contains(info.name)) {
        QStringList data = permDb[info.name];
        info.riskLevel = data[0];
        info.category = data[1];
        info.description = data[2];
    } else {
        info.riskLevel = "UNKNOWN";
        info.category = "Other";
        info.description = "Custom or unknown permission";
    }
    
    return info;
}

// ==================== String Search ====================
StringSearchDialog::StringSearchDialog(const QString &projectPath, QWidget *parent)
    : QDialog(parent), m_ProjectPath(projectPath)
{
    setWindowTitle(tr("Search in Project"));
    setMinimumSize(800, 500);
    
    auto layout = new QVBoxLayout(this);
    
    // Search options
    auto searchLayout = new QHBoxLayout();
    m_SearchInput = new QLineEdit(this);
    m_SearchInput->setPlaceholderText(tr("Enter search term..."));
    connect(m_SearchInput, &QLineEdit::returnPressed, this, &StringSearchDialog::performSearch);
    searchLayout->addWidget(m_SearchInput);
    
    auto searchBtn = new QPushButton(tr("Search"), this);
    connect(searchBtn, &QPushButton::clicked, this, &StringSearchDialog::performSearch);
    searchLayout->addWidget(searchBtn);
    layout->addLayout(searchLayout);
    
    // Options
    auto optLayout = new QHBoxLayout();
    m_CaseSensitive = new QCheckBox(tr("Case sensitive"), this);
    m_Regex = new QCheckBox(tr("Regex"), this);
    m_WholeWord = new QCheckBox(tr("Whole word"), this);
    optLayout->addWidget(m_CaseSensitive);
    optLayout->addWidget(m_Regex);
    optLayout->addWidget(m_WholeWord);
    optLayout->addStretch();
    layout->addLayout(optLayout);
    
    m_Progress = new QProgressBar(this);
    m_Progress->setVisible(false);
    layout->addWidget(m_Progress);
    
    m_Results = new QTableWidget(this);
    m_Results->setColumnCount(3);
    m_Results->setHorizontalHeaderLabels({tr("File"), tr("Line"), tr("Content")});
    m_Results->horizontalHeader()->setStretchLastSection(true);
    m_Results->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_Results->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(m_Results, &QTableWidget::itemDoubleClicked, this, &StringSearchDialog::handleResultClicked);
    layout->addWidget(m_Results);
    
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void StringSearchDialog::performSearch()
{
    QString term = m_SearchInput->text();
    if (term.isEmpty()) return;
    
    m_Results->setRowCount(0);
    m_Progress->setVisible(true);
    
    QStringList filters = {"*.smali", "*.xml", "*.java", "*.json", "*.txt", "*.properties"};
    QDirIterator it(m_ProjectPath, filters, QDir::Files, QDirIterator::Subdirectories);
    
    QRegularExpression regex;
    if (m_Regex->isChecked()) {
        regex.setPattern(term);
    } else {
        QString pattern = QRegularExpression::escape(term);
        if (m_WholeWord->isChecked()) {
            pattern = "\\b" + pattern + "\\b";
        }
        regex.setPattern(pattern);
    }
    
    if (!m_CaseSensitive->isChecked()) {
        regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    }
    
    int row = 0;
    while (it.hasNext()) {
        QString filePath = it.next();
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        
        QTextStream in(&file);
        int lineNum = 0;
        while (!in.atEnd()) {
            QString line = in.readLine();
            lineNum++;
            
            if (regex.match(line).hasMatch()) {
                m_Results->insertRow(row);
                QString relPath = filePath.mid(m_ProjectPath.length() + 1);
                m_Results->setItem(row, 0, new QTableWidgetItem(relPath));
                m_Results->setItem(row, 1, new QTableWidgetItem(QString::number(lineNum)));
                m_Results->setItem(row, 2, new QTableWidgetItem(line.trimmed()));
                
                // Store full path for navigation
                m_Results->item(row, 0)->setData(Qt::UserRole, filePath);
                m_Results->item(row, 1)->setData(Qt::UserRole, lineNum);
                row++;
            }
        }
    }
    
    m_Progress->setVisible(false);
    setWindowTitle(tr("Search Results - %1 matches").arg(row));
}

void StringSearchDialog::handleResultClicked(QTableWidgetItem *item)
{
    int row = item->row();
    QString path = m_Results->item(row, 0)->data(Qt::UserRole).toString();
    int line = m_Results->item(row, 1)->data(Qt::UserRole).toInt();
    emit fileSelected(path, line);
}

// ==================== API Key Finder ====================
ApiKeyFinderDialog::ApiKeyFinderDialog(const QString &projectPath, QWidget *parent)
    : QDialog(parent), m_ProjectPath(projectPath)
{
    setWindowTitle(tr("API Key & Secret Finder"));
    setMinimumSize(800, 500);
    
    // Common API key patterns
    m_Patterns = {
        R"((?:api[_-]?key|apikey)\s*[=:]\s*["']?([a-zA-Z0-9_\-]{20,}))",
        R"((?:secret[_-]?key|secretkey)\s*[=:]\s*["']?([a-zA-Z0-9_\-]{20,}))",
        R"(AIza[0-9A-Za-z_-]{35})",  // Google API
        R"(AAAA[A-Za-z0-9_-]{7}:[A-Za-z0-9_-]{140})",  // Firebase
        R"(sk_live_[0-9a-zA-Z]{24})",  // Stripe
        R"((?:ghp_|gho_|ghu_|ghs_|ghr_)[A-Za-z0-9_]{36})",  // GitHub
        R"(xox[baprs]-[0-9]{10,13}-[0-9]{10,13}[a-zA-Z0-9-]*)",  // Slack
        R"(-----BEGIN (?:RSA )?PRIVATE KEY-----)",  // Private keys
        R"(eyJ[a-zA-Z0-9_-]*\.eyJ[a-zA-Z0-9_-]*\.[a-zA-Z0-9_-]*)",  // JWT
        R"((?:password|passwd|pwd)\s*[=:]\s*["']([^"']{4,})["'])",  // Passwords
    };
    
    auto layout = new QVBoxLayout(this);
    
    auto scanBtn = new QPushButton(tr("🔍 Start Scan"), this);
    connect(scanBtn, &QPushButton::clicked, this, &ApiKeyFinderDialog::startScan);
    layout->addWidget(scanBtn);
    
    m_Progress = new QProgressBar(this);
    m_Progress->setVisible(false);
    layout->addWidget(m_Progress);
    
    m_Results = new QTableWidget(this);
    m_Results->setColumnCount(4);
    m_Results->setHorizontalHeaderLabels({tr("Type"), tr("File"), tr("Line"), tr("Value (truncated)")});
    m_Results->horizontalHeader()->setStretchLastSection(true);
    m_Results->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_Results);
    
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void ApiKeyFinderDialog::startScan()
{
    m_Results->setRowCount(0);
    m_Progress->setVisible(true);
    
    QStringList filters = {"*.smali", "*.xml", "*.java", "*.json", "*.properties", "*.gradle"};
    QDirIterator it(m_ProjectPath, filters, QDir::Files, QDirIterator::Subdirectories);
    
    while (it.hasNext()) {
        scanFile(it.next());
    }
    
    m_Progress->setVisible(false);
    setWindowTitle(tr("API Key Finder - %1 findings").arg(m_Results->rowCount()));
}

void ApiKeyFinderDialog::scanFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    
    QTextStream in(&file);
    int lineNum = 0;
    
    while (!in.atEnd()) {
        QString line = in.readLine();
        lineNum++;
        
        for (const QString &pattern : m_Patterns) {
            QRegularExpression regex(pattern, QRegularExpression::CaseInsensitiveOption);
            auto match = regex.match(line);
            if (match.hasMatch()) {
                int row = m_Results->rowCount();
                m_Results->insertRow(row);
                
                QString type = "Secret";
                if (pattern.contains("api")) type = "API Key";
                else if (pattern.contains("password")) type = "Password";
                else if (pattern.contains("PRIVATE KEY")) type = "Private Key";
                else if (pattern.contains("eyJ")) type = "JWT Token";
                
                QString relPath = path.mid(m_ProjectPath.length() + 1);
                QString value = match.captured(0);
                if (value.length() > 40) value = value.left(40) + "...";
                
                m_Results->setItem(row, 0, new QTableWidgetItem(type));
                m_Results->setItem(row, 1, new QTableWidgetItem(relPath));
                m_Results->setItem(row, 2, new QTableWidgetItem(QString::number(lineNum)));
                m_Results->setItem(row, 3, new QTableWidgetItem(value));
                
                m_Results->item(row, 0)->setBackground(QColor(255, 200, 200));
            }
        }
    }
}

// ==================== Native Library Inspector ====================
NativeLibraryDialog::NativeLibraryDialog(const QString &projectPath, QWidget *parent)
    : QDialog(parent), m_ProjectPath(projectPath)
{
    setWindowTitle(tr("Native Library Inspector"));
    setMinimumSize(700, 500);
    
    auto layout = new QVBoxLayout(this);
    
    auto splitter = new QSplitter(Qt::Horizontal, this);
    
    m_LibTree = new QTreeWidget(this);
    m_LibTree->setHeaderLabels({tr("Library"), tr("Size"), tr("Architecture")});
    connect(m_LibTree, &QTreeWidget::itemClicked, [this](QTreeWidgetItem *item) {
        QString path = item->data(0, Qt::UserRole).toString();
        if (!path.isEmpty()) analyzeLibrary(path);
    });
    splitter->addWidget(m_LibTree);
    
    m_Details = new QTextBrowser(this);
    splitter->addWidget(m_Details);
    
    layout->addWidget(splitter);
    
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
    
    scanLibraries();
}

void NativeLibraryDialog::scanLibraries()
{
    QDir libDir(m_ProjectPath + "/lib");
    if (!libDir.exists()) {
        m_Details->setText(tr("No native libraries found in this project."));
        return;
    }
    
    QStringList archs = libDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &arch : archs) {
        auto archItem = new QTreeWidgetItem({arch});
        
        QDir archDir(libDir.path() + "/" + arch);
        QStringList libs = archDir.entryList({"*.so"}, QDir::Files);
        
        for (const QString &lib : libs) {
            QString fullPath = archDir.path() + "/" + lib;
            QFileInfo info(fullPath);
            
            auto libItem = new QTreeWidgetItem({
                lib,
                QString::number(info.size() / 1024) + " KB",
                arch
            });
            libItem->setData(0, Qt::UserRole, fullPath);
            archItem->addChild(libItem);
        }
        
        m_LibTree->addTopLevelItem(archItem);
        archItem->setExpanded(true);
    }
}

void NativeLibraryDialog::analyzeLibrary(const QString &path)
{
    QFileInfo info(path);
    QString details;
    details += "<h3>" + info.fileName() + "</h3>";
    details += "<p><b>Path:</b> " + path + "</p>";
    details += "<p><b>Size:</b> " + QString::number(info.size()) + " bytes</p>";
    
    // Try to read ELF header
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray header = file.read(64);
        if (header.startsWith("\x7fELF")) {
            details += "<p><b>Format:</b> ELF</p>";
            
            int bitClass = header[4];
            details += "<p><b>Class:</b> " + QString(bitClass == 1 ? "32-bit" : "64-bit") + "</p>";
            
            int machine = header[18];
            QString arch = "Unknown";
            if (machine == 3) arch = "x86";
            else if (machine == 40) arch = "ARM";
            else if (machine == 62) arch = "x86_64";
            else if (machine == 183) arch = "ARM64";
            details += "<p><b>Architecture:</b> " + arch + "</p>";
        }
        file.close();
    }
    
    m_Details->setHtml(details);
}

// ==================== APK Size Analyzer ====================
ApkSizeAnalyzerDialog::ApkSizeAnalyzerDialog(const QString &projectPath, QWidget *parent)
    : QDialog(parent), m_ProjectPath(projectPath)
{
    setWindowTitle(tr("APK Size Analyzer"));
    setMinimumSize(600, 500);
    
    auto layout = new QVBoxLayout(this);
    
    m_UsageBar = new QProgressBar(this);
    m_UsageBar->setFormat(tr("Total Size: %v KB"));
    layout->addWidget(m_UsageBar);
    
    m_SizeTree = new QTreeWidget(this);
    m_SizeTree->setHeaderLabels({tr("Directory"), tr("Size"), tr("Percentage")});
    m_SizeTree->setColumnWidth(0, 300);
    layout->addWidget(m_SizeTree);
    
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
    
    analyzeSize();
}

void ApkSizeAnalyzerDialog::analyzeSize()
{
    QMap<QString, qint64> dirSizes;
    qint64 totalSize = 0;
    
    QDirIterator it(m_ProjectPath, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString path = it.next();
        QFileInfo info(path);
        qint64 size = info.size();
        totalSize += size;
        
        // Get top-level directory
        QString relPath = path.mid(m_ProjectPath.length() + 1);
        QString topDir = relPath.section('/', 0, 0);
        dirSizes[topDir] += size;
    }
    
    m_UsageBar->setMaximum(totalSize / 1024);
    m_UsageBar->setValue(totalSize / 1024);
    
    // Sort by size
    QList<QPair<QString, qint64>> sorted;
    for (auto it = dirSizes.begin(); it != dirSizes.end(); ++it) {
        sorted.append({it.key(), it.value()});
    }
    std::sort(sorted.begin(), sorted.end(), [](auto a, auto b) { return a.second > b.second; });
    
    for (const auto &pair : sorted) {
        double percent = (pair.second * 100.0) / totalSize;
        auto item = new QTreeWidgetItem({
            pair.first,
            formatSize(pair.second),
            QString::number(percent, 'f', 1) + "%"
        });
        m_SizeTree->addTopLevelItem(item);
    }
}

QString ApkSizeAnalyzerDialog::formatSize(qint64 bytes)
{
    if (bytes < 1024) return QString::number(bytes) + " B";
    if (bytes < 1024 * 1024) return QString::number(bytes / 1024) + " KB";
    return QString::number(bytes / (1024 * 1024)) + " MB";
}

// ==================== Smali Patcher ====================
SmaliPatcherDialog::SmaliPatcherDialog(const QString &projectPath, QWidget *parent)
    : QDialog(parent), m_ProjectPath(projectPath)
{
    setWindowTitle(tr("Smali Patcher"));
    setMinimumSize(800, 500);
    
    auto layout = new QVBoxLayout(this);
    
    // Find/Replace inputs
    auto findLayout = new QHBoxLayout();
    findLayout->addWidget(new QLabel(tr("Find:"), this));
    m_FindInput = new QLineEdit(this);
    findLayout->addWidget(m_FindInput);
    layout->addLayout(findLayout);
    
    auto replaceLayout = new QHBoxLayout();
    replaceLayout->addWidget(new QLabel(tr("Replace:"), this));
    m_ReplaceInput = new QLineEdit(this);
    replaceLayout->addWidget(m_ReplaceInput);
    layout->addLayout(replaceLayout);
    
    auto optLayout = new QHBoxLayout();
    m_CaseSensitive = new QCheckBox(tr("Case sensitive"), this);
    m_Regex = new QCheckBox(tr("Regex"), this);
    optLayout->addWidget(m_CaseSensitive);
    optLayout->addWidget(m_Regex);
    
    auto findBtn = new QPushButton(tr("Find All"), this);
    connect(findBtn, &QPushButton::clicked, this, &SmaliPatcherDialog::findOccurrences);
    optLayout->addWidget(findBtn);
    
    auto replaceBtn = new QPushButton(tr("Replace All"), this);
    connect(replaceBtn, &QPushButton::clicked, this, &SmaliPatcherDialog::replaceAll);
    optLayout->addWidget(replaceBtn);
    
    optLayout->addStretch();
    layout->addLayout(optLayout);
    
    m_Results = new QTableWidget(this);
    m_Results->setColumnCount(4);
    m_Results->setHorizontalHeaderLabels({tr("File"), tr("Line"), tr("Before"), tr("After")});
    m_Results->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(m_Results);
    
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void SmaliPatcherDialog::findOccurrences()
{
    QString term = m_FindInput->text();
    if (term.isEmpty()) return;
    
    m_Results->setRowCount(0);
    
    QDirIterator it(m_ProjectPath, {"*.smali"}, QDir::Files, QDirIterator::Subdirectories);
    
    QRegularExpression regex;
    if (m_Regex->isChecked()) {
        regex.setPattern(term);
    } else {
        regex.setPattern(QRegularExpression::escape(term));
    }
    if (!m_CaseSensitive->isChecked()) {
        regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    }
    
    QString replacement = m_ReplaceInput->text();
    
    while (it.hasNext()) {
        QString path = it.next();
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        
        QTextStream in(&file);
        int lineNum = 0;
        while (!in.atEnd()) {
            QString line = in.readLine();
            lineNum++;
            
            if (regex.match(line).hasMatch()) {
                int row = m_Results->rowCount();
                m_Results->insertRow(row);
                
                QString relPath = path.mid(m_ProjectPath.length() + 1);
                QString after = line;
                after.replace(regex, replacement);
                
                m_Results->setItem(row, 0, new QTableWidgetItem(relPath));
                m_Results->setItem(row, 1, new QTableWidgetItem(QString::number(lineNum)));
                m_Results->setItem(row, 2, new QTableWidgetItem(line.trimmed()));
                m_Results->setItem(row, 3, new QTableWidgetItem(after.trimmed()));
                
                m_Results->item(row, 0)->setData(Qt::UserRole, path);
            }
        }
    }
}

void SmaliPatcherDialog::replaceAll()
{
    QString term = m_FindInput->text();
    QString replacement = m_ReplaceInput->text();
    if (term.isEmpty()) return;
    
    QRegularExpression regex;
    if (m_Regex->isChecked()) {
        regex.setPattern(term);
    } else {
        regex.setPattern(QRegularExpression::escape(term));
    }
    if (!m_CaseSensitive->isChecked()) {
        regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    }
    
    int count = 0;
    QDirIterator it(m_ProjectPath, {"*.smali"}, QDir::Files, QDirIterator::Subdirectories);
    
    while (it.hasNext()) {
        QString path = it.next();
        QFile file(path);
        if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) continue;
        
        QString content = file.readAll();
        int before = content.count(regex);
        if (before > 0) {
            content.replace(regex, replacement);
            file.seek(0);
            file.resize(0);
            file.write(content.toUtf8());
            count += before;
        }
        file.close();
    }
    
    QMessageBox::information(this, tr("Replace Complete"), 
        tr("Replaced %1 occurrences in smali files.").arg(count));
    findOccurrences();
}

void SmaliPatcherDialog::replaceSelected()
{
    // TODO: Implement selective replacement
}

// ==================== Hardcoded Finder ====================
HardcodedFinderDialog::HardcodedFinderDialog(const QString &projectPath, QWidget *parent)
    : QDialog(parent), m_ProjectPath(projectPath)
{
    setWindowTitle(tr("Hardcoded URL/IP Finder"));
    setMinimumSize(800, 500);
    
    auto layout = new QVBoxLayout(this);
    
    auto optLayout = new QHBoxLayout();
    m_FindUrls = new QCheckBox(tr("URLs"), this);
    m_FindUrls->setChecked(true);
    m_FindIps = new QCheckBox(tr("IP Addresses"), this);
    m_FindIps->setChecked(true);
    m_FindEmails = new QCheckBox(tr("Emails"), this);
    optLayout->addWidget(m_FindUrls);
    optLayout->addWidget(m_FindIps);
    optLayout->addWidget(m_FindEmails);
    
    auto scanBtn = new QPushButton(tr("Scan"), this);
    connect(scanBtn, &QPushButton::clicked, this, &HardcodedFinderDialog::startScan);
    optLayout->addWidget(scanBtn);
    optLayout->addStretch();
    layout->addLayout(optLayout);
    
    m_Results = new QTableWidget(this);
    m_Results->setColumnCount(4);
    m_Results->setHorizontalHeaderLabels({tr("Type"), tr("File"), tr("Line"), tr("Value")});
    m_Results->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(m_Results);
    
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void HardcodedFinderDialog::startScan()
{
    m_Results->setRowCount(0);
    
    QStringList patterns;
    if (m_FindUrls->isChecked()) {
        patterns << R"(https?://[^\s"'<>]+)";
    }
    if (m_FindIps->isChecked()) {
        patterns << R"(\b(?:\d{1,3}\.){3}\d{1,3}\b)";
    }
    if (m_FindEmails->isChecked()) {
        patterns << R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})";
    }
    
    QDirIterator it(m_ProjectPath, {"*.smali", "*.xml", "*.java", "*.json"}, 
                    QDir::Files, QDirIterator::Subdirectories);
    
    while (it.hasNext()) {
        QString path = it.next();
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        
        QTextStream in(&file);
        int lineNum = 0;
        while (!in.atEnd()) {
            QString line = in.readLine();
            lineNum++;
            
            for (int i = 0; i < patterns.size(); ++i) {
                QRegularExpression regex(patterns[i]);
                auto match = regex.match(line);
                if (match.hasMatch()) {
                    int row = m_Results->rowCount();
                    m_Results->insertRow(row);
                    
                    QString type;
                    if (i == 0) type = "URL";
                    else if (i == 1) type = "IP";
                    else type = "Email";
                    
                    QString relPath = path.mid(m_ProjectPath.length() + 1);
                    
                    m_Results->setItem(row, 0, new QTableWidgetItem(type));
                    m_Results->setItem(row, 1, new QTableWidgetItem(relPath));
                    m_Results->setItem(row, 2, new QTableWidgetItem(QString::number(lineNum)));
                    m_Results->setItem(row, 3, new QTableWidgetItem(match.captured(0)));
                }
            }
        }
    }
}

// ==================== Firebase Extractor ====================
FirebaseExtractorDialog::FirebaseExtractorDialog(const QString &projectPath, QWidget *parent)
    : QDialog(parent), m_ProjectPath(projectPath)
{
    setWindowTitle(tr("Firebase Config Extractor"));
    setMinimumSize(600, 400);
    
    auto layout = new QVBoxLayout(this);
    
    m_ConfigTable = new QTableWidget(this);
    m_ConfigTable->setColumnCount(2);
    m_ConfigTable->setHorizontalHeaderLabels({tr("Key"), tr("Value")});
    m_ConfigTable->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(m_ConfigTable);
    
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
    
    extractConfig();
}

void FirebaseExtractorDialog::extractConfig()
{
    // Look for google-services.json or firebase config in strings.xml
    QString googleServices = m_ProjectPath + "/assets/google-services.json";
    QString stringsXml = m_ProjectPath + "/res/values/strings.xml";
    
    QMap<QString, QString> config;
    
    // Try google-services.json
    QFile gsFile(googleServices);
    if (gsFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(gsFile.readAll());
        QJsonObject obj = doc.object();
        
        if (obj.contains("project_info")) {
            QJsonObject proj = obj["project_info"].toObject();
            config["Project ID"] = proj["project_id"].toString();
            config["Project Number"] = proj["project_number"].toString();
            config["Storage Bucket"] = proj["storage_bucket"].toString();
        }
        
        if (obj.contains("client")) {
            QJsonArray clients = obj["client"].toArray();
            if (!clients.isEmpty()) {
                QJsonObject client = clients[0].toObject();
                QJsonObject clientInfo = client["client_info"].toObject();
                config["Package Name"] = clientInfo["android_client_info"].toObject()["package_name"].toString();
                
                QJsonArray apiKey = client["api_key"].toArray();
                if (!apiKey.isEmpty()) {
                    config["API Key"] = apiKey[0].toObject()["current_key"].toString();
                }
            }
        }
    }
    
    // Look in strings.xml for Firebase keys
    QFile strFile(stringsXml);
    if (strFile.open(QIODevice::ReadOnly)) {
        QXmlStreamReader xml(&strFile);
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isStartElement() && xml.name().toString() == "string") {
                QString name = xml.attributes().value("name").toString();
                QString value = xml.readElementText();
                
                if (name.contains("firebase", Qt::CaseInsensitive) ||
                    name.contains("google_", Qt::CaseInsensitive) ||
                    name.contains("gcm", Qt::CaseInsensitive)) {
                    config[name] = value;
                }
            }
        }
    }
    
    m_ConfigTable->setRowCount(config.size());
    int row = 0;
    for (auto it = config.begin(); it != config.end(); ++it) {
        m_ConfigTable->setItem(row, 0, new QTableWidgetItem(it.key()));
        m_ConfigTable->setItem(row, 1, new QTableWidgetItem(it.value()));
        row++;
    }
}

// ==================== Android TV Optimizer ====================
AndroidTVOptimizerDialog::AndroidTVOptimizerDialog(const QString &projectPath, QWidget *parent)
    : QDialog(parent), m_ProjectPath(projectPath)
{
    setWindowTitle(tr("Android TV Optimizer"));
    setMinimumSize(700, 500);
    
    auto layout = new QVBoxLayout(this);
    
    auto splitter = new QSplitter(Qt::Horizontal, this);
    
    // Checklist
    auto checkWidget = new QWidget(this);
    auto checkLayout = new QVBoxLayout(checkWidget);
    checkLayout->addWidget(new QLabel(tr("TV Compatibility Checklist:"), this));
    
    m_CheckList = new QListWidget(this);
    checkLayout->addWidget(m_CheckList);
    
    auto btnLayout = new QVBoxLayout();
    auto addLeanbackBtn = new QPushButton(tr("Add Leanback Support"), this);
    connect(addLeanbackBtn, &QPushButton::clicked, this, &AndroidTVOptimizerDialog::addLeanbackSupport);
    btnLayout->addWidget(addLeanbackBtn);
    
    auto addBannerBtn = new QPushButton(tr("Add Banner Icon"), this);
    connect(addBannerBtn, &QPushButton::clicked, this, &AndroidTVOptimizerDialog::addBannerIcon);
    btnLayout->addWidget(addBannerBtn);
    
    auto optimizeBtn = new QPushButton(tr("Optimize for TV"), this);
    connect(optimizeBtn, &QPushButton::clicked, this, &AndroidTVOptimizerDialog::optimizeForTV);
    btnLayout->addWidget(optimizeBtn);
    
    btnLayout->addStretch();
    checkLayout->addLayout(btnLayout);
    splitter->addWidget(checkWidget);
    
    // Recommendations
    m_Recommendations = new QTextBrowser(this);
    splitter->addWidget(m_Recommendations);
    
    layout->addWidget(splitter);
    
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
    
    analyzeForTV();
}

void AndroidTVOptimizerDialog::analyzeForTV()
{
    QString manifestPath = m_ProjectPath + "/AndroidManifest.xml";
    QFile file(manifestPath);
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QString content = file.readAll();
    
    struct Check {
        QString name;
        bool passed;
        QString recommendation;
    };
    
    QList<Check> checks = {
        {"Leanback Support", content.contains("android.software.leanback"), 
         "Add <uses-feature android:name=\"android.software.leanback\" android:required=\"false\" />"},
        {"Touchscreen Not Required", content.contains("android.hardware.touchscreen") && content.contains("required=\"false\""),
         "Add <uses-feature android:name=\"android.hardware.touchscreen\" android:required=\"false\" />"},
        {"Banner Icon", content.contains("android:banner"),
         "Add android:banner=\"@drawable/banner\" to <application> tag (320x180 px)"},
        {"Leanback Launcher", content.contains("LEANBACK_LAUNCHER"),
         "Add <category android:name=\"android.intent.category.LEANBACK_LAUNCHER\" /> to main activity"},
        {"D-pad Navigation", true, "Ensure UI is navigable with D-pad (arrow keys)"},
    };
    
    QString html = "<h3>Android TV Optimization Recommendations</h3><ul>";
    
    for (const auto &check : checks) {
        auto item = new QListWidgetItem(check.name, m_CheckList);
        item->setCheckState(check.passed ? Qt::Checked : Qt::Unchecked);
        item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);
        
        if (!check.passed) {
            html += "<li><b>" + check.name + ":</b> " + check.recommendation + "</li>";
        }
    }
    
    html += "</ul>";
    m_Recommendations->setHtml(html);
}

void AndroidTVOptimizerDialog::addLeanbackSupport()
{
    QString manifestPath = m_ProjectPath + "/AndroidManifest.xml";
    QFile file(manifestPath);
    if (!file.open(QIODevice::ReadWrite)) return;
    
    QString content = file.readAll();
    
    if (!content.contains("android.software.leanback")) {
        int pos = content.indexOf("<application");
        if (pos != -1) {
            QString insert = "    <uses-feature android:name=\"android.software.leanback\" android:required=\"false\" />\n"
                           "    <uses-feature android:name=\"android.hardware.touchscreen\" android:required=\"false\" />\n";
            content.insert(pos, insert);
            
            file.seek(0);
            file.resize(0);
            file.write(content.toUtf8());
        }
    }
    file.close();
    
    QMessageBox::information(this, tr("Done"), tr("Leanback support added to manifest."));
    analyzeForTV();
}

void AndroidTVOptimizerDialog::addBannerIcon()
{
    QMessageBox::information(this, tr("Banner Icon"), 
        tr("Create a 320x180 pixel banner image and save it as:\n%1/res/drawable-xhdpi/banner.png\n\n"
           "Then add android:banner=\"@drawable/banner\" to the <application> tag in AndroidManifest.xml")
           .arg(m_ProjectPath));
}

void AndroidTVOptimizerDialog::optimizeForTV()
{
    addLeanbackSupport();
    QMessageBox::information(this, tr("Optimization"), 
        tr("Basic TV optimization applied. Review the checklist and recommendations for additional improvements."));
}

// ==================== Obfuscation Detector ====================
ObfuscationDetectorDialog::ObfuscationDetectorDialog(const QString &projectPath, QWidget *parent)
    : QDialog(parent), m_ProjectPath(projectPath)
{
    setWindowTitle(tr("Obfuscation Detector"));
    setMinimumSize(600, 400);
    
    auto layout = new QVBoxLayout(this);
    
    layout->addWidget(new QLabel(tr("Obfuscation Level:"), this));
    m_ObfuscationLevel = new QProgressBar(this);
    m_ObfuscationLevel->setFormat("%v%");
    layout->addWidget(m_ObfuscationLevel);
    
    m_Results = new QTableWidget(this);
    m_Results->setColumnCount(3);
    m_Results->setHorizontalHeaderLabels({tr("Indicator"), tr("Count"), tr("Impact")});
    m_Results->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(m_Results);
    
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
    
    detectObfuscation();
}

void ObfuscationDetectorDialog::detectObfuscation()
{
    int shortNames = 0;  // a, b, c class names
    int longStrings = 0; // Encrypted strings
    int nativeLib = 0;   // Native libraries
    int totalClasses = 0;
    
    QDirIterator it(m_ProjectPath + "/smali", {"*.smali"}, QDir::Files, QDirIterator::Subdirectories);
    
    while (it.hasNext()) {
        QString path = it.next();
        QFileInfo info(path);
        totalClasses++;
        
        // Check for short class names (a.smali, b.smali, etc.)
        QString baseName = info.baseName();
        if (baseName.length() <= 2 && baseName.at(0).isLower()) {
            shortNames++;
        }
        
        // Check for encrypted/encoded strings
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            QString content = file.readAll();
            QRegularExpression encRegex(R"(const-string[^\n]+\"[A-Za-z0-9+/=]{50,}\")");
            longStrings += content.count(encRegex);
        }
    }
    
    // Check native libs
    QDir libDir(m_ProjectPath + "/lib");
    if (libDir.exists()) {
        nativeLib = QDirIterator(libDir.path(), {"*.so"}, QDir::Files, QDirIterator::Subdirectories).hasNext() ? 1 : 0;
    }
    
    // Calculate score
    int score = 0;
    if (totalClasses > 0) {
        double shortRatio = (double)shortNames / totalClasses;
        score += qMin(40, (int)(shortRatio * 100));
    }
    score += qMin(30, longStrings);
    score += nativeLib * 20;
    
    m_ObfuscationLevel->setMaximum(100);
    m_ObfuscationLevel->setValue(qMin(100, score));
    
    // Results table
    m_Results->setRowCount(4);
    m_Results->setItem(0, 0, new QTableWidgetItem(tr("Short class names (a, b, c...)")));
    m_Results->setItem(0, 1, new QTableWidgetItem(QString::number(shortNames)));
    m_Results->setItem(0, 2, new QTableWidgetItem(tr("ProGuard/R8")));
    
    m_Results->setItem(1, 0, new QTableWidgetItem(tr("Encoded strings")));
    m_Results->setItem(1, 1, new QTableWidgetItem(QString::number(longStrings)));
    m_Results->setItem(1, 2, new QTableWidgetItem(tr("String encryption")));
    
    m_Results->setItem(2, 0, new QTableWidgetItem(tr("Native libraries")));
    m_Results->setItem(2, 1, new QTableWidgetItem(nativeLib ? "Yes" : "No"));
    m_Results->setItem(2, 2, new QTableWidgetItem(tr("Native protection")));
    
    m_Results->setItem(3, 0, new QTableWidgetItem(tr("Total classes")));
    m_Results->setItem(3, 1, new QTableWidgetItem(QString::number(totalClasses)));
    m_Results->setItem(3, 2, new QTableWidgetItem("-"));
}

// ==================== String Resource Editor ====================
StringResourceEditorDialog::StringResourceEditorDialog(const QString &projectPath, QWidget *parent)
    : QDialog(parent), m_ProjectPath(projectPath)
{
    setWindowTitle(tr("String Resource Editor"));
    setMinimumSize(700, 500);
    
    auto layout = new QVBoxLayout(this);
    
    auto topLayout = new QHBoxLayout();
    topLayout->addWidget(new QLabel(tr("Language:"), this));
    m_LanguageCombo = new QComboBox(this);
    connect(m_LanguageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StringResourceEditorDialog::loadStrings);
    topLayout->addWidget(m_LanguageCombo);
    topLayout->addStretch();
    
    auto addBtn = new QPushButton(tr("Add"), this);
    connect(addBtn, &QPushButton::clicked, this, &StringResourceEditorDialog::addString);
    topLayout->addWidget(addBtn);
    
    auto removeBtn = new QPushButton(tr("Remove"), this);
    connect(removeBtn, &QPushButton::clicked, this, &StringResourceEditorDialog::removeString);
    topLayout->addWidget(removeBtn);
    
    layout->addLayout(topLayout);
    
    m_StringsTable = new QTableWidget(this);
    m_StringsTable->setColumnCount(2);
    m_StringsTable->setHorizontalHeaderLabels({tr("Name"), tr("Value")});
    m_StringsTable->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(m_StringsTable);
    
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &StringResourceEditorDialog::saveStrings);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
    
    // Find all values directories
    QDir resDir(m_ProjectPath + "/res");
    QStringList valueDirs = resDir.entryList(QStringList() << "values*", QDir::Dirs);
    for (const QString &dir : valueDirs) {
        QString lang = dir == "values" ? "Default" : dir.mid(7);
        m_LanguageCombo->addItem(lang, dir);
    }
    
    if (m_LanguageCombo->count() > 0) {
        loadStrings();
    }
}

void StringResourceEditorDialog::loadStrings()
{
    m_StringsTable->setRowCount(0);
    
    QString valuesDir = m_LanguageCombo->currentData().toString();
    QString stringsPath = m_ProjectPath + "/res/" + valuesDir + "/strings.xml";
    
    QFile file(stringsPath);
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QXmlStreamReader xml(&file);
    int row = 0;
    
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name().toString() == "string") {
            QString name = xml.attributes().value("name").toString();
            QString value = xml.readElementText();
            
            m_StringsTable->insertRow(row);
            m_StringsTable->setItem(row, 0, new QTableWidgetItem(name));
            m_StringsTable->setItem(row, 1, new QTableWidgetItem(value));
            row++;
        }
    }
}

void StringResourceEditorDialog::saveStrings()
{
    QString valuesDir = m_LanguageCombo->currentData().toString();
    QString stringsPath = m_ProjectPath + "/res/" + valuesDir + "/strings.xml";
    
    QFile file(stringsPath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Error"), tr("Could not save strings.xml"));
        return;
    }
    
    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("resources");
    
    for (int i = 0; i < m_StringsTable->rowCount(); ++i) {
        QString name = m_StringsTable->item(i, 0)->text();
        QString value = m_StringsTable->item(i, 1)->text();
        
        xml.writeStartElement("string");
        xml.writeAttribute("name", name);
        xml.writeCharacters(value);
        xml.writeEndElement();
    }
    
    xml.writeEndElement();
    xml.writeEndDocument();
    
    QMessageBox::information(this, tr("Saved"), tr("Strings saved successfully."));
}

void StringResourceEditorDialog::addString()
{
    int row = m_StringsTable->rowCount();
    m_StringsTable->insertRow(row);
    m_StringsTable->setItem(row, 0, new QTableWidgetItem("new_string"));
    m_StringsTable->setItem(row, 1, new QTableWidgetItem(""));
    m_StringsTable->editItem(m_StringsTable->item(row, 0));
}

void StringResourceEditorDialog::removeString()
{
    int row = m_StringsTable->currentRow();
    if (row >= 0) {
        m_StringsTable->removeRow(row);
    }
}

// ==================== Certificate Info ====================
CertificateInfoDialog::CertificateInfoDialog(const QString &apkPath, QWidget *parent)
    : QDialog(parent), m_ApkPath(apkPath)
{
    setWindowTitle(tr("Certificate Information"));
    setMinimumSize(500, 400);
    
    auto layout = new QVBoxLayout(this);
    
    m_CertTable = new QTableWidget(this);
    m_CertTable->setColumnCount(2);
    m_CertTable->setHorizontalHeaderLabels({tr("Property"), tr("Value")});
    m_CertTable->horizontalHeader()->setStretchLastSection(true);
    m_CertTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_CertTable);
    
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
    
    extractCertInfo();
}

void CertificateInfoDialog::extractCertInfo()
{
    // Use keytool or apksigner to extract certificate info
    QSettings settings;
    QString javaPath = settings.value("java_binary").toString();
    
    QProcess process;
    process.start("keytool", QStringList() << "-printcert" << "-jarfile" << m_ApkPath);
    process.waitForFinished(10000);
    
    QString output = process.readAllStandardOutput();
    
    if (output.isEmpty()) {
        m_CertTable->setRowCount(1);
        m_CertTable->setItem(0, 0, new QTableWidgetItem("Status"));
        m_CertTable->setItem(0, 1, new QTableWidgetItem("Could not extract certificate. Make sure keytool is available."));
        return;
    }
    
    // Parse keytool output
    QStringList lines = output.split('\n');
    int row = 0;
    
    for (const QString &line : lines) {
        if (line.contains(':')) {
            int colonPos = line.indexOf(':');
            QString key = line.left(colonPos).trimmed();
            QString value = line.mid(colonPos + 1).trimmed();
            
            if (!key.isEmpty() && !value.isEmpty()) {
                m_CertTable->insertRow(row);
                m_CertTable->setItem(row, 0, new QTableWidgetItem(key));
                m_CertTable->setItem(row, 1, new QTableWidgetItem(value));
                row++;
            }
        }
    }
}

// ==================== APK Compare ====================
ApkCompareDialog::ApkCompareDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Compare APKs"));
    setMinimumSize(800, 600);
    
    auto layout = new QVBoxLayout(this);
    
    auto selectLayout = new QHBoxLayout();
    
    auto apk1Btn = new QPushButton(tr("Select APK 1..."), this);
    connect(apk1Btn, &QPushButton::clicked, this, &ApkCompareDialog::selectApk1);
    selectLayout->addWidget(apk1Btn);
    
    auto apk2Btn = new QPushButton(tr("Select APK 2..."), this);
    connect(apk2Btn, &QPushButton::clicked, this, &ApkCompareDialog::selectApk2);
    selectLayout->addWidget(apk2Btn);
    
    auto compareBtn = new QPushButton(tr("Compare"), this);
    connect(compareBtn, &QPushButton::clicked, this, &ApkCompareDialog::compare);
    selectLayout->addWidget(compareBtn);
    
    layout->addLayout(selectLayout);
    
    m_DiffTree = new QTreeWidget(this);
    m_DiffTree->setHeaderLabels({tr("File"), tr("Status"), tr("Size Diff")});
    m_DiffTree->setColumnWidth(0, 400);
    layout->addWidget(m_DiffTree);
    
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void ApkCompareDialog::selectApk1()
{
    m_Apk1Path = QFileDialog::getOpenFileName(this, tr("Select APK 1"), QString(), tr("APK Files (*.apk)"));
}

void ApkCompareDialog::selectApk2()
{
    m_Apk2Path = QFileDialog::getOpenFileName(this, tr("Select APK 2"), QString(), tr("APK Files (*.apk)"));
}

void ApkCompareDialog::compare()
{
    if (m_Apk1Path.isEmpty() || m_Apk2Path.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select both APK files."));
        return;
    }
    
    m_DiffTree->clear();
    
    // This is a simplified comparison - just compare file sizes for now
    QFileInfo info1(m_Apk1Path);
    QFileInfo info2(m_Apk2Path);
    
    auto root1 = new QTreeWidgetItem({info1.fileName()});
    root1->setData(1, Qt::DisplayRole, QString::number(info1.size() / 1024) + " KB");
    m_DiffTree->addTopLevelItem(root1);
    
    auto root2 = new QTreeWidgetItem({info2.fileName()});
    root2->setData(1, Qt::DisplayRole, QString::number(info2.size() / 1024) + " KB");
    m_DiffTree->addTopLevelItem(root2);
    
    qint64 diff = info2.size() - info1.size();
    auto diffItem = new QTreeWidgetItem({tr("Size Difference")});
    diffItem->setData(1, Qt::DisplayRole, (diff >= 0 ? "+" : "") + QString::number(diff / 1024) + " KB");
    m_DiffTree->addTopLevelItem(diffItem);
    
    QMessageBox::information(this, tr("Comparison"), 
        tr("Basic comparison complete.\n\nFor detailed file-by-file comparison, decompile both APKs and use a diff tool."));
}

// ==================== String Resource Editor ====================
StringResourceEditorDialog::StringResourceEditorDialog(const QString &projectPath, QWidget *parent)
    : QDialog(parent), m_ProjectPath(projectPath)
{
    setWindowTitle(tr("String Resource Editor"));
    setMinimumSize(700, 500);
    
    auto layout = new QVBoxLayout(this);
    
    auto topLayout = new QHBoxLayout();
    topLayout->addWidget(new QLabel(tr("Language:"), this));
    m_LanguageCombo = new QComboBox(this);
    connect(m_LanguageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StringResourceEditorDialog::loadStrings);
    topLayout->addWidget(m_LanguageCombo);
    topLayout->addStretch();
    
    layout->addLayout(topLayout);
    
    m_StringsTable = new QTableWidget(this);
    m_StringsTable->setColumnCount(2);
    m_StringsTable->setHorizontalHeaderLabels({tr("Name"), tr("Value")});
    m_StringsTable->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(m_StringsTable);
    
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &StringResourceEditorDialog::saveStrings);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
    
    QDir resDir(m_ProjectPath + "/res");
    QStringList valueDirs = resDir.entryList(QStringList() << "values*", QDir::Dirs);
    for (const QString &dir : valueDirs) {
        QString lang = dir == "values" ? "Default" : dir.mid(7);
        m_LanguageCombo->addItem(lang, dir);
    }
    if (m_LanguageCombo->count() > 0) loadStrings();
}

void StringResourceEditorDialog::loadStrings() { m_StringsTable->setRowCount(0); }
void StringResourceEditorDialog::saveStrings() { accept(); }
void StringResourceEditorDialog::addString() {}
void StringResourceEditorDialog::removeString() {}

// ==================== Certificate Info ====================
CertificateInfoDialog::CertificateInfoDialog(const QString &apkPath, QWidget *parent)
    : QDialog(parent), m_ApkPath(apkPath)
{
    setWindowTitle(tr("Certificate Information"));
    setMinimumSize(500, 400);
    auto layout = new QVBoxLayout(this);
    m_CertTable = new QTableWidget(this);
    m_CertTable->setColumnCount(2);
    m_CertTable->setHorizontalHeaderLabels({tr("Property"), tr("Value")});
    layout->addWidget(m_CertTable);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
    extractCertInfo();
}

void CertificateInfoDialog::extractCertInfo()
{
    m_CertTable->setRowCount(1);
    m_CertTable->setItem(0, 0, new QTableWidgetItem("APK"));
    m_CertTable->setItem(0, 1, new QTableWidgetItem(m_ApkPath));
}

// ==================== APK Compare ====================
ApkCompareDialog::ApkCompareDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Compare APKs"));
    setMinimumSize(800, 600);
    auto layout = new QVBoxLayout(this);
    m_DiffTree = new QTreeWidget(this);
    m_DiffTree->setHeaderLabels({tr("File"), tr("Status")});
    layout->addWidget(m_DiffTree);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void ApkCompareDialog::selectApk1() {}
void ApkCompareDialog::selectApk2() {}
void ApkCompareDialog::compare() {}
