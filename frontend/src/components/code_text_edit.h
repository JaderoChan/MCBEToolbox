#pragma once

#include <qmap.h>
#include <qplaintextedit.h>

class QPaintEvent;
class QResizeEvent;
class QKeyEvent;

class CodeTextEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    enum ColorRole
    {
        LineNumberBackground,
        LineNumberText,
        CurrentLineBackground,
        MatchedBracketBackground,
        MatchedStringBackground
    };

    explicit CodeTextEdit(QWidget* parent = nullptr);

    bool tabToSpaces() const;
    void setTabToSpaces(bool enabled);

    int  indentWidth() const;
    void setIndentWidth(int width);

    bool wordWrapEnabled() const;
    void setWordWrapEnabled(bool enabled);

    bool showWhitespace() const;
    void setShowWhitespace(bool enabled);

    QColor color(ColorRole role) const;
    void   setColor(ColorRole role, const QColor& color);
    void   resetColor(ColorRole role);

protected:
    void resizeEvent(QResizeEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;

    void lineNumberAreaPaintEvent(QPaintEvent* e);
    int  lineNumberAreaWidth() const;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect& rect, int dy);
    void updateExtraSelections();

private:
    class LineNumberArea;

    QColor defaultColor(ColorRole role) const;
    void   applyWhitespaceFlags();

    // 返回光标附近匹配的括号/字符串引号的一对位置（文档字符下标），未找到时二者均为 -1。
    QPair<int, int> findMatchingBracket(int cursorPos) const;
    QPair<int, int> findMatchingQuote(int cursorPos) const;

    LineNumberArea* lineNumberArea_ = nullptr;

    bool tabToSpaces_    = true;
    int  indentWidth_    = 4;
    bool showWhitespace_ = false;

    QMap<int, QColor> colorOverrides_;
};
