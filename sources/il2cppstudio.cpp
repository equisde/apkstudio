#include "il2cppstudio.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QLabel>
#include <QMessageBox>
#include <QTime>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>

Il2CppStudio::Il2CppStudio(const QString &projectPath, QWidget *parent)
    : QWidget(parent), m_ProjectPath(projectPath)
{
    m_NetworkManager = new QNetworkAccessManager(this);
    setupUI();
}

void Il2CppStudio::setProjectPath(const QString &path) {
    m_ProjectPath = path;
    m_DumpContent.clear();
    m_AnalysisReport->clear();
    logMessage("Project re-indexed. Ready for new dump.");
}

void Il2CppStudio::setupUI() {
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);

    auto header = new QLabel(tr("<b>IL2CPP STUDIO OMEGA</b>"));
    header->setStyleSheet("color: #58a6ff; font-size: 14px;");
    layout->addWidget(header);

    auto btnLoad = new QPushButton(tr("📂 Load dump.cs / script.py"));
    connect(btnLoad, &QPushButton::clicked, this, &Il2CppStudio::loadDumpFile);
    layout->addWidget(btnLoad);

    auto groupTools = new QGroupBox(tr("35+ AI Modding Tools"));
    auto toolsLayout = new QVBoxLayout(groupTools);
    m_ToolsList = new QListWidget();
    m_ToolsList->setStyleSheet("background-color: #161b22; color: #c9d1d9; border: none; min-height: 200px;");
    
    m_Tools = AIToolFactory::getAllTools();
    for(const auto &tool : m_Tools) {
        auto item = new QListWidgetItem(tool.name);
        item->setToolTip(tool.description);
        item->setData(Qt::UserRole, tool.id);
        m_ToolsList->addItem(item);
    }
    toolsLayout->addWidget(m_ToolsList);
    
    auto btnRun = new QPushButton(tr("🚀 Analyze with AI Engine"));
    btnRun->setStyleSheet("background-color: #238636; color: white; padding: 10px;");
    connect(btnRun, &QPushButton::clicked, this, &Il2CppStudio::runAiTool);
    toolsLayout->addWidget(btnRun);
    layout->addWidget(groupTools);

    m_AnalysisReport = new QTextBrowser();
    m_AnalysisReport->setStyleSheet("background-color: #0d1117; color: #d1d5da; font-family: monospace;");
    layout->addWidget(new QLabel(tr("<b>AI Analysis Output</b>")));
    layout->addWidget(m_AnalysisReport);

    auto btnMenu = new QPushButton(tr("🛠️ Build Floating Mod Menu"));
    btnMenu->setStyleSheet("background-color: #1f6feb; color: white; font-weight: bold; padding: 10px;");
    connect(btnMenu, &QPushButton::clicked, this, &Il2CppStudio::generateModMenu);
    layout->addWidget(btnMenu);
}

void Il2CppStudio::loadDumpFile() {
    QString path = QFileDialog::getOpenFileName(this, tr("Select Dump"), m_ProjectPath, "Files (*.cs *.py *.txt)");
    if(!path.isEmpty()) {
        QFile f(path);
        if(f.open(QFile::ReadOnly)) {
            m_DumpContent = f.readAll();
            f.close();
            logMessage("Target loaded. Engine ready for extraction.");
        }
    }
}

void Il2CppStudio::runAiTool() {
    auto item = m_ToolsList->currentItem();
    if(!item || m_DumpContent.isEmpty()) {
        QMessageBox::warning(this, "Studio", "Load dump.cs first.");
        return;
    }

    QString id = item->data(Qt::UserRole).toString();
    AITool selectedTool;
    for(const auto &t : m_Tools) if(t.id == id) selectedTool = t;

    logMessage("IA analyzing: " + selectedTool.name);
    
    // Tomar fragmentos clave del dump para no exceder límites
    QString context = m_DumpContent.mid(0, 20000); 
    QString prompt = QString("As an Android RE expert, analyze this Il2Cpp dump. "
                             "Task: %1. Return ONLY a JSON array of offsets and hex patches.")
                     .arg(selectedTool.aiPrompt);

    askAI(prompt + "\n\nDump:\n" + context, [this](const QString &res) {
        m_AnalysisReport->append("<h3>Results</h3><pre>" + res + "</pre>");
        
        QFile f(m_ProjectPath + "/AI_BINARY_PATCHES.json");
        if(f.open(QFile::WriteOnly)) {
            f.write(res.toUtf8());
            f.close();
            logMessage("Patches saved to AI_BINARY_PATCHES.json");
        }
    });
}

void Il2CppStudio::generateModMenu() {
    logMessage("IA is architecting the Mod Menu source code...");
    askAI("Generate a complete C++ Mod Menu source using ImGui based on typical Il2Cpp offsets.", [this](const QString &code) {
        m_AnalysisReport->append("<h2>Generated Mod Menu Code</h2><pre>" + code + "</pre>");
        QFile f(m_ProjectPath + "/AI_MOD_MENU.cpp");
        if(f.open(QFile::WriteOnly)) { f.write(code.toUtf8()); f.close(); }
    });
}

void Il2CppStudio::askAI(const QString &prompt, std::function<void(const QString&)> callback) {
    QSettings settings;
    QString key = settings.value("ai_api_key").toString();
    QString model = settings.value("ai_model", "gemini-2.0-flash-exp").toString();
    
    QJsonObject root;
    QJsonArray contents;
    QJsonObject content;
    QJsonArray parts;
    QJsonObject part;
    part["text"] = prompt;
    parts.append(part);
    content["parts"] = parts;
    contents.append(content);
    root["contents"] = contents;

    QNetworkRequest req;
    req.setUrl(QUrl(QString("https://generativelanguage.googleapis.com/v1beta/models/%1:generateContent?key=%2").arg(model, key)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = m_NetworkManager->post(req, QJsonDocument(root).toJson());
    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QString text = doc.object()["candidates"].toArray()[0].toObject()["content"].toObject()["parts"].toArray()[0].toObject()["text"].toString();
            callback(text);
        }
        reply->deleteLater();
    });
}

void Il2CppStudio::logMessage(const QString &msg) {
    m_AnalysisReport->append("<span style='color: #8b949e;'>[" + QTime::currentTime().toString() + "]</span> " + msg);
}
