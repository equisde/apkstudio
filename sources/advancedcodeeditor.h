#ifndef ADVANCEDCODEEDITOR_H
#define ADVANCEDCODEEDITOR_H

#include <QCompleter>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QTimer>
#include <QToolBar>
#include <QWidget>

class CodeEditorSidebar;
class CodeEditorMinimap;
class BreadcrumbBar;

class AdvancedCodeEditor : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit AdvancedCodeEditor(QWidget *parent = nullptr);
    
    // File operations
    QString filePath() const { return m_FilePath; }
    QString fileType() const { return m_FileType; }
    QString encoding() const { return m_Encoding; }
    bool isModified() const { return document()->isModified(); }
    void open(const QString &path);
    bool save();
    bool saveAs(const QString &path);
    void reload();
    
    // Editor features
    void gotoLine(int line);
    void gotoColumn(int col);
    void formatDocument();
    void foldAll();
    void unfoldAll();
    void toggleFold(int line);
    void duplicateLine();
    void deleteLine();
    void deleteWord();
    void moveLineUp();
    void moveLineDown();
    void toggleComment();
    void selectWord();
    void selectLine();
    void selectAll();
    void joinLines();
    void sortLines();
    void removeDuplicateLines();
    void convertTabsToSpaces();
    void convertSpacesToTabs();
    void trimTrailingWhitespace();
    void insertSnippet(const QString &snippet);
    
    // Search & Navigation
    int findText(const QString &text, bool caseSensitive = false, bool wholeWord = false, bool regex = false);
    int replaceText(const QString &find, const QString &replace, bool all = false);
    void findNext();
    void findPrevious();
    QStringList getSymbols();
    void gotoSymbol(const QString &symbol);
    
    // Bookmarks
    void toggleBookmark(int line = -1);
    void nextBookmark();
    void previousBookmark();
    void clearBookmarks();
    QList<int> getBookmarks() const { return m_Bookmarks.values(); }
    
    // Sidebar access
    QRectF blockBoundingGeometryProxy(const QTextBlock &block);
    QRectF blockBoundingRectProxy(const QTextBlock &block);
    QPointF contentOffsetProxy();
    QTextBlock firstVisibleBlockProxy();
    
    // Folding
    bool isLineFolded(int line) const;
    bool isLineFoldable(int line) const;
    bool hasBookmark(int line) const { return m_Bookmarks.contains(line); }
    
    // Statistics
    int wordCount() const;
    int charCount() const;
    int lineCount() const { return blockCount(); }
    
signals:
    void aiResponseReceived(const QString &response);
    void fileTypeChanged(const QString &type);
    void encodingChanged(const QString &encoding);
    void cursorPositionInfo(int line, int column);
    void modifiedChanged(bool modified);
    void wordCountChanged(int count);
    void symbolsChanged(const QStringList &symbols);
    
protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    
private slots:
    void handleBlockCountChanged(int count);
    void handleCursorPositionChanged();
    void handleUpdateRequest(const QRect &rect, int dy);
    void handleTextChanged();
    void handleCompletionSelected(const QString &completion);
    void showCompletions();
    
    // AI actions
    void aiExplainCode();
    void aiFixCode();
    void aiFindIssues();
    void aiOptimizeCode();
    void aiAddComments();
    void aiGenerateCode();
    void aiRefactorCode();
    
private:
    QString m_FilePath;
    QString m_FileType;
    QString m_Encoding;
    QString m_LastSearchText;
    bool m_LastSearchCaseSensitive;
    bool m_LastSearchWholeWord;
    bool m_LastSearchRegex;
    CodeEditorSidebar *m_Sidebar;
    CodeEditorMinimap *m_Minimap;
    QNetworkAccessManager *m_NetworkManager;
    QCompleter *m_Completer;
    QTimer *m_CompletionTimer;
    QTimer *m_SymbolUpdateTimer;
    QSet<int> m_FoldedLines;
    QSet<int> m_Bookmarks;
    QMap<int, int> m_FoldRanges; // start line -> end line
    QStringList m_Symbols;
    
    // Bracket matching
    QList<QTextEdit::ExtraSelection> m_BracketSelections;
    QList<QTextEdit::ExtraSelection> m_SearchSelections;
    
    void detectFileType(const QString &path);
    void detectEncoding(const QByteArray &data);
    void setupHighlighter();
    void updateSidebarWidth();
    void highlightCurrentLine();
    void highlightMatchingBrackets();
    void highlightSearchResults();
    void calculateFoldRanges();
    void extractSymbols();
    
    // Auto-formatting
    QString formatSmali(const QString &code);
    QString formatXml(const QString &code);
    QString formatJson(const QString &code);
    QString formatYaml(const QString &code);
    QString formatJava(const QString &code);
    QString formatDart(const QString &code);
    QString formatHtml(const QString &code);
    QString formatCss(const QString &code);
    QString formatProperties(const QString &code);
    
    // Completions
    QStringList getCompletionsForType();
    void updateCompletions(const QString &prefix);
    
    // AI
    void askAI(const QString &prompt, bool replaceSelection = false);
    void handleAIResponse(QNetworkReply *reply, bool replaceSelection);
    
    // Indentation
    int getIndentLevel(const QString &line);
    QString getIndentString(int level);
    bool shouldIncreaseIndent(const QString &line);
    bool shouldDecreaseIndent(const QString &line);
    void autoIndent();
    
    // Comment detection
    QString getCommentPrefix();
};

// Sidebar with line numbers and fold indicators
class CodeEditorSidebar : public QWidget
{
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

// Minimap for code overview
class CodeEditorMinimap : public QWidget
{
    Q_OBJECT
public:
    explicit CodeEditorMinimap(AdvancedCodeEditor *editor);
    void updateContent();
    
protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    
private:
    AdvancedCodeEditor *m_Editor;
    QImage m_CachedContent;
    bool m_Dragging;
    
    void navigateToPosition(int y);
};

#endif // ADVANCEDCODEEDITOR_H
