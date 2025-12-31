#include "cloningstudio.h"
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QRegularExpression>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

CloningStudio::CloningStudio(const QString &projectPath, QWidget *parent)
    : QWidget(parent), m_ProjectPath(projectPath)
{
    setupUI();
}

void CloningStudio::setProjectPath(const QString &path) {
    m_ProjectPath = path;
    m_AiLog->append("Project changed: " + path);
}

void CloningStudio::setupUI() {
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(15);

    auto group = new QGroupBox(tr("📦 APK Cloning & Renaming"));
    auto form = new QFormLayout(group);

    m_EditNewPackage = new QLineEdit();
    m_EditNewPackage->setPlaceholderText("com.new.package.name");
    form->addRow(tr("New Package Name:"), m_EditNewPackage);

    m_EditNewName = new QLineEdit();
    m_EditNewName->setPlaceholderText("Cloned App Name");
    form->addRow(tr("New App Name:"), m_EditNewName);

    auto btnAi = new QPushButton(tr("🤖 AI Suggest Clone Config"));
    connect(btnAi, &QPushButton::clicked, this, &CloningStudio::aiSuggestCloneConfig);
    form->addRow(btnAi);

    layout->addWidget(group);

    m_AiLog = new QTextBrowser();
    m_AiLog->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4;");
    layout->addWidget(new QLabel(tr("<b>AI Cloning Log</b>")));
    layout->addWidget(m_AiLog);

    auto btnClone = new QPushButton(tr("🚀 Generate Cloned APK"));
    btnClone->setStyleSheet("background-color: #238636; color: white; font-weight: bold; padding: 10px;");
    connect(btnClone, &QPushButton::clicked, this, &CloningStudio::generateClone);
    layout->addWidget(btnClone);
}

void CloningStudio::generateClone() {
    QString newPackage = m_EditNewPackage->text().trimmed();
    QString newName = m_EditNewName->text().trimmed();
    
    if (newPackage.isEmpty()) {
        QMessageBox::warning(this, tr("Clone Error"), tr("Please enter a new package name."));
        return;
    }
    
    if (m_ProjectPath.isEmpty()) {
        QMessageBox::warning(this, tr("Clone Error"), tr("No project loaded."));
        return;
    }
    
    m_AiLog->append("<b>Starting cloning process...</b>");
    
    // 1. Read original package name from AndroidManifest.xml
    QString manifestPath = m_ProjectPath + "/AndroidManifest.xml";
    QFile manifestFile(manifestPath);
    if (!manifestFile.open(QIODevice::ReadOnly)) {
        m_AiLog->append("<span style='color: #f85149;'>Error: Cannot read AndroidManifest.xml</span>");
        return;
    }
    
    QString manifestContent = QString::fromUtf8(manifestFile.readAll());
    manifestFile.close();
    
    // Extract original package
    QRegularExpression pkgRegex("package=\"([^\"]+)\"");
    auto match = pkgRegex.match(manifestContent);
    if (!match.hasMatch()) {
        m_AiLog->append("<span style='color: #f85149;'>Error: Cannot find package name in manifest</span>");
        return;
    }
    
    QString originalPackage = match.captured(1);
    m_AiLog->append(tr("Original package: %1").arg(originalPackage));
    m_AiLog->append(tr("New package: %1").arg(newPackage));
    
    // 2. Replace package in AndroidManifest.xml
    manifestContent.replace("package=\"" + originalPackage + "\"", "package=\"" + newPackage + "\"");
    
    // Also update authorities for providers
    manifestContent.replace("android:authorities=\"" + originalPackage, "android:authorities=\"" + newPackage);
    
    // Update app name if provided
    if (!newName.isEmpty()) {
        // This would typically need to modify strings.xml, but for now we update manifest label
        m_AiLog->append(tr("New app name: %1").arg(newName));
    }
    
    // Write updated manifest
    if (!manifestFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        m_AiLog->append("<span style='color: #f85149;'>Error: Cannot write AndroidManifest.xml</span>");
        return;
    }
    manifestFile.write(manifestContent.toUtf8());
    manifestFile.close();
    m_AiLog->append("<span style='color: #7ee787;'>✅ Updated AndroidManifest.xml</span>");
    
    // 3. Rename smali directories
    QString oldSmaliPath = originalPackage;
    oldSmaliPath.replace('.', '/');
    QString newSmaliPath = newPackage;
    newSmaliPath.replace('.', '/');
    
    QStringList smaliDirs = {"smali", "smali_classes2", "smali_classes3", "smali_classes4", "smali_classes5"};
    int renamedDirs = 0;
    
    for (const QString &smaliDir : smaliDirs) {
        QString oldPath = m_ProjectPath + "/" + smaliDir + "/" + oldSmaliPath;
        QString newPath = m_ProjectPath + "/" + smaliDir + "/" + newSmaliPath;
        
        if (QDir(oldPath).exists()) {
            // Create new directory structure
            QDir().mkpath(QFileInfo(newPath).absolutePath());
            
            // Move directory
            if (QDir().rename(oldPath, newPath)) {
                renamedDirs++;
            }
        }
    }
    m_AiLog->append(tr("<span style='color: #7ee787;'>✅ Renamed %1 smali directories</span>").arg(renamedDirs));
    
    // 4. Update package references in smali files
    int updatedFiles = 0;
    QString oldSmaliPkg = "L" + oldSmaliPath + "/";
    QString newSmaliPkg = "L" + newSmaliPath + "/";
    
    QDirIterator it(m_ProjectPath, {"*.smali"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString filePath = it.next();
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) continue;
        
        QString content = QString::fromUtf8(file.readAll());
        file.close();
        
        if (content.contains(oldSmaliPkg)) {
            content.replace(oldSmaliPkg, newSmaliPkg);
            
            if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                file.write(content.toUtf8());
                file.close();
                updatedFiles++;
            }
        }
    }
    m_AiLog->append(tr("<span style='color: #7ee787;'>✅ Updated %1 smali files</span>").arg(updatedFiles));
    
    // 5. Update strings.xml if new name provided
    if (!newName.isEmpty()) {
        QString stringsPath = m_ProjectPath + "/res/values/strings.xml";
        QFile stringsFile(stringsPath);
        if (stringsFile.open(QIODevice::ReadOnly)) {
            QString stringsContent = QString::fromUtf8(stringsFile.readAll());
            stringsFile.close();
            
            // Update app_name string
            QRegularExpression appNameRegex("<string name=\"app_name\"[^>]*>([^<]*)</string>");
            stringsContent.replace(appNameRegex, "<string name=\"app_name\">" + newName + "</string>");
            
            if (stringsFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                stringsFile.write(stringsContent.toUtf8());
                stringsFile.close();
                m_AiLog->append("<span style='color: #7ee787;'>✅ Updated app name in strings.xml</span>");
            }
        }
    }
    
    m_AiLog->append("<br><b style='color: #58a6ff;'>🎉 Cloning complete!</b>");
    m_AiLog->append(tr("Recompile the project to generate the cloned APK."));
    
    QMessageBox::information(this, tr("Clone Complete"), 
        tr("Package renamed successfully!\n\nOriginal: %1\nNew: %2\n\nRecompile to generate the cloned APK.")
        .arg(originalPackage, newPackage));
}

