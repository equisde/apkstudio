#include <QApplication>
#include <QContextMenuEvent>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QPainter>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSettings>
#include <QShortcut>
#include <QStack>
#include <QStringListModel>
#include <QTextBlock>
#include <QTextStream>
#include <QToolTip>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#endif
#include "advancedcodeeditor.h"
#include "themedsyntaxhighlighter.h"

#define TAB_SIZE 4
#define MINIMAP_WIDTH 100
#define SIDEBAR_FOLD_WIDTH 16

AdvancedCodeEditor::AdvancedCodeEditor(QWidget *parent)
    : QPlainTextEdit(parent), m_NetworkManager(nullptr), m_Completer(nullptr),
      m_LastSearchCaseSensitive(false), m_LastSearchWholeWord(false), m_LastSearchRegex(false)
{
    m_Sidebar = new CodeEditorSidebar(this);
    m_Minimap = new CodeEditorMinimap(this);
    m_NetworkManager = new QNetworkAccessManager(this);
    m_CompletionTimer = new QTimer(this);
    m_CompletionTimer->setSingleShot(true);
    m_CompletionTimer->setInterval(300);
    
    // Symbol update timer
    m_SymbolUpdateTimer = new QTimer(this);
    m_SymbolUpdateTimer->setSingleShot(true);
    m_SymbolUpdateTimer->setInterval(1000);
    connect(m_SymbolUpdateTimer, &QTimer::timeout, this, &AdvancedCodeEditor::extractSymbols);
    
    // Modern font setup
    QSettings settings;
    QFont font;
#ifdef Q_OS_WIN
    font.setFamily(settings.value("editor_font", "Cascadia Code").toString());
#elif defined(Q_OS_MACOS)
    font.setFamily(settings.value("editor_font", "SF Mono").toString());
#else
    font.setFamily(settings.value("editor_font", "JetBrains Mono").toString());
#endif
    font.setFixedPitch(true);
    font.setPointSize(settings.value("editor_font_size", 12).toInt());
    font.setStyleHint(QFont::Monospace);
    setFont(font);
    
    // Tab settings
    QFontMetrics metrics(font);
    setTabStopDistance(TAB_SIZE * metrics.horizontalAdvance(' '));
    
    // Editor settings
    setCursorWidth(2);
    setFrameStyle(QFrame::NoFrame);
    setWordWrapMode(QTextOption::NoWrap);
    setLineWrapMode(QPlainTextEdit::NoWrap);
    
    // Show whitespace if enabled
    if (settings.value("editor_whitespaces", false).toBool()) {
        QTextOption options;
        options.setFlags(QTextOption::ShowTabsAndSpaces);
        document()->setDefaultTextOption(options);
    }
    
    // Connections
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &AdvancedCodeEditor::handleCursorPositionChanged);
    connect(this, &QPlainTextEdit::blockCountChanged, this, &AdvancedCodeEditor::handleBlockCountChanged);
    connect(this, &QPlainTextEdit::textChanged, this, &AdvancedCodeEditor::handleTextChanged);
    connect(this, &QPlainTextEdit::updateRequest, this, &AdvancedCodeEditor::handleUpdateRequest);
    connect(m_CompletionTimer, &QTimer::timeout, this, &AdvancedCodeEditor::showCompletions);
    connect(document(), &QTextDocument::modificationChanged, this, &AdvancedCodeEditor::modifiedChanged);
    
    // Keyboard shortcuts
    new QShortcut(Qt::CTRL | Qt::Key_D, this, [this]() { duplicateLine(); });
    new QShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_K, this, [this]() { deleteLine(); });
    new QShortcut(Qt::ALT | Qt::Key_Up, this, [this]() { moveLineUp(); });
    new QShortcut(Qt::ALT | Qt::Key_Down, this, [this]() { moveLineDown(); });
    new QShortcut(Qt::CTRL | Qt::Key_Slash, this, [this]() { toggleComment(); });
    new QShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_F, this, [this]() { formatDocument(); });
    new QShortcut(Qt::CTRL | Qt::Key_J, this, [this]() { joinLines(); });
    new QShortcut(Qt::CTRL | Qt::Key_L, this, [this]() { selectLine(); });
    new QShortcut(Qt::CTRL | Qt::Key_W, this, [this]() { selectWord(); });
    new QShortcut(Qt::CTRL | Qt::Key_M, this, [this]() { toggleBookmark(); });
    new QShortcut(Qt::Key_F2, this, [this]() { nextBookmark(); });
    new QShortcut(Qt::SHIFT | Qt::Key_F2, this, [this]() { previousBookmark(); });
    new QShortcut(Qt::Key_F3, this, [this]() { findNext(); });
    new QShortcut(Qt::SHIFT | Qt::Key_F3, this, [this]() { findPrevious(); });
    new QShortcut(Qt::CTRL | Qt::Key_U, this, [this]() {
        QTextCursor c = textCursor();
        if (c.hasSelection()) {
            c.insertText(c.selectedText().toUpper());
        }
    });
    new QShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_U, this, [this]() {
        QTextCursor c = textCursor();
        if (c.hasSelection()) {
            c.insertText(c.selectedText().toLower());
        }
    });
    new QShortcut(Qt::CTRL | Qt::Key_Backspace, this, [this]() { deleteWord(); });
    
    // Setup completer
    m_Completer = new QCompleter(this);
    m_Completer->setWidget(this);
    m_Completer->setCompletionMode(QCompleter::PopupCompletion);
    m_Completer->setCaseSensitivity(Qt::CaseInsensitive);
    connect(m_Completer, QOverload<const QString &>::of(&QCompleter::activated),
            this, &AdvancedCodeEditor::handleCompletionSelected);
    
    handleBlockCountChanged(0);
}

void AdvancedCodeEditor::open(const QString &path)
{
    QFile file(path);
    if (!file.open(QFile::ReadOnly)) {
        return;
    }
    
    QByteArray rawData = file.readAll();
    file.close();
    
    detectEncoding(rawData);
    detectFileType(path);
    
    QString content = QString::fromUtf8(rawData);
    setPlainText(content);
    
    m_FilePath = path;
    setupHighlighter();
    calculateFoldRanges();
    m_Minimap->updateContent();
    
    emit fileTypeChanged(m_FileType);
    emit encodingChanged(m_Encoding);
}

bool AdvancedCodeEditor::save()
{
    if (m_FilePath.isEmpty()) return false;
    
    QFile file(m_FilePath);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        return false;
    }
    
    QTextStream out(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    out.setEncoding(QStringConverter::Utf8);
#endif
    out.setGenerateByteOrderMark(false);
    out << toPlainText();
    out.flush();
    file.close();
    
    document()->setModified(false);
    return true;
}

void AdvancedCodeEditor::detectFileType(const QString &path)
{
    QFileInfo info(path);
    QString ext = info.suffix().toLower();
    QString name = info.fileName().toLower();
    
    // Map extensions to file types
    static QMap<QString, QString> extMap = {
        {"smali", "smali"}, {"java", "java"}, {"kt", "kotlin"}, {"kts", "kotlin"},
        {"xml", "xml"}, {"json", "json"}, {"yml", "yaml"}, {"yaml", "yaml"},
        {"properties", "properties"}, {"txt", "text"}, {"md", "markdown"},
        {"html", "html"}, {"htm", "html"}, {"css", "css"}, {"js", "javascript"},
        {"ts", "typescript"}, {"dart", "dart"}, {"swift", "swift"},
        {"m", "objective-c"}, {"mm", "objective-c"}, {"h", "c-header"},
        {"c", "c"}, {"cpp", "cpp"}, {"hpp", "cpp"}, {"py", "python"},
        {"rb", "ruby"}, {"go", "go"}, {"rs", "rust"}, {"sql", "sql"},
        {"sh", "shell"}, {"bash", "shell"}, {"zsh", "shell"},
        {"gradle", "gradle"}, {"pro", "qmake"}, {"cmake", "cmake"},
        {"ini", "ini"}, {"toml", "toml"}, {"cfg", "config"},
        {"arsc", "binary"}, {"dex", "binary"}, {"so", "binary"}
    };
    
    // Special file names
    if (name == "androidmanifest.xml") m_FileType = "android-manifest";
    else if (name == "build.gradle") m_FileType = "gradle";
    else if (name == "pubspec.yaml") m_FileType = "flutter-pubspec";
    else if (name == "apktool.yml") m_FileType = "apktool-config";
    else if (extMap.contains(ext)) m_FileType = extMap[ext];
    else m_FileType = "text";
}

void AdvancedCodeEditor::detectEncoding(const QByteArray &data)
{
    if (data.startsWith("\xEF\xBB\xBF")) m_Encoding = "UTF-8-BOM";
    else if (data.startsWith("\xFF\xFE")) m_Encoding = "UTF-16LE";
    else if (data.startsWith("\xFE\xFF")) m_Encoding = "UTF-16BE";
    else if (data.startsWith("\x00\x00\xFE\xFF")) m_Encoding = "UTF-32BE";
    else m_Encoding = "UTF-8";
}

