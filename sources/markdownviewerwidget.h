#ifndef MARKDOWNVIEWERWIDGET_H
#define MARKDOWNVIEWERWIDGET_H

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
    
private:
    QTextBrowser *m_Browser;
    QString m_FilePath;
    QString m_RawContent;
    
    QString convertMarkdownToHtml(const QString &markdown);
    QString processInlineFormatting(const QString &text);
    QString escapeHtml(const QString &text);
};

#endif // MARKDOWNVIEWERWIDGET_H
