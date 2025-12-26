#ifndef MARKDOWNVIEWERWIDGET_H
#define MARKDOWNVIEWERWIDGET_H

#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QTextBrowser>
#include <QWidget>

class MarkdownViewerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MarkdownViewerWidget(QWidget *parent = nullptr);
    QString filePath() const;
    void open(const QString &path);
    bool save();
    bool isSourceMode() const { return m_SourceMode; }
    
public slots:
    void toggleViewMode();
    
private:
    QStackedWidget *m_Stack;
    QTextBrowser *m_Browser;
    QPlainTextEdit *m_SourceEditor;
    QPushButton *m_ToggleButton;
    QString m_FilePath;
    QString m_RawContent;
    bool m_SourceMode;
    
    QString convertMarkdownToHtml(const QString &markdown);
    QString processInlineFormatting(const QString &text);
    QString escapeHtml(const QString &text);
    void updatePreview();
};

#endif // MARKDOWNVIEWERWIDGET_H