void AdvancedCodeEditor::setupHighlighter()
{
    QSettings settings;
    bool dark = settings.value("dark_theme", false).toBool();
    QFileInfo info(m_FilePath);
    QString ext = info.suffix().toLower();
    
    // Unified mapping for syntax definitions
    static QMap<QString, QString> langMap = {
        {"kt", "kotlin"}, {"kts", "kotlin"},
        {"js", "javascript"}, {"ts", "typescript"},
        {"py", "python"}, {"pyw", "python"},
        {"sh", "shell"}, {"bash", "shell"}, {"zsh", "shell"},
        {"yml", "yaml"}, {"yaml", "yaml"},
        {"h", "cpp"}, {"hpp", "cpp"}, {"c", "cpp"}, {"cpp", "cpp"},
        {"rs", "rust"}, {"go", "go"}, {"sql", "sql"}
    };
    
    QString lang = langMap.value(ext, ext);
    if (lang == "htm") lang = "html";

    new ThemedSyntaxHighlighter(
        ThemedSyntaxHighlighter::theme(dark ? "dark" : "light"),
        ThemedSyntaxHighlighter::definitions(lang),
        document()
    );
}

void AdvancedCodeEditor::gotoLine(int line)
{
    QTextCursor cursor(document()->findBlockByLineNumber(line - 1));
    setTextCursor(cursor);
    centerCursor();
}

void AdvancedCodeEditor::formatDocument()
{
    QString content = toPlainText();
    QString formatted;
    
    if (m_FileType == "xml" || m_FileType == "android-manifest") {
        formatted = formatXml(content);
    } else if (m_FileType == "json") {
        formatted = formatJson(content);
    } else if (m_FileType == "yaml" || m_FileType == "flutter-pubspec" || m_FileType == "apktool-config") {
        formatted = formatYaml(content);
    } else if (m_FileType == "smali") {
        formatted = formatSmali(content);
    } else if (m_FileType == "java" || m_FileType == "kotlin") {
        formatted = formatJava(content);
    } else if (m_FileType == "dart") {
        formatted = formatDart(content);
    } else if (m_FileType == "html") {
        formatted = formatHtml(content);
    } else if (m_FileType == "css") {
        formatted = formatCss(content);
    } else if (m_FileType == "properties") {
        formatted = formatProperties(content);
    } else {
        QToolTip::showText(mapToGlobal(cursorRect().topLeft()), 
            tr("No formatter available for %1 files").arg(m_FileType), this);
        return;
    }
    
    if (!formatted.isEmpty() && formatted != content) {
        QTextCursor cursor = textCursor();
        cursor.beginEditBlock();
        cursor.select(QTextCursor::Document);
        cursor.insertText(formatted);
        cursor.endEditBlock();
        QToolTip::showText(mapToGlobal(cursorRect().topLeft()), tr("✓ Document formatted"), this);
    }
}

QString AdvancedCodeEditor::formatXml(const QString &code)
{
    QString result;
    QTextStream out(&result);
    int indent = 0;
    bool inTag = false;
    bool inQuote = false;
    QChar quoteChar;
    QString currentTag;
    
    for (int i = 0; i < code.length(); ++i) {
        QChar c = code[i];
        
        if (inQuote) {
            currentTag += c;
            if (c == quoteChar) inQuote = false;
            continue;
        }
        
        if (c == '"' || c == '\'') {
            inQuote = true;
            quoteChar = c;
            currentTag += c;
            continue;
        }
        
        if (c == '<') {
            if (!currentTag.trimmed().isEmpty()) {
                out << currentTag.trimmed();
            }
            currentTag = "<";
            inTag = true;
        } else if (c == '>') {
            currentTag += ">";
            inTag = false;
            
            bool isClosing = currentTag.startsWith("</");
            bool isSelfClosing = currentTag.endsWith("/>");
            bool isDeclaration = currentTag.startsWith("<?") || currentTag.startsWith("<!");
            
            if (isClosing) indent = qMax(0, indent - 1);
            
            out << QString(indent * 2, ' ') << currentTag.trimmed() << "\n";
            
            if (!isClosing && !isSelfClosing && !isDeclaration) indent++;
            
            currentTag.clear();
        } else {
            currentTag += c;
        }
    }
    
    if (!currentTag.trimmed().isEmpty()) {
        out << currentTag.trimmed();
    }
    
    return result;
}

QString AdvancedCodeEditor::formatJson(const QString &code)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(code.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        QToolTip::showText(mapToGlobal(cursorRect().topLeft()), 
            tr("JSON parse error: %1").arg(error.errorString()), this);
        return QString();
    }
    return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}

QString AdvancedCodeEditor::formatYaml(const QString &code)
{
    // Simple YAML formatting - normalize indentation
    QStringList lines = code.split('\n');
    QString result;
    
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) {
            result += "\n";
            continue;
        }
        
        // Count leading spaces
        int spaces = 0;
        for (QChar c : line) {
            if (c == ' ') spaces++;
            else if (c == '\t') spaces += 2;
            else break;
        }
        
        // Normalize to 2-space indentation
        int level = spaces / 2;
        result += QString(level * 2, ' ') + trimmed + "\n";
    }
    
    return result;
}

QString AdvancedCodeEditor::formatSmali(const QString &code)
{
    QStringList lines = code.split('\n');
    QString result;
    int indent = 0;
    
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        
        if (trimmed.isEmpty()) {
            result += "\n";
            continue;
        }
        
        // Decrease indent for end markers
        if (trimmed.startsWith(".end ")) {
            indent = qMax(0, indent - 1);
        }
        
        // Add indentation
        result += QString(indent * 4, ' ') + trimmed + "\n";
        
        // Increase indent for start markers
        if (trimmed.startsWith(".method ") || trimmed.startsWith(".annotation ") ||
            trimmed.startsWith(".subannotation ") || trimmed.startsWith(".array-data ") ||
            trimmed.startsWith(".packed-switch ") || trimmed.startsWith(".sparse-switch ")) {
            indent++;
        }
    }
    
    return result;
}

QString AdvancedCodeEditor::formatJava(const QString &code)
{
    // Basic Java/Kotlin formatting
    QStringList lines = code.split('\n');
    QString result;
    int indent = 0;
    
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        
        if (trimmed.isEmpty()) {
            result += "\n";
            continue;
        }
        
        // Decrease indent for closing braces
        if (trimmed.startsWith("}") || trimmed.startsWith(")")) {
            indent = qMax(0, indent - 1);
        }
        
        result += QString(indent * 4, ' ') + trimmed + "\n";
        
        // Increase indent for opening braces
        if (trimmed.endsWith("{") || trimmed.endsWith("(")) {
            indent++;
        }
    }
    
    return result;
}

QString AdvancedCodeEditor::formatDart(const QString &code)
{
    // Similar to Java formatting for Dart/Flutter
    return formatJava(code);
}

void AdvancedCodeEditor::duplicateLine()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();
    
    int start = cursor.selectionStart();
    int end = cursor.selectionEnd();
    
    if (!cursor.hasSelection()) {
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    }
    
    QString text = cursor.selectedText();
    text.replace(QChar::ParagraphSeparator, '\n');
    
    cursor.movePosition(QTextCursor::EndOfBlock);
    cursor.insertText("\n" + text);
    cursor.endEditBlock();
}

void AdvancedCodeEditor::deleteLine()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    cursor.endEditBlock();
}

void AdvancedCodeEditor::moveLineUp()
{
    QTextCursor cursor = textCursor();
    if (cursor.blockNumber() == 0) return;
    
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    QString line = cursor.selectedText();
    cursor.removeSelectedText();
    cursor.deletePreviousChar(); // Remove newline
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.insertText(line + "\n");
    cursor.movePosition(QTextCursor::PreviousBlock);
    cursor.endEditBlock();
    setTextCursor(cursor);
}

void AdvancedCodeEditor::moveLineDown()
{
    QTextCursor cursor = textCursor();
    if (cursor.blockNumber() >= document()->blockCount() - 1) return;
    
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    QString line = cursor.selectedText();
    cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    cursor.movePosition(QTextCursor::EndOfBlock);
    cursor.insertText("\n" + line);
    cursor.endEditBlock();
    setTextCursor(cursor);
}

