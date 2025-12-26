#ifndef ADVANCEDCODEEDITOR_H
#define ADVANCEDCODEEDITOR_H

#include <QPlainTextEdit>
#include <QColor>
#include <QPainter>
#include <QTextBlock>
#include <QCompleter>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QSet>
#include <QMap>

class CodeEditorSidebar;
class CodeEditorMinimap;

class AdvancedCodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit AdvancedCodeEditor(QWidget *parent = nullptr);
    void open(const QString &path);
    bool save();
    bool saveAs(const QString &path);
    void reload();
    
    QString filePath() const { return m_FilePath; }
    void gotoLine(int line);
    void gotoColumn(int col);
    void gotoSymbol(const QString &symbol);
    
    // VS Code Features
    void formatDocument();
    void duplicateLine();
    void deleteLine();
    void deleteWord();
    void moveLineUp();
    void moveLineDown();
    void toggleComment();
    
    void toggleFold(int line);
    void foldAll();
    void unfoldAll();
    bool isLineFoldable(int line) const;
    bool isLineFolded(int line) const;
    
    void selectWord();
    void selectLine();
    void selectAllText();
    
    void joinLines();
    void sortLines();
    void removeDuplicateLines();
    
    void trimTrailingWhitespace();
    void convertTabsToSpaces();
    void convertSpacesToTabs();
    
    void toggleBookmark(int line = -1);
    void nextBookmark();
    void previousBookmark();
    void clearBookmarks();
    bool hasBookmark(int line) const { return m_Bookmarks.contains(line); }
    
    void insertSnippet(const QString &snippet);
    
    int findText(const QString &text, bool caseSensitive, bool wholeWord, bool regex);
    int replaceText(const QString &find, const QString &replace, bool all);
    void findNext();
    void findPrevious();
    
    QStringList getSymbols();
    int wordCount() const;
    int charCount() const;

    // AI Integration
    void aiExplainCode();
    void aiFixCode();
    void aiFindIssues();
    void aiOptimizeCode();
    void aiAddComments();
    void aiGenerateCode();
    void aiRefactorCode();

    // Proxies for sub-widgets
    QRectF blockBoundingGeometryProxy(const QTextBlock &block);
    QRectF blockBoundingRectProxy(const QTextBlock &block);
    QPointF contentOffsetProxy();
    QTextBlock firstVisibleBlockProxy();

signals:
    void fileTypeChanged(const QString &type);
    void encodingChanged(const QString &encoding);
    void cursorPositionInfo(int line, int col);
    void wordCountChanged(int count);
    void symbolsChanged(const QStringList &symbols);
    void aiResponseReceived(const QString &response);
    void modifiedChanged(bool modified);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;

private slots:
    void handleBlockCountChanged(int count);
    void handleCursorPositionChanged();
    void handleUpdateRequest(const QRect &rect, int dy);
    void handleTextChanged();
    void showCompletions();
    void handleCompletionSelected(const QString &completion);
    void extractSymbols();

private:
    void updateSidebarWidth();
    void setupHighlighter();
    void detectFileType(const QString &path);
    void detectEncoding(const QByteArray &data);
    void calculateFoldRanges();
    void highlightCurrentLine();
    void highlightMatchingBrackets();
    void highlightSearchResults();
    
    QString getCommentPrefix();
    QStringList getCompletionsForType();
    
    // Formatters
    QString formatXml(const QString &code);
    QString formatJson(const QString &code);
    QString formatYaml(const QString &code);
    QString formatSmali(const QString &code);
    QString formatJava(const QString &code);
    QString formatDart(const QString &code);
    QString formatHtml(const QString &code);
    QString formatCss(const QString &code);
    QString formatProperties(const QString &code);

    void askAI(const QString &prompt, bool replaceSelection);
    void handleAIResponse(QNetworkReply *reply, bool replaceSelection);

    CodeEditorSidebar *m_Sidebar;
    CodeEditorMinimap *m_Minimap;
    QNetworkAccessManager *m_NetworkManager;
    QCompleter *m_Completer;
    QTimer *m_CompletionTimer;
    QTimer *m_SymbolUpdateTimer;
    
    QString m_FilePath;
    QString m_FileType;
    QString m_Encoding;
    
    QMap<int, int> m_FoldRanges;
    QSet<int> m_FoldedLines;
    QSet<int> m_Bookmarks;
    QStringList m_Symbols;
    
    QList<QTextEdit::ExtraSelection> m_BracketSelections;
    QList<QTextEdit::ExtraSelection> m_SearchSelections;
    
    QString m_LastSearchText;
    bool m_LastSearchCaseSensitive;
    bool m_LastSearchWholeWord;
    bool m_LastSearchRegex;
};

class CodeEditorSidebar : public QWidget {
    Q_OBJECT
public:
    explicit CodeEditorSidebar(AdvancedCodeEditor *editor);
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
private:
    AdvancedCodeEditor *m_Editor;
};

class CodeEditorMinimap : public QWidget {
    Q_OBJECT
public:
    explicit CodeEditorMinimap(AdvancedCodeEditor *editor);
    void updateContent();
protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
private:
    void navigateToPosition(int y);
    AdvancedCodeEditor *m_Editor;
    QImage m_CachedContent;
    bool m_Dragging;
};

#endif // ADVANCEDCODEEDITOR_H
