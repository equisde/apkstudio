#include <QApplication>
#include <QContextMenuEvent>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QPainter>
#include <QScrollBar>
#include <QSettings>
#include <QShortcut>
#include <QTextBlock>
#include <QTextStream>
#include <QToolTip>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#endif
#include "sourcecodeedit.h"
#include "themedsyntaxhighlighter.h"

#define TAB_STOP_WIDTH 4
#define TABS_TO_SPACES true

SourceCodeEdit::SourceCodeEdit(QWidget *parent)
    : QPlainTextEdit(parent), m_NetworkManager(nullptr)
{
    m_Sidebar = new SourceCodeSidebarWidget(this);
    m_NetworkManager = new QNetworkAccessManager(this);
    
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
    font.setPointSize(settings.value("editor_font_size", 11).toInt());
    font.setStyleHint(QFont::Monospace);
    const bool whitespaces = settings.value("editor_whitespaces", false).toBool();
    if (whitespaces) {
        QTextOption options;
        options.setFlags(QTextOption::ShowTabsAndSpaces);
        document()->setDefaultTextOption(options);
    }
    setCursorWidth(2);
    setFrameStyle(QFrame::NoFrame);
    setFont(font);
    setTabChangesFocus(false);
    setWordWrapMode(QTextOption::NoWrap);
    
    // Set tab width
    QFontMetrics metrics(font);
    setTabStopDistance(TAB_STOP_WIDTH * metrics.horizontalAdvance(' '));
    
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &SourceCodeEdit::handleCursorPositionChanged);
    connect(this, &QPlainTextEdit::blockCountChanged, this, &SourceCodeEdit::handleBlockCountChanged);
    connect(this, &QPlainTextEdit::textChanged, this, &SourceCodeEdit::handleTextChanged);
    connect(this, &QPlainTextEdit::updateRequest, this, &SourceCodeEdit::handleUpdateRequest);
    connect(new QShortcut(Qt::CTRL | Qt::Key_U, this), &QShortcut::activated, [=] {
        transformText(true);
    });
    connect(new QShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_U, this), &QShortcut::activated, [=] {
        transformText(false);
    });
    connect(new QShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_Up, this), &QShortcut::activated, [=] {
        moveSelection(true);
    });
    connect(new QShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_Down, this), &QShortcut::activated, [=] {
        moveSelection(false);
    });
}

QRectF SourceCodeEdit::blockBoundingGeometryProxy(const QTextBlock &block)
{
    return blockBoundingGeometry(block);
}

QRectF SourceCodeEdit::blockBoundingRectProxy(const QTextBlock &block)
{
    return blockBoundingRect(block);
}

QPointF SourceCodeEdit::contentOffsetProxy()
{
    return contentOffset();
}

QString SourceCodeEdit::filePath()
{
    return m_FilePath;
}

QTextBlock SourceCodeEdit::firstVisibleBlockProxy()
{
    return firstVisibleBlock();
}

void SourceCodeEdit::gotoLine(const int no)
{
    QTextCursor cursor(document()->findBlockByLineNumber(no - 1));
    setTextCursor(cursor);
}

void SourceCodeEdit::handleBlockCountChanged(const int count)
{
    Q_UNUSED(count)
    setViewportMargins(m_Sidebar->sizeHint().width(), 0, 0, 0);
}

void SourceCodeEdit::handleCursorPositionChanged()
{
    QList<QTextEdit::ExtraSelection> selections;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        static QColor highlight = palette().color(QPalette::Text);
        highlight.setAlpha(25);
        selection.format.setBackground(highlight);
        selection.format.setProperty(QTextCharFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        selections.append(selection);
    }
    setExtraSelections(selections);
}

void SourceCodeEdit::handleUpdateRequest(const QRect &rect, const int column)
{
    if (column) {
        m_Sidebar->scroll(0, column);
    }
    m_Sidebar->update(0, rect.y(), m_Sidebar->width(), rect.height());
    if (rect.contains(viewport()->rect())) {
        handleBlockCountChanged(0);
    }
}

void SourceCodeEdit::handleTextChanged()
{
    handleCursorPositionChanged();
    handleBlockCountChanged(0);
}

int SourceCodeEdit::indentSize(const QString &text)
{
    int count = 0;
    int i = 0;
    int length = text.length();
    if (length == 0) {
        return 0;
    }
    QChar current = text.at(i);
    while ((i < length) && ((current == ' ') || (current == QChar('\t')))) {
        if (current == QChar('\t')) {
            count++;
            i++;
        } else if (current == ' ') {
            int j = 0;
            while ((i + j < length) && (text.at(i + j) == ' ')) {
                j++;
            }
            i += j;
            count += j / TAB_STOP_WIDTH;
        }
        if (i < length) {
            current = text.at(i);
        }
    }
    return count;
}