void AdvancedCodeEditor::toggleComment()
{
    QString prefix = getCommentPrefix();
    if (prefix.isEmpty()) return;
    
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();
    
    int start = cursor.selectionStart();
    int end = cursor.selectionEnd();
    
    QTextBlock startBlock = document()->findBlock(start);
    QTextBlock endBlock = document()->findBlock(end);
    if (end > start && end == endBlock.position()) {
        endBlock = endBlock.previous();
    }
    
    // Check if all lines are commented
    bool allCommented = true;
    for (QTextBlock block = startBlock; block.isValid() && block.position() <= endBlock.position(); block = block.next()) {
        if (!block.text().trimmed().startsWith(prefix)) {
            allCommented = false;
            break;
        }
    }
    
    // Toggle comments
    for (QTextBlock block = startBlock; block.isValid() && block.position() <= endBlock.position(); block = block.next()) {
        QTextCursor c(block);
        if (allCommented) {
            // Uncomment
            int idx = block.text().indexOf(prefix);
            if (idx >= 0) {
                c.setPosition(block.position() + idx);
                c.setPosition(block.position() + idx + prefix.length() + (block.text().mid(idx + prefix.length()).startsWith(" ") ? 1 : 0), QTextCursor::KeepAnchor);
                c.removeSelectedText();
            }
        } else {
            // Comment
            c.movePosition(QTextCursor::StartOfBlock);
            c.insertText(prefix + " ");
        }
    }
    
    cursor.endEditBlock();
}

QString AdvancedCodeEditor::getCommentPrefix()
{
    static QMap<QString, QString> commentMap = {
        {"smali", "#"}, {"java", "//"}, {"kotlin", "//"}, {"dart", "//"},
        {"javascript", "//"}, {"typescript", "//"}, {"swift", "//"},
        {"c", "//"}, {"cpp", "//"}, {"go", "//"}, {"rust", "//"},
        {"python", "#"}, {"ruby", "#"}, {"shell", "#"}, {"yaml", "#"},
        {"properties", "#"}, {"ini", ";"}, {"sql", "--"},
        {"xml", ""}, {"html", ""}, {"css", ""}
    };
    return commentMap.value(m_FileType, "//");
}

void AdvancedCodeEditor::calculateFoldRanges()
{
    m_FoldRanges.clear();
    QStack<int> stack;
    
    QTextBlock block = document()->begin();
    while (block.isValid()) {
        QString text = block.text().trimmed();
        int line = block.blockNumber();
        
        // Opening patterns
        if (text.endsWith("{") || text.startsWith(".method ") || 
            text.startsWith(".annotation ") || text.startsWith("<") && !text.startsWith("</") && !text.endsWith("/>")) {
            stack.push(line);
        }
        
        // Closing patterns
        if (text.startsWith("}") || text.startsWith(".end ") || text.startsWith("</")) {
            if (!stack.isEmpty()) {
                int startLine = stack.pop();
                if (line > startLine + 1) {
                    m_FoldRanges[startLine] = line;
                }
            }
        }
        
        block = block.next();
    }
}

bool AdvancedCodeEditor::isLineFoldable(int line) const
{
    return m_FoldRanges.contains(line);
}

bool AdvancedCodeEditor::isLineFolded(int line) const
{
    return m_FoldedLines.contains(line);
}

void AdvancedCodeEditor::toggleFold(int line)
{
    if (!isLineFoldable(line)) return;
    
    if (m_FoldedLines.contains(line)) {
        m_FoldedLines.remove(line);
        // Show blocks
        int endLine = m_FoldRanges[line];
        QTextBlock block = document()->findBlockByLineNumber(line + 1);
        while (block.isValid() && block.blockNumber() < endLine) {
            block.setVisible(true);
            block = block.next();
        }
    } else {
        m_FoldedLines.insert(line);
        // Hide blocks
        int endLine = m_FoldRanges[line];
        QTextBlock block = document()->findBlockByLineNumber(line + 1);
        while (block.isValid() && block.blockNumber() < endLine) {
            block.setVisible(false);
            block = block.next();
        }
    }
    
    viewport()->update();
    m_Sidebar->update();
}

void AdvancedCodeEditor::foldAll()
{
    for (auto it = m_FoldRanges.constBegin(); it != m_FoldRanges.constEnd(); ++it) {
        if (!m_FoldedLines.contains(it.key())) {
            toggleFold(it.key());
        }
    }
}

void AdvancedCodeEditor::unfoldAll()
{
    QSet<int> folded = m_FoldedLines;
    for (int line : folded) {
        toggleFold(line);
    }
}

void AdvancedCodeEditor::handleBlockCountChanged(int count)
{
    Q_UNUSED(count)
    updateSidebarWidth();
}

void AdvancedCodeEditor::updateSidebarWidth()
{
    int digits = QString::number(qMax(1, blockCount())).length();
    int width = fontMetrics().horizontalAdvance('9') * (digits + 2) + SIDEBAR_FOLD_WIDTH + 10;
    setViewportMargins(width, 0, MINIMAP_WIDTH, 0);
    
    QRect cr = contentsRect();
    m_Sidebar->setGeometry(QRect(cr.left(), cr.top(), width, cr.height()));
    m_Minimap->setGeometry(QRect(cr.right() - MINIMAP_WIDTH, cr.top(), MINIMAP_WIDTH, cr.height()));
}

void AdvancedCodeEditor::handleCursorPositionChanged()
{
    highlightCurrentLine();
    highlightMatchingBrackets();
    
    QTextCursor cursor = textCursor();
    emit cursorPositionInfo(cursor.blockNumber() + 1, cursor.columnNumber() + 1);
}

void AdvancedCodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> selections;
    
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor highlight = palette().color(QPalette::Highlight);
        highlight.setAlpha(30);
        selection.format.setBackground(highlight);
        selection.format.setProperty(QTextCharFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        selections.append(selection);
    }
    
    selections.append(m_BracketSelections);
    setExtraSelections(selections);
}

void AdvancedCodeEditor::highlightMatchingBrackets()
{
    m_BracketSelections.clear();
    
    QTextCursor cursor = textCursor();
    QTextDocument *doc = document();
    int pos = cursor.position();
    
    if (pos >= doc->characterCount()) return;
    
    QChar c = doc->characterAt(pos);
    QChar match;
    bool forward = true;
    
    static QMap<QChar, QChar> brackets = {
        {'(', ')'}, {')', '('}, 
        {'{', '}'}, {'}', '{'}, 
        {'[', ']'}, {']', '['},
        {'<', '>'}, {'>', '<'}
    };
    
    if (!brackets.contains(c)) {
        if (pos > 0) {
            c = doc->characterAt(pos - 1);
            pos--;
        }
    }
    
    if (!brackets.contains(c)) return;
    
    match = brackets[c];
    forward = (c == '(' || c == '{' || c == '[' || c == '<');
    
    // Find matching bracket
    int depth = 1;
    int i = forward ? pos + 1 : pos - 1;
    
    while (i >= 0 && i < doc->characterCount() && depth > 0) {
        QChar ch = doc->characterAt(i);
        if (ch == c) depth++;
        else if (ch == match) depth--;
        
        if (depth == 0) {
            // Highlight both brackets
            QTextEdit::ExtraSelection sel1, sel2;
            QColor color(Qt::yellow);
            color.setAlpha(100);
            
            sel1.format.setBackground(color);
            sel1.cursor = QTextCursor(doc);
            sel1.cursor.setPosition(pos);
            sel1.cursor.setPosition(pos + 1, QTextCursor::KeepAnchor);
            
            sel2.format.setBackground(color);
            sel2.cursor = QTextCursor(doc);
            sel2.cursor.setPosition(i);
            sel2.cursor.setPosition(i + 1, QTextCursor::KeepAnchor);
            
            m_BracketSelections << sel1 << sel2;
            break;
        }
        
        i += forward ? 1 : -1;
    }
}

void AdvancedCodeEditor::handleUpdateRequest(const QRect &rect, int dy)
{
    if (dy) {
        m_Sidebar->scroll(0, dy);
    }
    m_Sidebar->update(0, rect.y(), m_Sidebar->width(), rect.height());
    
    if (rect.contains(viewport()->rect())) {
        updateSidebarWidth();
    }
}

void AdvancedCodeEditor::handleTextChanged()
{
    m_CompletionTimer->start();
    m_SymbolUpdateTimer->start();
    m_Minimap->updateContent();
    emit wordCountChanged(wordCount());
}

void AdvancedCodeEditor::showCompletions()
{
    QTextCursor cursor = textCursor();
    cursor.select(QTextCursor::WordUnderCursor);
    QString prefix = cursor.selectedText();
    
    if (prefix.length() < 2) {
        m_Completer->popup()->hide();
        return;
    }
    
    QStringList completions = getCompletionsForType();
    QStringList filtered;
    for (const QString &c : completions) {
        if (c.startsWith(prefix, Qt::CaseInsensitive) && c != prefix) {
            filtered << c;
        }
    }
    
    if (filtered.isEmpty()) {
        m_Completer->popup()->hide();
        return;
    }
    
    m_Completer->setModel(new QStringListModel(filtered, m_Completer));
    m_Completer->setCompletionPrefix(prefix);
    
    QRect cr = cursorRect();
    cr.setWidth(m_Completer->popup()->sizeHintForColumn(0) + 
                m_Completer->popup()->verticalScrollBar()->sizeHint().width());
    m_Completer->complete(cr);
}

