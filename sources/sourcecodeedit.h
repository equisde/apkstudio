#ifndef SOURCECODEEDIT_H
#define SOURCECODEEDIT_H

#include <QMenu>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPlainTextEdit>

class SourceCodeSidebarWidget;

class SourceCodeEdit : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit SourceCodeEdit(QWidget *parent = nullptr);
    QRectF blockBoundingGeometryProxy(const QTextBlock &block);
    QRectF blockBoundingRectProxy(const QTextBlock &block);
    QPointF contentOffsetProxy();
    QString filePath();
    QString fileType() const;
    QTextBlock firstVisibleBlockProxy();
    void gotoLine(const int no);
    void moveCursor(const bool end);
    void open(const QString &path);
    bool save();
    
signals:
    void aiResponseReceived(const QString &response);
    
protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    
private:
    QString m_FilePath;
    QString m_FileType;
    QString m_Encoding;
    SourceCodeSidebarWidget *m_Sidebar;
    QNetworkAccessManager *m_NetworkManager;
    
    int indentSize(const QString &text);
    bool indentText(const bool forward);
    QString indentText(QString text, int count) const;
    void moveSelection(const bool up);
    void transformText(const bool upper);
    void detectFileType(const QString &path);
    void detectEncoding(const QByteArray &data);
    
    // AI functions
    void askAI(const QString &prompt, const QString &context);
    void handleAIResponse(QNetworkReply *reply);
    
private slots:
    void handleBlockCountChanged(const int count);
    void handleCursorPositionChanged();
    void handleUpdateRequest(const QRect &rect, const int column);
    void handleTextChanged();
    
    // AI context menu actions
    void aiExplainCode();
    void aiFixCode();
    void aiFindIssues();
    void aiOptimizeCode();
    void aiAddComments();
    void aiConvertFormat();
};

class SourceCodeSidebarWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SourceCodeSidebarWidget(SourceCodeEdit *edit);
    QSize sizeHint() const;
protected:
    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
private:
    void mouseEvent(QMouseEvent *event);
    SourceCodeEdit *m_Edit;
};

#endif // SOURCECODEEDIT_H
