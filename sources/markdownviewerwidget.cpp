#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>
#include <QVBoxLayout>
#include "markdownviewerwidget.h"

MarkdownViewerWidget::MarkdownViewerWidget(QWidget *parent)
    : QWidget(parent)
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    m_Browser = new QTextBrowser(this);
    m_Browser->setOpenExternalLinks(true);
    m_Browser->setFrameStyle(QFrame::NoFrame);
    
    // Modern styling
    m_Browser->setStyleSheet(R"(
        QTextBrowser {
            background-color: #fafafa;
            color: #333;
            padding: 20px;
            font-family: 'Segoe UI', 'SF Pro Display', 'Helvetica Neue', sans-serif;
            font-size: 14px;
            line-height: 1.6;
        }
    )");
    
    layout->addWidget(m_Browser);
}

QString MarkdownViewerWidget::filePath() const
{
    return m_FilePath;
}

void MarkdownViewerWidget::open(const QString &path)
{
    m_FilePath = path;
    
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_Browser->setHtml("<p style='color: red;'>Failed to open file.</p>");
        return;
    }
    
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    m_RawContent = in.readAll();
    file.close();
    
    QString html = convertMarkdownToHtml(m_RawContent);
    m_Browser->setHtml(html);
}

bool MarkdownViewerWidget::save()
{
    // Markdown viewer is read-only for now
    return true;
}

QString MarkdownViewerWidget::escapeHtml(const QString &text)
{
    QString result = text;
    result.replace("&", "&amp;");
    result.replace("<", "&lt;");
    result.replace(">", "&gt;");
    return result;
}