QStringList AdvancedCodeEditor::getCompletionsForType()
{
    static QMap<QString, QStringList> completions = {
        {"smali", {
            ".class", ".super", ".source", ".implements", ".field", ".method",
            ".end method", ".end field", ".annotation", ".end annotation",
            ".registers", ".locals", ".param", ".prologue", ".line",
            "invoke-virtual", "invoke-static", "invoke-direct", "invoke-interface",
            "move-result", "move-result-object", "move-result-wide",
            "const", "const/4", "const/16", "const-string", "const-class",
            "new-instance", "new-array", "iget", "iput", "sget", "sput",
            "if-eq", "if-ne", "if-lt", "if-ge", "if-gt", "if-le",
            "if-eqz", "if-nez", "if-ltz", "if-gez", "if-gtz", "if-lez",
            "goto", "return", "return-void", "return-object", "return-wide",
            "check-cast", "instance-of", "array-length", "throw"
        }},
        {"java", {
            "public", "private", "protected", "static", "final", "abstract",
            "class", "interface", "extends", "implements", "import", "package",
            "void", "int", "boolean", "String", "double", "float", "long",
            "if", "else", "for", "while", "do", "switch", "case", "default",
            "try", "catch", "finally", "throw", "throws", "return", "new",
            "this", "super", "null", "true", "false", "instanceof"
        }},
        {"kotlin", {
            "fun", "val", "var", "class", "object", "interface", "sealed",
            "data", "enum", "companion", "init", "constructor", "override",
            "open", "final", "abstract", "private", "public", "protected",
            "internal", "if", "else", "when", "for", "while", "do", "return",
            "break", "continue", "throw", "try", "catch", "finally",
            "is", "as", "in", "!in", "null", "true", "false", "this", "super"
        }},
        {"dart", {
            "void", "int", "double", "bool", "String", "List", "Map", "Set",
            "var", "final", "const", "late", "dynamic", "Object",
            "class", "extends", "implements", "with", "mixin", "abstract",
            "if", "else", "for", "while", "do", "switch", "case", "default",
            "try", "catch", "finally", "throw", "rethrow", "return", "break",
            "continue", "assert", "async", "await", "yield", "sync",
            "import", "export", "library", "part", "show", "hide", "as",
            "new", "this", "super", "null", "true", "false", "is", "is!"
        }},
        {"xml", {
            "android:", "app:", "tools:", "xmlns:", "layout_width", "layout_height",
            "match_parent", "wrap_content", "id", "text", "textSize", "textColor",
            "background", "padding", "margin", "orientation", "gravity",
            "LinearLayout", "RelativeLayout", "ConstraintLayout", "FrameLayout",
            "TextView", "EditText", "Button", "ImageView", "RecyclerView"
        }}
    };
    
    return completions.value(m_FileType, QStringList());
}

void AdvancedCodeEditor::handleCompletionSelected(const QString &completion)
{
    QTextCursor cursor = textCursor();
    cursor.select(QTextCursor::WordUnderCursor);
    cursor.insertText(completion);
}

void AdvancedCodeEditor::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();
    
    menu->addSeparator();
    
    // Edit actions
    QMenu *editMenu = menu->addMenu(tr("📝 Edit"));
    editMenu->addAction(tr("Duplicate Line"), this, &AdvancedCodeEditor::duplicateLine, QKeySequence(Qt::CTRL | Qt::Key_D));
    editMenu->addAction(tr("Delete Line"), this, &AdvancedCodeEditor::deleteLine, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_K));
    editMenu->addAction(tr("Move Line Up"), this, &AdvancedCodeEditor::moveLineUp, QKeySequence(Qt::ALT | Qt::Key_Up));
    editMenu->addAction(tr("Move Line Down"), this, &AdvancedCodeEditor::moveLineDown, QKeySequence(Qt::ALT | Qt::Key_Down));
    editMenu->addSeparator();
    editMenu->addAction(tr("Toggle Comment"), this, &AdvancedCodeEditor::toggleComment, QKeySequence(Qt::CTRL | Qt::Key_Slash));
    editMenu->addAction(tr("Format Document"), this, &AdvancedCodeEditor::formatDocument, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F));
    editMenu->addSeparator();
    editMenu->addAction(tr("Join Lines"), this, &AdvancedCodeEditor::joinLines, QKeySequence(Qt::CTRL | Qt::Key_J));
    editMenu->addAction(tr("Sort Lines"), this, &AdvancedCodeEditor::sortLines);
    editMenu->addAction(tr("Remove Duplicate Lines"), this, &AdvancedCodeEditor::removeDuplicateLines);
    
    // Selection
    QMenu *selectMenu = menu->addMenu(tr("🔍 Selection"));
    selectMenu->addAction(tr("Select Word"), this, &AdvancedCodeEditor::selectWord, QKeySequence(Qt::CTRL | Qt::Key_W));
    selectMenu->addAction(tr("Select Line"), this, &AdvancedCodeEditor::selectLine, QKeySequence(Qt::CTRL | Qt::Key_L));
    
    // Whitespace
    QMenu *wsMenu = menu->addMenu(tr("⬜ Whitespace"));
    wsMenu->addAction(tr("Trim Trailing Whitespace"), this, &AdvancedCodeEditor::trimTrailingWhitespace);
    wsMenu->addAction(tr("Convert Tabs to Spaces"), this, &AdvancedCodeEditor::convertTabsToSpaces);
    wsMenu->addAction(tr("Convert Spaces to Tabs"), this, &AdvancedCodeEditor::convertSpacesToTabs);
    
    // Bookmarks
    QMenu *bookmarkMenu = menu->addMenu(tr("🔖 Bookmarks"));
    bookmarkMenu->addAction(tr("Toggle Bookmark"), this, [this]() { toggleBookmark(); }, QKeySequence(Qt::CTRL | Qt::Key_M));
    bookmarkMenu->addAction(tr("Next Bookmark"), this, &AdvancedCodeEditor::nextBookmark, QKeySequence(Qt::Key_F2));
    bookmarkMenu->addAction(tr("Previous Bookmark"), this, &AdvancedCodeEditor::previousBookmark, QKeySequence(Qt::SHIFT | Qt::Key_F2));
    bookmarkMenu->addSeparator();
    bookmarkMenu->addAction(tr("Clear All Bookmarks"), this, &AdvancedCodeEditor::clearBookmarks);
    
    // Folding
    if (!m_FoldRanges.isEmpty()) {
        QMenu *foldMenu = menu->addMenu(tr("📁 Folding"));
        foldMenu->addAction(tr("Fold All"), this, &AdvancedCodeEditor::foldAll);
        foldMenu->addAction(tr("Unfold All"), this, &AdvancedCodeEditor::unfoldAll);
    }
    
    // AI actions
    QSettings settings;
    bool aiEnabled = settings.value("ai_enabled", false).toBool();
    QString apiKey = settings.value("ai_api_key").toString();
    
    if (aiEnabled && !apiKey.isEmpty()) {
        menu->addSeparator();
        QMenu *aiMenu = menu->addMenu(tr("🤖 AI Assistant"));
        
        QAction *explainAction = aiMenu->addAction(tr("Explain Code"));
        explainAction->setEnabled(textCursor().hasSelection());
        connect(explainAction, &QAction::triggered, this, &AdvancedCodeEditor::aiExplainCode);
        
        QAction *fixAction = aiMenu->addAction(tr("Fix Issues"));
        fixAction->setEnabled(textCursor().hasSelection());
        connect(fixAction, &QAction::triggered, this, &AdvancedCodeEditor::aiFixCode);
        
        aiMenu->addAction(tr("Find Security Issues"), this, &AdvancedCodeEditor::aiFindIssues);
        
        QAction *optimizeAction = aiMenu->addAction(tr("Optimize"));
        optimizeAction->setEnabled(textCursor().hasSelection());
        connect(optimizeAction, &QAction::triggered, this, &AdvancedCodeEditor::aiOptimizeCode);
        
        QAction *commentsAction = aiMenu->addAction(tr("Add Comments"));
        commentsAction->setEnabled(textCursor().hasSelection());
        connect(commentsAction, &QAction::triggered, this, &AdvancedCodeEditor::aiAddComments);
        
        aiMenu->addSeparator();
        aiMenu->addAction(tr("Generate Code..."), this, &AdvancedCodeEditor::aiGenerateCode);
        
        QAction *refactorAction = aiMenu->addAction(tr("Refactor"));
        refactorAction->setEnabled(textCursor().hasSelection());
        connect(refactorAction, &QAction::triggered, this, &AdvancedCodeEditor::aiRefactorCode);
    }
    
    menu->exec(event->globalPos());
    delete menu;
}

