#include "aiconsolewidget.h"
#include <QVBoxLayout>
#include <QScrollBar>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDateTime>

AIConsoleWidget::AIConsoleWidget(QWidget *parent) : QWidget(parent) {
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);

    m_Output = new QTextBrowser();
    m_Output->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4; font-family: 'Consolas'; border: 1px solid #333;");
    m_Output->setHtml("<h2 style='color: #58a6ff;'>APK Studio AI Agent</h2><p>System ready. Ask me to modify code or find vulnerabilities.</p>");
    layout->addWidget(m_Output);

    m_Input = new QLineEdit();
    m_Input->setPlaceholderText(tr("Type a command (e.g. 'Fix SSL Pinning in Smali')..."));
    m_Input->setStyleSheet("background-color: #252526; color: white; padding: 10px; border: none;");
    connect(m_Input, &QLineEdit::returnPressed, this, &AIConsoleWidget::handleCommand);
    layout->addWidget(m_Input);

    m_NetworkManager = new QNetworkAccessManager(this);
}

void AIConsoleWidget::setProjectPath(const QString &path) {
    m_ProjectPath = path;
    logMessage("Agent linked to project: " + path, "system");
}

void AIConsoleWidget::handleCommand() {
    QString cmd = m_Input->text();
    if (cmd.isEmpty()) return;

    logMessage(cmd, "user");
    m_Input->clear();

    QString prompt = QString("You are an AI Agent inside APK Studio Pro. Project Path: %1. User Command: %2. "
                             "Analyze the decompiled sources and provide actionable engineering steps or patches.")
                     .arg(m_ProjectPath, cmd);

    askAI(prompt);
}

void AIConsoleWidget::askAI(const QString &prompt) {
    QSettings settings;
    QString key = settings.value("ai_api_key").toString();
    QString model = settings.value("ai_model", "gemini-2.0-flash-exp").toString();

    if (key.isEmpty()) {
        logMessage("Error: API Key not set in Settings.", "error");
        return;
    }

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
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QString text = doc.object()["candidates"].toArray()[0].toObject()["content"].toObject()["parts"].toArray()[0].toObject()["text"].toString();
            logMessage(text, "ai");
        } else {
            logMessage("AI connection failed: " + reply->errorString(), "error");
        }
        reply->deleteLater();
    });
}

void AIConsoleWidget::logMessage(const QString &msg, const QString &sender) {
    QString color = (sender == "user") ? "#58a6ff" : (sender == "ai") ? "#7ee787" : (sender == "error") ? "#f85149" : "#8b949e";
    QString html = QString("<p><b style='color: %1;'>%2:</b> %3</p>").arg(color, sender.toUpper(), msg);
    m_Output->append(html);
    m_Output->verticalScrollBar()->setValue(m_Output->verticalScrollBar()->maximum());
}