bool SourceCodeEdit::indentText(const bool forward)
{
    QTextCursor cursor = textCursor();
    QTextCursor clone = cursor;
    if (!cursor.hasSelection()) {
        return false;
    }
    int start = cursor.selectionStart();
    cursor.setPosition(cursor.selectionStart());
    clone.setPosition(clone.selectionEnd());
    int stop = clone.blockNumber();
    cursor.beginEditBlock();
    do {
        int position = cursor.position();
        QString text = cursor.block().text();
        int count = indentSize(text);
        if (forward) {
            count++;
        } else if (count > 0) {
            count--;
        }
        cursor.select(QTextCursor::LineUnderCursor);
        if (forward) {
            cursor.insertText(TABS_TO_SPACES ? QString(TAB_STOP_WIDTH, ' ') + text : QChar('\t') + text);
        } else {
            cursor.insertText(indentText(text, count));
        }
        cursor.setPosition(position);
        if (!cursor.movePosition(QTextCursor::NextBlock)) {
            break;
        }
    }
    while(cursor.blockNumber() <= stop);
    cursor.setPosition(start, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
    while (cursor.block().blockNumber() < stop) {
        cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
    }
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    cursor.endEditBlock();
    setTextCursor(cursor);
    return true;
}

QString SourceCodeEdit::indentText(QString text, int count) const
{
    if (text.isEmpty()) {
        return text;
    }
    while ((text.at(0) == ' ') || (text.at(0) == '\t')) {
        text.remove(0, 1);
        if (text.isEmpty()) {
            break;
        }
    }
    while (count != 0) {
        text = text.insert(0, TABS_TO_SPACES ? QString(TAB_STOP_WIDTH, ' ') : QChar('\t'));
        count--;
    }
    return text;
}

void SourceCodeEdit::keyPressEvent(QKeyEvent *event)
{
    QTextCursor cursor = textCursor();
    switch (event->key()) {
    case Qt::Key_Backtab:
    case Qt::Key_Tab: {
        bool forward = !QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);
        if (indentText(forward)) {
            event->accept();
            return;
        } else if (forward) {
            QString text = TABS_TO_SPACES ? QString(TAB_STOP_WIDTH, ' ') : QChar('\t');
            QTextCursor cursor = textCursor();
            cursor.insertText(text);
            setTextCursor(cursor);
            event->accept();
            return;
        }
        break;
    }
    case Qt::Key_Enter:
    case Qt::Key_Return:
        break;
    case Qt::Key_Down:
    case Qt::Key_Up:
        if (QApplication::keyboardModifiers().testFlag(Qt::ControlModifier))  {
            if (event->key() == Qt::Key_Down) {
                verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
            } else {
                verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
            }
            event->accept();
        }
        break;
    case Qt::Key_Escape:
        if (cursor.hasSelection()) {
            cursor.clearSelection();
            setTextCursor(cursor);
            event->accept();
        }
        break;
    case Qt::Key_Home:
    case Qt::Key_End:
        if (!QApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) {
            moveCursor(event->key() != Qt::Key_Home);
            event->accept();
        }
        return;
    case Qt::Key_PageDown:
    case Qt::Key_PageUp:
        if (QApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) {
            if (event->key() == Qt::Key_Down) {
                verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepAdd);
            } else {
                verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepSub);
            }
            event->accept();
        }
        break;
    default:
        break;
    }
    QPlainTextEdit::keyPressEvent(event);
}

void SourceCodeEdit::moveCursor(const bool end)
{
    QTextCursor cursor = textCursor();
    int length = cursor.block().text().length();
    if (length != 0) {
        int original  = cursor.position();
        QTextCursor::MoveMode mode = QTextCursor::MoveAnchor;
        if (QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier)) {
            mode = QTextCursor::KeepAnchor;
        }
        cursor.movePosition(QTextCursor::StartOfLine, mode);
        int start = cursor.position();
        int i;
        if (end) {
            cursor.movePosition(QTextCursor::EndOfLine, mode);
            i = length;
            while (cursor.block().text()[i - 1].isSpace()) {
                i--;
                if (i == 1) {
                    i = length;
                    break;
                }
            }
        } else {
            i = 0;
            while (cursor.block().text()[i].isSpace()) {
                i++;
                if (i == length) {
                    i = 0;
                    break;
                }
            }
        }
        if ((original == start) || ((start + i) != original)) {
            cursor.setPosition(start + i, mode);
        }
        setTextCursor(cursor);
    }
}