void AdvancedCodeEditor::keyPressEvent(QKeyEvent *event)
{
    // Handle tab
    if (event->key() == Qt::Key_Tab && !event->modifiers()) {
        QTextCursor cursor = textCursor();
        if (cursor.hasSelection()) {
            // Indent selection
            int start = cursor.selectionStart();
            int end = cursor.selectionEnd();
            QTextBlock startBlock = document()->findBlock(start);
            QTextBlock endBlock = document()->findBlock(end);
            
            cursor.beginEditBlock();
            for (QTextBlock block = startBlock; block.isValid() && block.position() <= endBlock.position(); block = block.next()) {
                QTextCursor c(block);
                c.insertText(QString(TAB_SIZE, ' '));
            }
            cursor.endEditBlock();
        } else {
            cursor.insertText(QString(TAB_SIZE, ' '));
        }
        event->accept();
        return;
    }
    
    // Handle backtab (shift+tab)
    if (event->key() == Qt::Key_Backtab) {
        QTextCursor cursor = textCursor();
        int start = cursor.selectionStart();
        int end = cursor.selectionEnd();
        QTextBlock startBlock = document()->findBlock(start);
        QTextBlock endBlock = document()->findBlock(end);
        
        cursor.beginEditBlock();
        for (QTextBlock block = startBlock; block.isValid() && block.position() <= endBlock.position(); block = block.next()) {
            QString text = block.text();
            int spaces = 0;
            for (QChar c : text) {
                if (c == ' ' && spaces < TAB_SIZE) spaces++;
                else break;
            }
            if (spaces > 0) {
                QTextCursor c(block);
                c.setPosition(block.position() + spaces, QTextCursor::KeepAnchor);
                c.removeSelectedText();
            }
        }
        cursor.endEditBlock();
        event->accept();
        return;
    }
    
    // Auto-close brackets
    if (event->text() == "{") {
        textCursor().insertText("{}");
        moveCursor(QTextCursor::Left);
        event->accept();
        return;
    }
    if (event->text() == "[") {
        textCursor().insertText("[]");
        moveCursor(QTextCursor::Left);
        event->accept();
        return;
    }
    if (event->text() == "(") {
        textCursor().insertText("()");
        moveCursor(QTextCursor::Left);
        event->accept();
        return;
    }
    if (event->text() == "\"") {
        textCursor().insertText("\"\"");
        moveCursor(QTextCursor::Left);
        event->accept();
        return;
    }
    if (event->text() == "'") {
        textCursor().insertText("''");
        moveCursor(QTextCursor::Left);
        event->accept();
        return;
    }
    
    // Auto-indent on Enter
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        QTextCursor cursor = textCursor();
        QString line = cursor.block().text();
        QString indent;
        
        for (QChar c : line) {
            if (c == ' ' || c == '\t') indent += c;
            else break;
        }
        
        // Increase indent if line ends with { or :
        QString trimmed = line.trimmed();
        if (trimmed.endsWith("{") || trimmed.endsWith(":") || trimmed.endsWith("(")) {
            indent += QString(TAB_SIZE, ' ');
        }
        
        cursor.insertText("\n" + indent);
        event->accept();
        return;
    }
    
    QPlainTextEdit::keyPressEvent(event);
}

void AdvancedCodeEditor::paintEvent(QPaintEvent *event)
{
    QPainter painter(viewport());
    
    // Draw indent guides
    QSettings settings;
    if (settings.value("editor_indent_guides", true).toBool()) {
        QColor guideColor = palette().color(QPalette::Text);
        guideColor.setAlpha(30);
        painter.setPen(QPen(guideColor, 1, Qt::DotLine));
        
        int spaceWidth = fontMetrics().horizontalAdvance(' ');
        for (int i = 1; i <= 20; ++i) {
            int x = contentOffset().x() + document()->documentMargin() + i * TAB_SIZE * spaceWidth;
            if (x < viewport()->width()) {
                painter.drawLine(x, 0, x, viewport()->height());
            }
        }
    }
    
    // Draw right margin at 80 chars
    int marginX = static_cast<int>((fontMetrics().horizontalAdvance('8') * 80)
                                    + contentOffset().x()
                                    + document()->documentMargin());
    QColor marginColor = palette().color(QPalette::Text);
    marginColor.setAlpha(50);
    painter.setPen(QPen(marginColor, 1, Qt::DotLine));
    painter.drawLine(marginX, 0, marginX, viewport()->height());
    
    QPlainTextEdit::paintEvent(event);
}

void AdvancedCodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    updateSidebarWidth();
}

void AdvancedCodeEditor::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        int delta = event->angleDelta().y();
        if (delta > 0) zoomIn();
        else if (delta < 0) zoomOut();
        event->accept();
        return;
    }
    QPlainTextEdit::wheelEvent(event);
}

void AdvancedCodeEditor::focusInEvent(QFocusEvent *event)
{
    QPlainTextEdit::focusInEvent(event);
}

QRectF AdvancedCodeEditor::blockBoundingGeometryProxy(const QTextBlock &block)
{
    return blockBoundingGeometry(block);
}

QRectF AdvancedCodeEditor::blockBoundingRectProxy(const QTextBlock &block)
{
    return blockBoundingRect(block);
}

QPointF AdvancedCodeEditor::contentOffsetProxy()
{
    return contentOffset();
}

QTextBlock AdvancedCodeEditor::firstVisibleBlockProxy()
{
    return firstVisibleBlock();
}

// AI Functions
void AdvancedCodeEditor::aiExplainCode()
{
    QString selected = textCursor().selectedText().replace(QChar::ParagraphSeparator, '\n');
    askAI(QString("Explain this %1 code in detail:\n\n```%1\n%2\n```").arg(m_FileType, selected), false);
}

void AdvancedCodeEditor::aiFixCode()
{
    QString selected = textCursor().selectedText().replace(QChar::ParagraphSeparator, '\n');
    askAI(QString("Fix bugs and issues in this %1 code. Return ONLY the fixed code:\n\n```%1\n%2\n```").arg(m_FileType, selected), true);
}

void AdvancedCodeEditor::aiFindIssues()
{
    QString content = toPlainText();
    if (content.length() > 15000) content = content.left(15000) + "\n... (truncated)";
    askAI(QString("Analyze this %1 file for security vulnerabilities and bugs. List each issue with severity:\n\n```%1\n%2\n```").arg(m_FileType, content), false);
}

void AdvancedCodeEditor::aiOptimizeCode()
{
    QString selected = textCursor().selectedText().replace(QChar::ParagraphSeparator, '\n');
    askAI(QString("Optimize this %1 code for performance. Return ONLY optimized code:\n\n```%1\n%2\n```").arg(m_FileType, selected), true);
}

void AdvancedCodeEditor::aiAddComments()
{
    QString selected = textCursor().selectedText().replace(QChar::ParagraphSeparator, '\n');
    askAI(QString("Add helpful comments to this %1 code. Return the code with comments:\n\n```%1\n%2\n```").arg(m_FileType, selected), true);
}

void AdvancedCodeEditor::aiGenerateCode()
{
    QString prompt = QInputDialog::getText(this, tr("Generate Code"), 
        tr("Describe what code you want to generate:"));
    if (prompt.isEmpty()) return;
    
    askAI(QString("Generate %1 code for: %2\n\nReturn ONLY the code without explanation.").arg(m_FileType, prompt), true);
}

void AdvancedCodeEditor::aiRefactorCode()
{
    QString selected = textCursor().selectedText().replace(QChar::ParagraphSeparator, '\n');
    askAI(QString("Refactor this %1 code to be cleaner and more maintainable. Return ONLY the refactored code:\n\n```%1\n%2\n```").arg(m_FileType, selected), true);
}

void AdvancedCodeEditor::askAI(const QString &prompt, bool replaceSelection)
{
    QSettings settings;
    QString provider = settings.value("ai_provider", "gemini").toString();
    QString model = settings.value("ai_model", "gemini-2.0-flash-exp").toString();
    QString apiKey = settings.value("ai_api_key").toString();
    
    if (apiKey.isEmpty()) {
        QMessageBox::warning(this, tr("AI Not Configured"), tr("Configure API key in Settings > AI Assistant."));
        return;
    }
    
    QString endpoint;
    QJsonObject root;
    
    if (provider == "gemini") {
        endpoint = QString("https://generativelanguage.googleapis.com/v1beta/models/%1:generateContent?key=%2").arg(model, apiKey);
        QJsonArray contents;
        QJsonObject content;
        QJsonArray parts;
        QJsonObject part;
        part["text"] = prompt;
        parts.append(part);
        content["parts"] = parts;
        contents.append(content);
        root["contents"] = contents;
    } else if (provider == "openai" || provider == "copilot") {
        endpoint = "https://api.openai.com/v1/chat/completions";
        root["model"] = model;
        QJsonArray messages;
        QJsonObject msg;
        msg["role"] = "user";
        msg["content"] = prompt;
        messages.append(msg);
        root["messages"] = messages;
        root["max_tokens"] = 4096;
    } else if (provider == "anthropic") {
        endpoint = "https://api.anthropic.com/v1/messages";
        root["model"] = model;
        root["max_tokens"] = 4096;
        QJsonArray messages;
        QJsonObject msg;
        msg["role"] = "user";
        msg["content"] = prompt;
        messages.append(msg);
        root["messages"] = messages;
    }
    
    QNetworkRequest request;
    request.setUrl(QUrl(endpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    if (provider == "openai" || provider == "copilot") {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());
    } else if (provider == "anthropic") {
        request.setRawHeader("x-api-key", apiKey.toUtf8());
        request.setRawHeader("anthropic-version", "2023-06-01");
    }
    
    QToolTip::showText(mapToGlobal(cursorRect().topLeft()), tr("🤖 AI thinking..."), this);
    
    QNetworkReply *reply = m_NetworkManager->post(request, QJsonDocument(root).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply, replaceSelection]() {
        handleAIResponse(reply, replaceSelection);
    });
}