void CloningStudio::aiSuggestCloneConfig() {
    m_AiLog->append("<i>[AI] Generating stealth clone configuration...</i>");
    
    if (m_ProjectPath.isEmpty()) {
        m_AiLog->append("<span style='color: #f85149;'>No project loaded.</span>");
        return;
    }
    
    // Read original package from AndroidManifest.xml
    QString manifestPath = m_ProjectPath + "/AndroidManifest.xml";
    QFile manifestFile(manifestPath);
    QString originalPkg = "com.unknown.app";
    QString originalName = "Unknown App";
    
    if (manifestFile.open(QIODevice::ReadOnly)) {
        QString content = QString::fromUtf8(manifestFile.readAll());
        manifestFile.close();
        
        // Extract package
        QRegularExpression pkgRegex("package=\"([^\"]+)\"");
        auto match = pkgRegex.match(content);
        if (match.hasMatch()) {
            originalPkg = match.captured(1);
        }
        
        // Try to extract app name from label
        QRegularExpression labelRegex("android:label=\"([^\"]+)\"");
        match = labelRegex.match(content);
        if (match.hasMatch()) {
            originalName = match.captured(1);
            // Handle @string/app_name reference
            if (originalName.startsWith("@string/")) {
                // Try to read from strings.xml
                QString stringsPath = m_ProjectPath + "/res/values/strings.xml";
                QFile stringsFile(stringsPath);
                if (stringsFile.open(QIODevice::ReadOnly)) {
                    QString stringsContent = QString::fromUtf8(stringsFile.readAll());
                    stringsFile.close();
                    
                    QString stringName = originalName.mid(8); // Remove @string/
                    QRegularExpression stringRegex(QString("<string name=\"%1\"[^>]*>([^<]*)</string>").arg(stringName));
                    auto strMatch = stringRegex.match(stringsContent);
                    if (strMatch.hasMatch()) {
                        originalName = strMatch.captured(1);
                    }
                }
            }
        }
    }
    
    m_AiLog->append(tr("Detected original package: <b>%1</b>").arg(originalPkg));
    m_AiLog->append(tr("Detected app name: <b>%1</b>").arg(originalName));
    
    // Generate stealth clone suggestions
    QString timestamp = QDateTime::currentDateTime().toString("MMddHHmm");
    QStringList suggestions;
    
    // Strategy 1: Add suffix
    suggestions << originalPkg + ".lite";
    suggestions << originalPkg + ".pro";
    suggestions << originalPkg + ".mod";
    
    // Strategy 2: Change TLD
    QStringList parts = originalPkg.split('.');
    if (parts.size() >= 2) {
        parts[0] = "io";
        suggestions << parts.join('.');
        parts[0] = "net";
        suggestions << parts.join('.');
    }
    
    // Strategy 3: Add random suffix for uniqueness
    suggestions << originalPkg + ".v" + timestamp;
    
    // Use first suggestion
    QString suggested = suggestions.first();
    QString suggestedName = originalName + " Mod";
    
    m_EditNewPackage->setText(suggested);
    m_EditNewName->setText(suggestedName);
    
    m_AiLog->append("<br><b style='color: #58a6ff;'>AI Suggestions:</b>");
    for (const QString &s : suggestions) {
        m_AiLog->append(QString("• %1").arg(s));
    }
    m_AiLog->append("<span style='color: #7ee787;'>Selected: " + suggested + "</span>");
}