void SourceCodeEdit::moveSelection(const bool up)
{
    QTextCursor original = textCursor();
    QTextCursor moved = original;
    moved.setVisualNavigation(false);
    moved.beginEditBlock();
    bool selected = original.hasSelection();
    if (selected) {
        moved.setPosition(original.selectionStart());
        moved.movePosition(QTextCursor::StartOfBlock);
        moved.setPosition(original.selectionEnd(), QTextCursor::KeepAnchor);
        moved.movePosition(moved.atBlockStart() ? QTextCursor::Left : QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    } else {
        moved.movePosition(QTextCursor::StartOfBlock);
        moved.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    }
    QString text = moved.selectedText();
    moved.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
    moved.removeSelectedText();
    if (up) {
        moved.movePosition(QTextCursor::PreviousBlock);
        moved.insertBlock();
        moved.movePosition(QTextCursor::Left);
    } else {
        moved.movePosition(QTextCursor::EndOfBlock);
        if (moved.atBlockStart()) {
            moved.movePosition(QTextCursor::NextBlock);
            moved.insertBlock();
            moved.movePosition(QTextCursor::Left);
        } else {
            moved.insertBlock();
        }
    }
    int start = moved.position();
    moved.clearSelection();
    moved.insertText(text);
    int end = moved.position();
    if (selected) {
        moved.setPosition(start);
        moved.setPosition(end, QTextCursor::KeepAnchor);
    }
    moved.endEditBlock();
    setTextCursor(moved);
}

void SourceCodeEdit::open(const QString &path)
{
    QFile file(path);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QByteArray rawData = file.readAll();
        detectEncoding(rawData);
        auto content = QString::fromUtf8(rawData);
        setPlainText(content);
        QFileInfo info(path);
        QString extension = info.suffix().toLower();
        detectFileType(path);
        QSettings settings;
        const bool dark = settings.value("dark_theme", false).toBool();
        new ThemedSyntaxHighlighter(
                    ThemedSyntaxHighlighter::theme(dark ? "dark" : "light"),
                    ThemedSyntaxHighlighter::definitions(extension),
                    document());
    }
    m_FilePath = path;
}

void SourceCodeEdit::paintEvent(QPaintEvent *event)
{
    QPainter line(viewport());
    const int offset = static_cast<int>((fontMetrics().horizontalAdvance('8') * 80)
                                        + contentOffset().x()
                                        + document()->documentMargin());
    QPen pen = line.pen();
    static QColor eol = palette().color(QPalette::Text);
    eol.setAlpha(50);
    pen.setColor(eol);
    pen.setStyle(Qt::DotLine);
    line.setPen(pen);
    line.drawLine(offset, 0, offset, viewport()->height());
    QPlainTextEdit::paintEvent(event);
}

void SourceCodeEdit::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    QRect rect = contentsRect();
    m_Sidebar->setGeometry(QRect(rect.left(), rect.top(), m_Sidebar->sizeHint().width(), rect.height()));
}

bool SourceCodeEdit::save()
{
    QFile file(m_FilePath);
    if (file.open(QFile::WriteOnly | QFile::Text)) {
        QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        out.setCodec("UTF-8");
#else
        out.setEncoding(QStringConverter::Utf8);
#endif
        out.setGenerateByteOrderMark(false);
        out << toPlainText();
        out.flush();
        file.close();
        return true;
    }
    return false;
}

void SourceCodeEdit::transformText(const bool upper)
{
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::WordUnderCursor);
    }
    QString before = cursor.selectedText();
    QString after = upper ? before.toUpper() : before.toLower();
    if (before != after) {
        cursor.beginEditBlock();
        cursor.deleteChar();
        cursor.insertText(after);
        cursor.endEditBlock();
        setTextCursor(cursor);
    }
}

void SourceCodeEdit::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        const int delta = event->angleDelta().y();
#else
        const int delta = event->delta();
#endif
        if (delta > 0) {
            zoomIn();
        } else if (delta < 0) {
            zoomOut();
        }
    } else {
        QPlainTextEdit::wheelEvent(event);
    }
}

SourceCodeSidebarWidget::SourceCodeSidebarWidget(SourceCodeEdit *edit)
    : QWidget(edit), m_Edit(edit)
{
}