void AdvancedCodeEditor::handleAIResponse(QNetworkReply *reply, bool replaceSelection)
{
    QToolTip::hideText();
    
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::warning(this, tr("AI Error"), reply->errorString());
        reply->deleteLater();
        return;
    }
    
    QByteArray data = reply->readAll();
    reply->deleteLater();
    
    QSettings settings;
    QString provider = settings.value("ai_provider", "gemini").toString();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QString response;
    
    if (provider == "gemini") {
        QJsonArray candidates = doc.object()["candidates"].toArray();
        if (!candidates.isEmpty()) {
            response = candidates[0].toObject()["content"].toObject()["parts"].toArray()[0].toObject()["text"].toString();
        }
    } else if (provider == "openai" || provider == "copilot") {
        QJsonArray choices = doc.object()["choices"].toArray();
        if (!choices.isEmpty()) {
            response = choices[0].toObject()["message"].toObject()["content"].toString();
        }
    } else if (provider == "anthropic") {
        QJsonArray content = doc.object()["content"].toArray();
        if (!content.isEmpty()) {
            response = content[0].toObject()["text"].toString();
        }
    }
    
    if (response.isEmpty()) {
        QMessageBox::information(this, tr("AI Response"), tr("No response received."));
        return;
    }
    
    // Extract code from response
    QRegularExpression codeRegex("```[a-z]*\\n([\\s\\S]*?)\\n```");
    QRegularExpressionMatch match = codeRegex.match(response);
    
    if (replaceSelection && match.hasMatch() && textCursor().hasSelection()) {
        QString code = match.captured(1);
        QMessageBox::StandardButton btn = QMessageBox::question(this, tr("AI Suggestion"),
            tr("Replace selected text with AI suggestion?"),
            QMessageBox::Yes | QMessageBox::No);
        
        if (btn == QMessageBox::Yes) {
            textCursor().insertText(code);
        }
    } else {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("AI Response"));
        msgBox.setText(tr("AI Analysis Complete"));
        msgBox.setDetailedText(response);
        msgBox.exec();
    }
    
    emit aiResponseReceived(response);
}

// === Sidebar Implementation ===
CodeEditorSidebar::CodeEditorSidebar(AdvancedCodeEditor *editor)
    : QWidget(editor), m_Editor(editor)
{
    setMouseTracking(true);
}

QSize CodeEditorSidebar::sizeHint() const
{
    return QSize(m_Editor->fontMetrics().horizontalAdvance('9') * 
                 (QString::number(m_Editor->blockCount()).length() + 2) + SIDEBAR_FOLD_WIDTH + 10, 0);
}

void CodeEditorSidebar::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    
    // Background
    QColor bgColor = m_Editor->palette().color(QPalette::Base);
    bgColor = bgColor.darker(105);
    painter.fillRect(event->rect(), bgColor);
    
    QTextBlock block = m_Editor->firstVisibleBlockProxy();
    int blockNumber = block.blockNumber();
    int top = static_cast<int>(m_Editor->blockBoundingGeometryProxy(block).translated(m_Editor->contentOffsetProxy()).top());
    int bottom = top + static_cast<int>(m_Editor->blockBoundingRectProxy(block).height());
    
    QFont font = m_Editor->font();
    font.setPointSize(font.pointSize());
    painter.setFont(font);
    
    int currentLine = m_Editor->textCursor().blockNumber();
    
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            
            // Draw bookmark indicator
            if (m_Editor->hasBookmark(blockNumber)) {
                painter.setBrush(QColor(0, 122, 204));
                painter.setPen(Qt::NoPen);
                int bmY = top + (m_Editor->fontMetrics().height() - 8) / 2;
                painter.drawEllipse(2, bmY, 8, 8);
            }
            
            // Line number color
            if (blockNumber == currentLine) {
                painter.setPen(m_Editor->palette().color(QPalette::Text));
                font.setWeight(QFont::Bold);
            } else {
                QColor numColor = m_Editor->palette().color(QPalette::Text);
                numColor.setAlpha(128);
                painter.setPen(numColor);
                font.setWeight(QFont::Normal);
            }
            painter.setFont(font);
            
            // Draw line number
            int numberWidth = width() - SIDEBAR_FOLD_WIDTH - 5;
            painter.drawText(12, top, numberWidth - 12, m_Editor->fontMetrics().height(),
                           Qt::AlignRight | Qt::AlignVCenter, number);
            
            // Draw fold indicator
            if (m_Editor->isLineFoldable(blockNumber)) {
                int foldX = width() - SIDEBAR_FOLD_WIDTH + 2;
                int foldY = top + (m_Editor->fontMetrics().height() - 10) / 2;
                
                painter.setPen(m_Editor->palette().color(QPalette::Text));
                painter.setBrush(Qt::NoBrush);
                painter.drawRect(foldX, foldY, 10, 10);
                
                // Draw +/- 
                painter.drawLine(foldX + 2, foldY + 5, foldX + 8, foldY + 5);
                if (m_Editor->isLineFolded(blockNumber)) {
                    painter.drawLine(foldX + 5, foldY + 2, foldX + 5, foldY + 8);
                }
            }
        }
        
        block = block.next();
        top = bottom;
        bottom = top + static_cast<int>(m_Editor->blockBoundingRectProxy(block).height());
        ++blockNumber;
    }
}

void CodeEditorSidebar::mousePressEvent(QMouseEvent *event)
{
    int y = event->pos().y();
    
    QTextBlock block = m_Editor->firstVisibleBlockProxy();
    int top = static_cast<int>(m_Editor->blockBoundingGeometryProxy(block).translated(m_Editor->contentOffsetProxy()).top());
    
    while (block.isValid()) {
        int bottom = top + static_cast<int>(m_Editor->blockBoundingRectProxy(block).height());
        if (y >= top && y < bottom) {
            int line = block.blockNumber();
            
            // Check if click is in fold area
            if (event->pos().x() >= width() - SIDEBAR_FOLD_WIDTH) {
                if (m_Editor->isLineFoldable(line)) {
                    m_Editor->toggleFold(line);
                }
            } else {
                // Select line
                m_Editor->gotoLine(line + 1);
            }
            break;
        }
        top = bottom;
        block = block.next();
    }
}

void CodeEditorSidebar::wheelEvent(QWheelEvent *event)
{
    QApplication::sendEvent(m_Editor->viewport(), event);
}

// === Minimap Implementation ===
CodeEditorMinimap::CodeEditorMinimap(AdvancedCodeEditor *editor)
    : QWidget(editor), m_Editor(editor), m_Dragging(false)
{
    setMouseTracking(true);
    setFixedWidth(MINIMAP_WIDTH);
}

void CodeEditorMinimap::updateContent()
{
    // Create cached minimap image
    int lines = m_Editor->blockCount();
    int h = qMin(height(), lines * 2);
    
    m_CachedContent = QImage(width(), h, QImage::Format_ARGB32);
    m_CachedContent.fill(Qt::transparent);
    
    QPainter painter(&m_CachedContent);
    QColor textColor = m_Editor->palette().color(QPalette::Text);
    textColor.setAlpha(100);
    
    QTextBlock block = m_Editor->document()->begin();
    int y = 0;
    
    while (block.isValid() && y < h) {
        QString text = block.text();
        int x = 2;
        
        for (int i = 0; i < qMin(text.length(), 40); ++i) {
            if (!text[i].isSpace()) {
                painter.fillRect(x, y, 1, 2, textColor);
            }
            x++;
        }
        
        y += 2;
        block = block.next();
    }
    
    update();
}

void CodeEditorMinimap::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    
    // Background
    QColor bgColor = m_Editor->palette().color(QPalette::Base);
    bgColor = bgColor.darker(110);
    painter.fillRect(rect(), bgColor);
    
    // Draw cached content
    painter.drawImage(0, 0, m_CachedContent);
    
    // Draw viewport indicator
    int totalLines = m_Editor->blockCount();
    if (totalLines > 0) {
        int firstVisible = m_Editor->firstVisibleBlockProxy().blockNumber();
        int visibleLines = m_Editor->viewport()->height() / m_Editor->fontMetrics().height();
        
        int y1 = (firstVisible * height()) / totalLines;
        int y2 = ((firstVisible + visibleLines) * height()) / totalLines;
        
        QColor viewportColor = m_Editor->palette().color(QPalette::Highlight);
        viewportColor.setAlpha(50);
        painter.fillRect(0, y1, width(), y2 - y1, viewportColor);
    }
}