QString MarkdownViewerWidget::convertMarkdownToHtml(const QString &markdown)
{
    QString html;
    QTextStream out(&html);
    
    // CSS styling for markdown
    out << R"(
<!DOCTYPE html>
<html>
<head>
<style>
body {
    font-family: 'Segoe UI', 'SF Pro Display', -apple-system, BlinkMacSystemFont, sans-serif;
    font-size: 14px;
    line-height: 1.7;
    color: #24292e;
    max-width: 900px;
    margin: 0 auto;
    padding: 20px;
    background: #fff;
}
h1 { font-size: 2em; border-bottom: 2px solid #eaecef; padding-bottom: 0.3em; margin-top: 24px; margin-bottom: 16px; font-weight: 600; }
h2 { font-size: 1.5em; border-bottom: 1px solid #eaecef; padding-bottom: 0.3em; margin-top: 24px; margin-bottom: 16px; font-weight: 600; }
h3 { font-size: 1.25em; margin-top: 24px; margin-bottom: 16px; font-weight: 600; }
h4 { font-size: 1em; margin-top: 24px; margin-bottom: 16px; font-weight: 600; }
p { margin-top: 0; margin-bottom: 16px; }
code {
    font-family: 'Cascadia Code', 'JetBrains Mono', 'Fira Code', Consolas, monospace;
    font-size: 0.9em;
    background-color: #f6f8fa;
    border-radius: 6px;
    padding: 0.2em 0.4em;
}
pre {
    font-family: 'Cascadia Code', 'JetBrains Mono', 'Fira Code', Consolas, monospace;
    font-size: 0.85em;
    background-color: #f6f8fa;
    border-radius: 6px;
    padding: 16px;
    overflow: auto;
    line-height: 1.5;
}
pre code {
    background: none;
    padding: 0;
}
blockquote {
    margin: 0;
    padding: 0 1em;
    color: #6a737d;
    border-left: 4px solid #dfe2e5;
}
ul, ol { padding-left: 2em; margin-top: 0; margin-bottom: 16px; }
li { margin-top: 0.25em; }
li + li { margin-top: 0.25em; }
hr { border: 0; height: 4px; background: #e1e4e8; margin: 24px 0; }
a { color: #0366d6; text-decoration: none; }
a:hover { text-decoration: underline; }
strong { font-weight: 600; }
em { font-style: italic; }
table { border-collapse: collapse; width: 100%; margin-bottom: 16px; }
th, td { border: 1px solid #dfe2e5; padding: 6px 13px; }
th { background-color: #f6f8fa; font-weight: 600; }
tr:nth-child(2n) { background-color: #f6f8fa; }
.warning { background: #fff3cd; border-left: 4px solid #ffc107; padding: 12px; margin: 16px 0; border-radius: 4px; }
.info { background: #d1ecf1; border-left: 4px solid #17a2b8; padding: 12px; margin: 16px 0; border-radius: 4px; }
.success { background: #d4edda; border-left: 4px solid #28a745; padding: 12px; margin: 16px 0; border-radius: 4px; }
.danger { background: #f8d7da; border-left: 4px solid #dc3545; padding: 12px; margin: 16px 0; border-radius: 4px; }
</style>
</head>
<body>
)";
    
    QStringList lines = markdown.split('\n');
    bool inCodeBlock = false;
    bool inList = false;
    QString listType;
    
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i];
        
        // Code blocks
        if (line.startsWith("```")) {
            if (inCodeBlock) {
                out << "</code></pre>\n";
                inCodeBlock = false;
            } else {
                out << "<pre><code>";
                inCodeBlock = true;
            }
            continue;
        }
        
        if (inCodeBlock) {
            out << escapeHtml(line) << "\n";
            continue;
        }
        
        // Close list if line is empty or doesn't continue list
        if (inList && line.trimmed().isEmpty()) {
            out << (listType == "ul" ? "</ul>\n" : "</ol>\n");
            inList = false;
        }
        
        // Headers
        if (line.startsWith("##### ")) {
            out << "<h5>" << escapeHtml(line.mid(6)) << "</h5>\n";
        } else if (line.startsWith("#### ")) {
            out << "<h4>" << escapeHtml(line.mid(5)) << "</h4>\n";
        } else if (line.startsWith("### ")) {
            out << "<h3>" << escapeHtml(line.mid(4)) << "</h3>\n";
        } else if (line.startsWith("## ")) {
            out << "<h2>" << escapeHtml(line.mid(3)) << "</h2>\n";
        } else if (line.startsWith("# ")) {
            out << "<h1>" << escapeHtml(line.mid(2)) << "</h1>\n";
        }
        // Horizontal rule
        else if (line.trimmed() == "---" || line.trimmed() == "***" || line.trimmed() == "___") {
            out << "<hr>\n";
        }
        // Blockquote
        else if (line.startsWith("> ")) {
            out << "<blockquote>" << escapeHtml(line.mid(2)) << "</blockquote>\n";
        }
        // Unordered list
        else if (line.startsWith("- ") || line.startsWith("* ") || line.startsWith("+ ")) {
            if (!inList || listType != "ul") {
                if (inList) out << (listType == "ul" ? "</ul>\n" : "</ol>\n");
                out << "<ul>\n";
                inList = true;
                listType = "ul";
            }
            QString content = line.mid(2);
            // Process inline formatting
            content = processInlineFormatting(content);
            out << "<li>" << content << "</li>\n";
        }
        // Ordered list
        else if (QRegularExpression("^\\d+\\. ").match(line).hasMatch()) {
            if (!inList || listType != "ol") {
                if (inList) out << (listType == "ul" ? "</ul>\n" : "</ol>\n");
                out << "<ol>\n";
                inList = true;
                listType = "ol";
            }
            int dotPos = line.indexOf(". ");
            QString content = line.mid(dotPos + 2);
            content = processInlineFormatting(content);
            out << "<li>" << content << "</li>\n";
        }
        // Empty line
        else if (line.trimmed().isEmpty()) {
            out << "\n";
        }
        // Paragraph
        else {
            QString content = processInlineFormatting(line);
            out << "<p>" << content << "</p>\n";
        }
    }
    
    if (inList) {
        out << (listType == "ul" ? "</ul>\n" : "</ol>\n");
    }
    if (inCodeBlock) {
        out << "</code></pre>\n";
    }
    
    out << "</body></html>";
    
    return html;
}

QString MarkdownViewerWidget::processInlineFormatting(const QString &text)
{
    QString result = escapeHtml(text);
    
    // Bold: **text** or __text__
    result.replace(QRegularExpression("\\*\\*(.+?)\\*\\*"), "<strong>\\1</strong>");
    result.replace(QRegularExpression("__(.+?)__"), "<strong>\\1</strong>");
    
    // Italic: *text* or _text_
    result.replace(QRegularExpression("\\*(.+?)\\*"), "<em>\\1</em>");
    result.replace(QRegularExpression("_(.+?)_"), "<em>\\1</em>");
    
    // Inline code: `code`
    result.replace(QRegularExpression("`([^`]+)`"), "<code>\\1</code>");
    
    // Links: [text](url)
    result.replace(QRegularExpression("\\[([^\\]]+)\\]\\(([^)]+)\\)"), "<a href=\"\\2\">\\1</a>");
    
    // Strikethrough: ~~text~~
    result.replace(QRegularExpression("~~(.+?)~~"), "<del>\\1</del>");
    
    return result;
}
