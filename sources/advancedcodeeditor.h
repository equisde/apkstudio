#ifndef ADVANCEDCODEEDITOR_H
#define ADVANCEDCODEEDITOR_H

#include <QPlainTextEdit>
#include <QColor>
#include <QPainter>
#include <QTextBlock>
#include <QCompleter>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class CodeEditorSidebar;
class CodeEditorMinimap;

class AdvancedCodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit AdvancedCodeEditor(QWidget *parent = nullptr);
    void open(const QString &path);
    void save();
    
    // VS Code Features
    void toggleFolding(int line);
    void highlightCurrentLine();
    void setLanguage(const QString &lang);

    // AI Integration
    void requestAiPatch(const QString &instruction);
    void explainCode();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    void updateSidebar(const QRect &rect, int dy);
    void updateMinimap();
    void handleAiResponse();

private:
    CodeEditorSidebar *m_Sidebar;
    CodeEditorMinimap *m_Minimap;
    QNetworkAccessManager *m_AiManager;
    QString m_CurrentLanguage;
    QString m_FilePath;
};

class CodeEditorSidebar : public QWidget {
    Q_OBJECT
public:
    CodeEditorSidebar(AdvancedCodeEditor *editor);
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    AdvancedCodeEditor *m_Editor;
};

class CodeEditorMinimap : public QWidget {
    Q_OBJECT
public:
    CodeEditorMinimap(AdvancedCodeEditor *editor);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    AdvancedCodeEditor *m_Editor;
};

#endif // ADVANCEDCODEEDITOR_H