void CodeEditorMinimap::mousePressEvent(QMouseEvent *event)
{
    m_Dragging = true;
    navigateToPosition(event->pos().y());
}

void CodeEditorMinimap::mouseMoveEvent(QMouseEvent *event)
{
    if (m_Dragging) {
        navigateToPosition(event->pos().y());
    }
}

void CodeEditorMinimap::navigateToPosition(int y)
{
    int totalLines = m_Editor->blockCount();
    if (totalLines == 0) return;
    
    int line = (y * totalLines) / height();
    line = qBound(0, line, totalLines - 1);
    m_Editor->gotoLine(line + 1);
}

// === New Editor Functions ===

bool AdvancedCodeEditor::saveAs(const QString &path)
{
    QString oldPath = m_FilePath;
    m_FilePath = path;
    if (!save()) {
        m_FilePath = oldPath;
        return false;
    }
    return true;
}

void AdvancedCodeEditor::reload()
{
    if (m_FilePath.isEmpty()) return;
    
    int currentLine = textCursor().blockNumber();
    int currentCol = textCursor().columnNumber();
    
    QFile file(m_FilePath);
    if (file.open(QFile::ReadOnly)) {
        QByteArray rawData = file.readAll();
        file.close();
        setPlainText(QString::fromUtf8(rawData));
        document()->setModified(false);
        
        // Restore cursor position
        QTextCursor cursor(document()->findBlockByLineNumber(currentLine));
        cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, 
                           qMin(currentCol, cursor.block().text().length()));
        setTextCursor(cursor);
    }
}

void AdvancedCodeEditor::gotoColumn(int col)
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfLine);
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, 
                       qMin(col - 1, cursor.block().text().length()));
    setTextCursor(cursor);
}

void AdvancedCodeEditor::deleteWord()
{
    QTextCursor cursor = textCursor();
    cursor.select(QTextCursor::WordUnderCursor);
    cursor.removeSelectedText();
}

void AdvancedCodeEditor::selectWord()
{
    QTextCursor cursor = textCursor();
    cursor.select(QTextCursor::WordUnderCursor);
    setTextCursor(cursor);
}

void AdvancedCodeEditor::selectLine()
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    setTextCursor(cursor);
}

void AdvancedCodeEditor::selectAllText()
{
    QTextCursor cursor = textCursor();
    cursor.select(QTextCursor::Document);
    setTextCursor(cursor);
}

void AdvancedCodeEditor::joinLines()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();
    
    if (cursor.hasSelection()) {
        int start = cursor.selectionStart();
        int end = cursor.selectionEnd();
        cursor.setPosition(start);
        cursor.setPosition(end, QTextCursor::KeepAnchor);
        QString text = cursor.selectedText();
        text.replace(QChar::ParagraphSeparator, ' ');
        text.replace('\n', ' ');
        // Remove multiple spaces
        while (text.contains("  ")) {
            text.replace("  ", " ");
        }
        cursor.insertText(text);
    } else {
        cursor.movePosition(QTextCursor::EndOfBlock);
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        if (!cursor.atEnd()) {
            cursor.insertText(" ");
        }
    }
    
    cursor.endEditBlock();
}

void AdvancedCodeEditor::sortLines()
{
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) {
        selectAllText();
        cursor = textCursor();
    }
    
    int start = cursor.selectionStart();
    int end = cursor.selectionEnd();
    
    cursor.setPosition(start);
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    
    QString text = cursor.selectedText();
    text.replace(QChar::ParagraphSeparator, '\n');
    QStringList lines = text.split('\n');
    lines.sort(Qt::CaseInsensitive);
    
    cursor.beginEditBlock();
    cursor.insertText(lines.join('\n'));
    cursor.endEditBlock();
}

void AdvancedCodeEditor::removeDuplicateLines()
{
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) {
        selectAllText();
        cursor = textCursor();
    }
    
    QString text = cursor.selectedText();
    text.replace(QChar::ParagraphSeparator, '\n');
    QStringList lines = text.split('\n');
    
    QStringList unique;
    QSet<QString> seen;
    for (const QString &line : lines) {
        if (!seen.contains(line)) {
            unique << line;
            seen.insert(line);
        }
    }
    
    cursor.beginEditBlock();
    cursor.insertText(unique.join('\n'));
    cursor.endEditBlock();
}

void AdvancedCodeEditor::convertTabsToSpaces()
{
    QString text = toPlainText();
    text.replace('\t', QString(TAB_SIZE, ' '));
    
    QTextCursor cursor = textCursor();
    int pos = cursor.position();
    
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.insertText(text);
    cursor.endEditBlock();
    
    cursor.setPosition(qMin(pos, text.length()));
    setTextCursor(cursor);
}

void AdvancedCodeEditor::convertSpacesToTabs()
{
    QString text = toPlainText();
    text.replace(QString(TAB_SIZE, ' '), "\t");
    
    QTextCursor cursor = textCursor();
    int pos = cursor.position();
    
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.insertText(text);
    cursor.endEditBlock();
    
    cursor.setPosition(qMin(pos, text.length()));
    setTextCursor(cursor);
}

void AdvancedCodeEditor::trimTrailingWhitespace()
{
    QString text = toPlainText();
    QStringList lines = text.split('\n');
    
    for (int i = 0; i < lines.size(); ++i) {
        lines[i] = lines[i].trimmed();
    }
    
    QTextCursor cursor = textCursor();
    int pos = cursor.position();
    
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.insertText(lines.join('\n'));
    cursor.endEditBlock();
    
    cursor.setPosition(qMin(pos, document()->characterCount() - 1));
    setTextCursor(cursor);
}

void AdvancedCodeEditor::insertSnippet(const QString &snippet)
{
    QTextCursor cursor = textCursor();
    
    // Replace ${1}, ${2}, etc. with placeholders
    QString processed = snippet;
    processed.replace("${cursor}", "");
    
    cursor.insertText(processed);
    
    // Find cursor placeholder position
    int cursorPos = snippet.indexOf("${cursor}");
    if (cursorPos >= 0) {
        cursor.setPosition(cursor.position() - (snippet.length() - cursorPos));
        setTextCursor(cursor);
    }
}