void SourceCodeSidebarWidget::leaveEvent(QEvent *e)
{
    Q_UNUSED(e)
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QMouseEvent copy(QEvent::MouseMove, QPointF(-1, -1), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
#else
    QMouseEvent copy(QEvent::MouseMove, QPoint(-1, -1), Qt::NoButton, nullptr, nullptr);
#endif
    mouseEvent(&copy);
}

void SourceCodeSidebarWidget::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    QTextBlock block = m_Edit->firstVisibleBlockProxy();
    int i = block.blockNumber();
    int top = static_cast<int>(m_Edit->blockBoundingGeometryProxy(block).translated(m_Edit->contentOffsetProxy()).top());
    int bottom = top + static_cast<int>(m_Edit->blockBoundingRectProxy(block).height());
    QRect full = e->rect();
    painter.fillRect(full, palette().color(QPalette::Base));
    while (block.isValid() && (top <= full.bottom())) {
        if (block.isVisible() && (bottom >= full.top())) {
            QRect box(0, top, width(), m_Edit->fontMetrics().height());
            QFont font = painter.font();
            font.setFamily(m_Edit->font().family());
            font.setPointSize(m_Edit->font().pointSize());
            if (m_Edit->textCursor().blockNumber() == i) {
                painter.fillRect(box, palette().color(QPalette::Highlight));
                painter.setPen(palette().color(QPalette::HighlightedText));
                font.setWeight(QFont::Bold);
            } else {
                font.setWeight(QFont::Normal);
                painter.setPen(palette().color(QPalette::Text));
            }
            painter.setFont(font);
            painter.drawText(box.left(), box.top(), box.width(), box.height(), Qt::AlignRight, QString::number(i + 1).append(' '));
            painter.setPen(palette().color(QPalette::Highlight));
            painter.drawLine(full.topRight(), full.bottomRight());
        }
        block = block.next();
        top = bottom;
        bottom = (top + static_cast<int>(m_Edit->blockBoundingRectProxy(block).height()));
        ++i;
    }
}

void SourceCodeSidebarWidget::mouseEvent(QMouseEvent *e)
{
    QTextCursor cursor = m_Edit->cursorForPosition(QPoint(0, e->pos().y()));
    if ((e->type() == QEvent::MouseButtonPress) && (e->button() == Qt::LeftButton)) {
        cursor.movePosition(QTextCursor::EndOfBlock);
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
        cursor.setVisualNavigation(true);
        m_Edit->setTextCursor(cursor);
    }
}

void SourceCodeSidebarWidget::mouseMoveEvent(QMouseEvent *event)
{
    mouseEvent(event);
}
void SourceCodeSidebarWidget::mousePressEvent(QMouseEvent *event)
{
    mouseEvent(event);
}
void SourceCodeSidebarWidget::mouseReleaseEvent(QMouseEvent *event)
{
    mouseEvent(event);
}

QSize SourceCodeSidebarWidget::sizeHint() const
{
    int digits = 1;
    int blocks = qMax(1, m_Edit->blockCount());
    while (blocks >= 10) {
        blocks /= 10;
        digits++;
    }
    digits++;
    digits++;
    return QSize((3 + (m_Edit->fontMetrics().horizontalAdvance('8') * digits)), 0);
}

void SourceCodeSidebarWidget::wheelEvent(QWheelEvent *e)
{
    QApplication::sendEvent(m_Edit->viewport(), e);
}

// === AI-Powered Features ===

QString SourceCodeEdit::fileType() const
{
    return m_FileType;
}

void SourceCodeEdit::detectFileType(const QString &path)
{
    QFileInfo info(path);
    QString ext = info.suffix().toLower();
    
    if (ext == "smali") m_FileType = "smali";
    else if (ext == "java") m_FileType = "java";
    else if (ext == "xml") m_FileType = "xml";
    else if (ext == "json") m_FileType = "json";
    else if (ext == "yml" || ext == "yaml") m_FileType = "yaml";
    else if (ext == "properties") m_FileType = "properties";
    else if (ext == "txt") m_FileType = "text";
    else if (ext == "md" || ext == "markdown") m_FileType = "markdown";
    else if (ext == "html" || ext == "htm") m_FileType = "html";
    else if (ext == "css") m_FileType = "css";
    else if (ext == "js") m_FileType = "javascript";
    else if (ext == "kt" || ext == "kts") m_FileType = "kotlin";
    else if (ext == "gradle") m_FileType = "gradle";
    else m_FileType = "text";
}