int AdvancedCodeEditor::findText(const QString &text, bool caseSensitive, bool wholeWord, bool regex)
{
    m_LastSearchText = text;
    m_LastSearchCaseSensitive = caseSensitive;
    m_LastSearchWholeWord = wholeWord;
    m_LastSearchRegex = regex;
    
    if (text.isEmpty()) {
        m_SearchSelections.clear();
        highlightCurrentLine();
        return 0;
    }
    
    m_SearchSelections.clear();
    QTextDocument *doc = document();
    QTextCursor cursor(doc);
    
    QTextDocument::FindFlags flags;
    if (caseSensitive) flags |= QTextDocument::FindCaseSensitively;
    if (wholeWord) flags |= QTextDocument::FindWholeWords;
    
    int count = 0;
    
    if (regex) {
        QRegularExpression re(text, caseSensitive ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
        cursor = doc->find(re, 0, flags);
        while (!cursor.isNull()) {
            QTextEdit::ExtraSelection sel;
            sel.format.setBackground(QColor(255, 255, 0, 100));
            sel.cursor = cursor;
            m_SearchSelections << sel;
            count++;
            cursor = doc->find(re, cursor, flags);
        }
    } else {
        cursor = doc->find(text, 0, flags);
        while (!cursor.isNull()) {
            QTextEdit::ExtraSelection sel;
            sel.format.setBackground(QColor(255, 255, 0, 100));
            sel.cursor = cursor;
            m_SearchSelections << sel;
            count++;
            cursor = doc->find(text, cursor, flags);
        }
    }
    
    highlightSearchResults();
    
    // Move to first match
    if (count > 0) {
        findNext();
    }
    
    return count;
}

int AdvancedCodeEditor::replaceText(const QString &find, const QString &replace, bool all)
{
    if (find.isEmpty()) return 0;
    
    QTextCursor cursor = textCursor();
    int count = 0;
    
    if (all) {
        cursor.beginEditBlock();
        cursor.movePosition(QTextCursor::Start);
        setTextCursor(cursor);
        
        while (textCursor().hasSelection() || find != replace) {
            QTextDocument::FindFlags flags;
            if (m_LastSearchCaseSensitive) flags |= QTextDocument::FindCaseSensitively;
            if (m_LastSearchWholeWord) flags |= QTextDocument::FindWholeWords;
            
            if (!QPlainTextEdit::find(find, flags)) break;
            
            QTextCursor c = textCursor();
            c.insertText(replace);
            count++;
        }
        
        cursor.endEditBlock();
    } else {
        if (cursor.hasSelection() && cursor.selectedText() == find) {
            cursor.insertText(replace);
            count = 1;
        }
        findNext();
    }
    
    // Refresh search highlights
    if (!m_LastSearchText.isEmpty()) {
        findText(m_LastSearchText, m_LastSearchCaseSensitive, m_LastSearchWholeWord, m_LastSearchRegex);
    }
    
    return count;
}

void AdvancedCodeEditor::findNext()
{
    if (m_LastSearchText.isEmpty()) return;
    
    QTextDocument::FindFlags flags;
    if (m_LastSearchCaseSensitive) flags |= QTextDocument::FindCaseSensitively;
    if (m_LastSearchWholeWord) flags |= QTextDocument::FindWholeWords;
    
    bool found;
    if (m_LastSearchRegex) {
        QRegularExpression re(m_LastSearchText, m_LastSearchCaseSensitive ? 
            QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
        found = QPlainTextEdit::find(re, flags);
    } else {
        found = QPlainTextEdit::find(m_LastSearchText, flags);
    }
    
    if (!found) {
        // Wrap around
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Start);
        setTextCursor(cursor);
        
        if (m_LastSearchRegex) {
            QRegularExpression re(m_LastSearchText, m_LastSearchCaseSensitive ? 
                QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
            QPlainTextEdit::find(re, flags);
        } else {
            QPlainTextEdit::find(m_LastSearchText, flags);
        }
    }
}

void AdvancedCodeEditor::findPrevious()
{
    if (m_LastSearchText.isEmpty()) return;
    
    QTextDocument::FindFlags flags = QTextDocument::FindBackward;
    if (m_LastSearchCaseSensitive) flags |= QTextDocument::FindCaseSensitively;
    if (m_LastSearchWholeWord) flags |= QTextDocument::FindWholeWords;
    
    bool found;
    if (m_LastSearchRegex) {
        QRegularExpression re(m_LastSearchText, m_LastSearchCaseSensitive ? 
            QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
        found = QPlainTextEdit::find(re, flags);
    } else {
        found = QPlainTextEdit::find(m_LastSearchText, flags);
    }
    
    if (!found) {
        // Wrap around
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::End);
        setTextCursor(cursor);
        
        if (m_LastSearchRegex) {
            QRegularExpression re(m_LastSearchText, m_LastSearchCaseSensitive ? 
                QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
            QPlainTextEdit::find(re, flags);
        } else {
            QPlainTextEdit::find(m_LastSearchText, flags);
        }
    }
}

QStringList AdvancedCodeEditor::getSymbols()
{
    return m_Symbols;
}

void AdvancedCodeEditor::gotoSymbol(const QString &symbol)
{
    // Search for the symbol in the document
    QTextDocument *doc = document();
    QTextCursor cursor = doc->find(symbol, 0);
    if (!cursor.isNull()) {
        setTextCursor(cursor);
        centerCursor();
    }
}

void AdvancedCodeEditor::extractSymbols()
{
    m_Symbols.clear();
    QString content = toPlainText();
    
    if (m_FileType == "smali") {
        // Extract method names
        QRegularExpression methodRe("\\.method[^\\n]+ (\\w+)\\(");
        auto it = methodRe.globalMatch(content);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            m_Symbols << match.captured(1);
        }
        // Extract field names
        QRegularExpression fieldRe("\\.field[^\\n]+ (\\w+):");
        it = fieldRe.globalMatch(content);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            m_Symbols << match.captured(1);
        }
    } else if (m_FileType == "java" || m_FileType == "kotlin") {
        // Extract class/method/field names
        QRegularExpression classRe("(class|interface|enum)\\s+(\\w+)");
        auto it = classRe.globalMatch(content);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            m_Symbols << match.captured(2);
        }
        QRegularExpression methodRe("(public|private|protected)?\\s*(static)?\\s*\\w+\\s+(\\w+)\\s*\\(");
        it = methodRe.globalMatch(content);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            m_Symbols << match.captured(3);
        }
    } else if (m_FileType == "dart") {
        // Extract class/method names
        QRegularExpression classRe("class\\s+(\\w+)");
        auto it = classRe.globalMatch(content);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            m_Symbols << match.captured(1);
        }
        QRegularExpression funcRe("(\\w+)\\s+\\w+\\s*\\(");
        it = funcRe.globalMatch(content);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            m_Symbols << match.captured(1);
        }
    } else if (m_FileType == "xml" || m_FileType == "android-manifest") {
        // Extract android:id values
        QRegularExpression idRe("android:id=\"@\\+id/(\\w+)\"");
        auto it = idRe.globalMatch(content);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            m_Symbols << match.captured(1);
        }
    }
    
    m_Symbols.removeDuplicates();
    m_Symbols.sort();
    emit symbolsChanged(m_Symbols);
}

void AdvancedCodeEditor::toggleBookmark(int line)
{
    if (line < 0) {
        line = textCursor().blockNumber();
    }
    
    if (m_Bookmarks.contains(line)) {
        m_Bookmarks.remove(line);
    } else {
        m_Bookmarks.insert(line);
    }
    
    m_Sidebar->update();
}

void AdvancedCodeEditor::nextBookmark()
{
    if (m_Bookmarks.isEmpty()) return;
    
    int currentLine = textCursor().blockNumber();
    QList<int> bookmarks = m_Bookmarks.values();
    std::sort(bookmarks.begin(), bookmarks.end());
    
    for (int line : bookmarks) {
        if (line > currentLine) {
            gotoLine(line + 1);
            return;
        }
    }
    
    // Wrap to first bookmark
    gotoLine(bookmarks.first() + 1);
}

void AdvancedCodeEditor::previousBookmark()
{
    if (m_Bookmarks.isEmpty()) return;
    
    int currentLine = textCursor().blockNumber();
    QList<int> bookmarks = m_Bookmarks.values();
    std::sort(bookmarks.begin(), bookmarks.end());
    
    for (int i = bookmarks.size() - 1; i >= 0; --i) {
        if (bookmarks[i] < currentLine) {
            gotoLine(bookmarks[i] + 1);
            return;
        }
    }
    
    // Wrap to last bookmark
    gotoLine(bookmarks.last() + 1);
}

void AdvancedCodeEditor::clearBookmarks()
{
    m_Bookmarks.clear();
    m_Sidebar->update();
}

int AdvancedCodeEditor::wordCount() const
{
    QString text = toPlainText();
    if (text.isEmpty()) return 0;
    
    // Split by whitespace and count non-empty parts
    QStringList words = text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    return words.count();
}

int AdvancedCodeEditor::charCount() const
{
    return toPlainText().length();
}

void AdvancedCodeEditor::highlightSearchResults()
{
    QList<QTextEdit::ExtraSelection> selections;
    
    // Current line highlight
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor highlight = palette().color(QPalette::Highlight);
        highlight.setAlpha(30);
        selection.format.setBackground(highlight);
        selection.format.setProperty(QTextCharFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        selections.append(selection);
    }
    
    // Bracket matches
    selections.append(m_BracketSelections);
    
    // Search results
    selections.append(m_SearchSelections);
    
    setExtraSelections(selections);
}

QString AdvancedCodeEditor::formatHtml(const QString &code)
{
    // Basic HTML formatting - indent properly
    QString result;
    int indent = 0;
    QStringList lines = code.split('\n');
    
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) {
            result += "\n";
            continue;
        }
        
        // Check for closing tags
        if (trimmed.startsWith("</")) {
            indent = qMax(0, indent - 1);
        }
        
        result += QString(indent * 2, ' ') + trimmed + "\n";
        
        // Check for opening tags (not self-closing)
        if (trimmed.startsWith("<") && !trimmed.startsWith("</") && 
            !trimmed.endsWith("/>") && !trimmed.startsWith("<!") && !trimmed.startsWith("<?")) {
            indent++;
        }
    }
    
    return result;
}

QString AdvancedCodeEditor::formatCss(const QString &code)
{
    // Basic CSS formatting
    QString result;
    int indent = 0;
    QString current;
    
    for (int i = 0; i < code.length(); ++i) {
        QChar c = code[i];
        
        if (c == '{') {
            result += current.trimmed() + " {\n";
            current.clear();
            indent++;
        } else if (c == '}') {
            if (!current.trimmed().isEmpty()) {
                result += QString(indent * 2, ' ') + current.trimmed() + "\n";
            }
            current.clear();
            indent = qMax(0, indent - 1);
            result += QString(indent * 2, ' ') + "}\n\n";
        } else if (c == ';') {
            result += QString(indent * 2, ' ') + current.trimmed() + ";\n";
            current.clear();
        } else if (c != '\n' && c != '\r') {
            current += c;
        }
    }
    
    return result;
}

QString AdvancedCodeEditor::formatProperties(const QString &code)
{
    // Simple properties file formatting - sort keys
    QStringList lines = code.split('\n');
    QStringList properties;
    QStringList comments;
    
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;
        
        if (trimmed.startsWith('#') || trimmed.startsWith('!')) {
            comments << trimmed;
        } else {
            properties << trimmed;
        }
    }
    
    properties.sort(Qt::CaseInsensitive);
    
    QString result;
    for (const QString &comment : comments) {
        result += comment + "\n";
    }
    if (!comments.isEmpty() && !properties.isEmpty()) {
        result += "\n";
    }
    for (const QString &prop : properties) {
        result += prop + "\n";
    }
    
    return result;
}