void SourceCodeEdit::detectEncoding(const QByteArray &data)
{
    // Simple BOM detection
    if (data.startsWith("\xEF\xBB\xBF")) {
        m_Encoding = "UTF-8-BOM";
    } else if (data.startsWith("\xFF\xFE")) {
        m_Encoding = "UTF-16LE";
    } else if (data.startsWith("\xFE\xFF")) {
        m_Encoding = "UTF-16BE";
    } else {
        m_Encoding = "UTF-8";
    }
}

void SourceCodeEdit::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();
    
    QSettings settings;
    bool aiEnabled = settings.value("ai_enabled", false).toBool();
    QString apiKey = settings.value("ai_api_key").toString();
    
    if (aiEnabled && !apiKey.isEmpty()) {
        menu->addSeparator();
        
        QMenu *aiMenu = menu->addMenu(tr("🤖 AI Assistant"));
        
        QAction *explainAction = aiMenu->addAction(tr("Explain Selected Code"));
        explainAction->setEnabled(textCursor().hasSelection());
        connect(explainAction, &QAction::triggered, this, &SourceCodeEdit::aiExplainCode);
        
        QAction *fixAction = aiMenu->addAction(tr("Fix Issues in Selection"));
        fixAction->setEnabled(textCursor().hasSelection());
        connect(fixAction, &QAction::triggered, this, &SourceCodeEdit::aiFixCode);
        
        QAction *issuesAction = aiMenu->addAction(tr("Find Security Issues"));
        connect(issuesAction, &QAction::triggered, this, &SourceCodeEdit::aiFindIssues);
        
        QAction *optimizeAction = aiMenu->addAction(tr("Optimize Code"));
        optimizeAction->setEnabled(textCursor().hasSelection());
        connect(optimizeAction, &QAction::triggered, this, &SourceCodeEdit::aiOptimizeCode);
        
        QAction *commentsAction = aiMenu->addAction(tr("Add Comments"));
        commentsAction->setEnabled(textCursor().hasSelection());
        connect(commentsAction, &QAction::triggered, this, &SourceCodeEdit::aiAddComments);
        
        aiMenu->addSeparator();
        
        QAction *convertAction = aiMenu->addAction(tr("Convert Format..."));
        convertAction->setEnabled(textCursor().hasSelection());
        connect(convertAction, &QAction::triggered, this, &SourceCodeEdit::aiConvertFormat);
    }
    
    menu->exec(event->globalPos());
    delete menu;
}

void SourceCodeEdit::aiExplainCode()
{
    QString selected = textCursor().selectedText();
    if (selected.isEmpty()) return;
    
    // Replace paragraph separators with newlines
    selected.replace(QChar::ParagraphSeparator, '\n');
    
    QString prompt = QString(
        "Explain this %1 code in detail. What does it do? "
        "Break down each significant part:\n\n```%1\n%2\n```"
    ).arg(m_FileType, selected);
    
    askAI(prompt, selected);
}

void SourceCodeEdit::aiFixCode()
{
    QString selected = textCursor().selectedText();
    if (selected.isEmpty()) return;
    
    selected.replace(QChar::ParagraphSeparator, '\n');
    
    QString prompt = QString(
        "Fix any bugs, errors, or issues in this %1 code. "
        "Return ONLY the corrected code without explanations:\n\n```%1\n%2\n```"
    ).arg(m_FileType, selected);
    
    askAI(prompt, selected);
}

void SourceCodeEdit::aiFindIssues()
{
    QString content = toPlainText();
    if (content.length() > 10000) {
        content = content.left(10000) + "\n... (truncated)";
    }
    
    QString prompt = QString(
        "Analyze this %1 file for security vulnerabilities, bugs, and potential issues. "
        "List each issue with its line number (if possible), severity (HIGH/MEDIUM/LOW), and how to fix it:\n\n"
        "```%1\n%2\n```"
    ).arg(m_FileType, content);
    
    askAI(prompt, content);
}

void SourceCodeEdit::aiOptimizeCode()
{
    QString selected = textCursor().selectedText();
    if (selected.isEmpty()) return;
    
    selected.replace(QChar::ParagraphSeparator, '\n');
    
    QString prompt = QString(
        "Optimize this %1 code for better performance and readability. "
        "Return ONLY the optimized code without explanations:\n\n```%1\n%2\n```"
    ).arg(m_FileType, selected);
    
    askAI(prompt, selected);
}

void SourceCodeEdit::aiAddComments()
{
    QString selected = textCursor().selectedText();
    if (selected.isEmpty()) return;
    
    selected.replace(QChar::ParagraphSeparator, '\n');
    
    QString prompt = QString(
        "Add helpful comments to this %1 code explaining what each section does. "
        "Return the code with comments added:\n\n```%1\n%2\n```"
    ).arg(m_FileType, selected);
    
    askAI(prompt, selected);
}

void SourceCodeEdit::aiConvertFormat()
{
    QString selected = textCursor().selectedText();
    if (selected.isEmpty()) return;
    
    selected.replace(QChar::ParagraphSeparator, '\n');
    
    QString prompt = QString(
        "Convert this %1 code/data to a different format. "
        "If it's JSON, convert to YAML. If YAML, convert to JSON. "
        "If it's Java, convert to Kotlin. If smali, explain what the Java equivalent would be. "
        "Return ONLY the converted content:\n\n```%1\n%2\n```"
    ).arg(m_FileType, selected);
    
    askAI(prompt, selected);
}

void SourceCodeEdit::askAI(const QString &prompt, const QString &context)
{
    Q_UNUSED(context)
    
    QSettings settings;
    QString provider = settings.value("ai_provider", "gemini").toString();
    QString model = settings.value("ai_model", "gemini-2.0-flash-exp").toString();
    QString apiKey = settings.value("ai_api_key").toString();
    
    if (apiKey.isEmpty()) {
        QMessageBox::warning(this, tr("AI Not Configured"), 
            tr("Please configure your AI API key in Settings > AI Assistant."));
        return;
    }
    
    // Build request
    QString endpoint;
    QJsonObject root;
    
    if (provider == "gemini") {
        endpoint = QString("https://generativelanguage.googleapis.com/v1beta/models/%1:generateContent?key=%2")
            .arg(model, apiKey);
        
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
    
    QToolTip::showText(mapToGlobal(cursorRect().topLeft()), tr("🤖 AI is thinking..."), this);
    
    QNetworkReply *reply = m_NetworkManager->post(request, QJsonDocument(root).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleAIResponse(reply);
    });
}

void SourceCodeEdit::handleAIResponse(QNetworkReply *reply)
{
    QToolTip::hideText();
    
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::warning(this, tr("AI Error"), 
            tr("Failed to get AI response: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }
    
    QByteArray data = reply->readAll();
    reply->deleteLater();
    
    QSettings settings;
    QString provider = settings.value("ai_provider", "gemini").toString();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QString responseText;
    
    if (provider == "gemini") {
        QJsonArray candidates = doc.object()["candidates"].toArray();
        if (!candidates.isEmpty()) {
            QJsonObject content = candidates[0].toObject()["content"].toObject();
            QJsonArray parts = content["parts"].toArray();
            if (!parts.isEmpty()) {
                responseText = parts[0].toObject()["text"].toString();
            }
        }
    } else if (provider == "openai" || provider == "copilot") {
        QJsonArray choices = doc.object()["choices"].toArray();
        if (!choices.isEmpty()) {
            responseText = choices[0].toObject()["message"].toObject()["content"].toString();
        }
    } else if (provider == "anthropic") {
        QJsonArray content = doc.object()["content"].toArray();
        if (!content.isEmpty()) {
            responseText = content[0].toObject()["text"].toString();
        }
    }
    
    if (responseText.isEmpty()) {
        QMessageBox::information(this, tr("AI Response"), tr("No response received."));
        return;
    }
    
    // Check if response contains code block - if so, offer to replace selection
    QRegularExpression codeBlockRegex("```[a-z]*\\n([\\s\\S]*?)\\n```");
    QRegularExpressionMatch match = codeBlockRegex.match(responseText);
    
    if (match.hasMatch() && textCursor().hasSelection()) {
        QString code = match.captured(1);
        
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("AI Response"));
        msgBox.setText(tr("AI has suggested code changes."));
        msgBox.setInformativeText(tr("Do you want to replace the selected text with the AI suggestion?"));
        msgBox.setDetailedText(code);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        
        if (msgBox.exec() == QMessageBox::Yes) {
            QTextCursor cursor = textCursor();
            cursor.beginEditBlock();
            cursor.removeSelectedText();
            cursor.insertText(code);
            cursor.endEditBlock();
            setTextCursor(cursor);
        }
    } else {
        // Show response in a message box
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("AI Response"));
        msgBox.setText(tr("AI Analysis Complete"));
        msgBox.setDetailedText(responseText);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
    }
    
    emit aiResponseReceived(responseText);
}
