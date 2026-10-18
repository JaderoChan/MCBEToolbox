#include "code_text_edit.h"

#include <qevent.h>
#include <qpainter.h>
#include <qtextobject.h>

#include "components/color_scheme_manager.h"

class CodeTextEdit::LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(CodeTextEdit* editor)
        : QWidget(editor), editor_(editor)
    {}

    QSize sizeHint() const override
    {
        return QSize(editor_->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent* e) override
    {
        editor_->lineNumberAreaPaintEvent(e);
    }

private:
    CodeTextEdit* editor_ = nullptr;
};

CodeTextEdit::CodeTextEdit(QWidget* parent)
    : QPlainTextEdit(parent)
{
    lineNumberArea_ = new LineNumberArea(this);

    connect(this, &QPlainTextEdit::blockCountChanged,     this, &CodeTextEdit::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest,         this, &CodeTextEdit::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &CodeTextEdit::updateExtraSelections);
    connect(this, &QPlainTextEdit::textChanged,           this, &CodeTextEdit::updateExtraSelections);
    connect(
        &ColorSchemeManager::getInstance(), &ColorSchemeManager::loadedColorSchemeChanged,
        this, [this]()
        {
            lineNumberArea_->update();
            updateExtraSelections();
        }
    );

    updateLineNumberAreaWidth(0);
    setIndentWidth(indentWidth_);
    applyWhitespaceFlags();
    updateExtraSelections();
}

bool CodeTextEdit::tabToSpaces() const
{
    return tabToSpaces_;
}

void CodeTextEdit::setTabToSpaces(bool enabled)
{
    tabToSpaces_ = enabled;
}

int CodeTextEdit::indentWidth() const
{
    return indentWidth_;
}

void CodeTextEdit::setIndentWidth(int width)
{
    indentWidth_ = qMax(1, width);
    setTabStopDistance(fontMetrics().horizontalAdvance(QLatin1Char(' ')) * indentWidth_);
}

bool CodeTextEdit::wordWrapEnabled() const
{
    return lineWrapMode() != QPlainTextEdit::NoWrap;
}

void CodeTextEdit::setWordWrapEnabled(bool enabled)
{
    setLineWrapMode(enabled ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
}

bool CodeTextEdit::showWhitespace() const
{
    return showWhitespace_;
}

void CodeTextEdit::setShowWhitespace(bool enabled)
{
    showWhitespace_ = enabled;
    applyWhitespaceFlags();
}

QColor CodeTextEdit::color(ColorRole role) const
{
    return colorOverrides_.value(static_cast<int>(role), defaultColor(role));
}

void CodeTextEdit::setColor(ColorRole role, const QColor& color)
{
    colorOverrides_.insert(static_cast<int>(role), color);
    lineNumberArea_->update();
    updateExtraSelections();
}

void CodeTextEdit::resetColor(ColorRole role)
{
    colorOverrides_.remove(static_cast<int>(role));
    lineNumberArea_->update();
    updateExtraSelections();
}

QColor CodeTextEdit::defaultColor(ColorRole role) const
{
    const bool dark =
        ColorSchemeManager::getInstance().getLoadedColorScheme() == ColorSchemeManager::COLOR_SCHEME_DARK;
    switch (role)
    {
        case LineNumberBackground:     return dark ? QColor(45, 45, 45)    : QColor(240, 240, 240);
        case LineNumberText:           return dark ? QColor(150, 150, 150) : QColor(120, 120, 120);
        case CurrentLineBackground:    return dark ? QColor(60, 60, 60)    : QColor(232, 242, 254);
        case MatchedBracketBackground: return dark ? QColor(90, 90, 40)    : QColor(255, 240, 180);
        case MatchedStringBackground:  return dark ? QColor(40, 70, 90)    : QColor(210, 235, 255);
    }
    return QColor();
}

void CodeTextEdit::applyWhitespaceFlags()
{
    QTextOption opt = document()->defaultTextOption();
    QTextOption::Flags flags = opt.flags();
    if (showWhitespace_) flags |=  QTextOption::ShowTabsAndSpaces;
    else                 flags &= ~QTextOption::ShowTabsAndSpaces;
    opt.setFlags(flags);
    document()->setDefaultTextOption(opt);
}

int CodeTextEdit::lineNumberAreaWidth() const
{
    int digits = 1;
    int maxBlock = qMax(1, blockCount());
    while (maxBlock >= 10)
    {
        maxBlock /= 10;
        ++digits;
    }
    return 8 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits + 8;
}

void CodeTextEdit::lineNumberAreaPaintEvent(QPaintEvent* e)
{
    QPainter painter(lineNumberArea_);
    painter.fillRect(e->rect(), color(LineNumberBackground));

    QTextBlock block      = firstVisibleBlock();
    int        blockIndex = block.blockNumber();
    int        top        = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int        bottom     = top + qRound(blockBoundingRect(block).height());

    painter.setPen(color(LineNumberText));
    while (block.isValid() && top <= e->rect().bottom())
    {
        if (block.isVisible() && bottom >= e->rect().top())
        {
            painter.drawText(
                0, top, lineNumberArea_->width() - 6, fontMetrics().height(),
                Qt::AlignRight, QString::number(blockIndex + 1)
            );
        }
        block  = block.next();
        top    = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockIndex;
    }
}

void CodeTextEdit::resizeEvent(QResizeEvent* e)
{
    QPlainTextEdit::resizeEvent(e);
    const QRect cr = contentsRect();
    lineNumberArea_->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeTextEdit::keyPressEvent(QKeyEvent* e)
{
    if (tabToSpaces_ && e->key() == Qt::Key_Tab)
    {
        QTextCursor cursor = textCursor();
        cursor.insertText(QString(indentWidth_, QLatin1Char(' ')));
        setTextCursor(cursor);
        return;
    }
    QPlainTextEdit::keyPressEvent(e);
}

void CodeTextEdit::updateLineNumberAreaWidth(int /*newBlockCount*/)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeTextEdit::updateLineNumberArea(const QRect& rect, int dy)
{
    if (dy) lineNumberArea_->scroll(0, dy);
    else    lineNumberArea_->update(0, rect.y(), lineNumberArea_->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeTextEdit::updateExtraSelections()
{
    QList<QTextEdit::ExtraSelection> selections;

    if (!isReadOnly())
    {
        QTextEdit::ExtraSelection currentLineSelection;
        currentLineSelection.format.setBackground(color(CurrentLineBackground));
        currentLineSelection.format.setProperty(QTextFormat::FullWidthSelection, true);
        currentLineSelection.cursor = textCursor();
        currentLineSelection.cursor.clearSelection();
        selections.append(currentLineSelection);
    }

    const int pos = textCursor().position();

    const auto bracket = findMatchingBracket(pos);
    if (bracket.first >= 0)
    {
        for (int p : {bracket.first, bracket.second})
        {
            QTextEdit::ExtraSelection selection;
            selection.format.setBackground(color(MatchedBracketBackground));
            selection.cursor = textCursor();
            selection.cursor.setPosition(p);
            selection.cursor.setPosition(p + 1, QTextCursor::KeepAnchor);
            selections.append(selection);
        }
    }

    const auto quote = findMatchingQuote(pos);
    if (quote.first >= 0)
    {
        for (int p : {quote.first, quote.second})
        {
            QTextEdit::ExtraSelection selection;
            selection.format.setBackground(color(MatchedStringBackground));
            selection.cursor = textCursor();
            selection.cursor.setPosition(p);
            selection.cursor.setPosition(p + 1, QTextCursor::KeepAnchor);
            selections.append(selection);
        }
    }

    setExtraSelections(selections);
}

QPair<int, int> CodeTextEdit::findMatchingBracket(int cursorPos) const
{
    static const QString openers = QStringLiteral("{[(");
    static const QString closers = QStringLiteral("}])");

    const QString text = toPlainText();

    auto tryFrom = [&](int pos) -> QPair<int, int>
    {
        if (pos < 0 || pos >= text.size())
            return {-1, -1};

        const QChar ch = text.at(pos);

        const int openerIndex = openers.indexOf(ch);
        if (openerIndex >= 0)
        {
            const QChar open  = ch;
            const QChar close = closers.at(openerIndex);
            int depth = 0;
            for (int i = pos; i < text.size(); ++i)
            {
                if (text.at(i) == open) ++depth;
                else if (text.at(i) == close && --depth == 0) return {pos, i};
            }
            return {-1, -1};
        }

        const int closerIndex = closers.indexOf(ch);
        if (closerIndex >= 0)
        {
            const QChar close = ch;
            const QChar open  = openers.at(closerIndex);
            int depth = 0;
            for (int i = pos; i >= 0; --i)
            {
                if (text.at(i) == close) ++depth;
                else if (text.at(i) == open && --depth == 0) return {i, pos};
            }
            return {-1, -1};
        }

        return {-1, -1};
    };

    QPair<int, int> result = tryFrom(cursorPos);
    if (result.first < 0)
        result = tryFrom(cursorPos - 1);
    return result;
}

QPair<int, int> CodeTextEdit::findMatchingQuote(int cursorPos) const
{
    const QString text = toPlainText();

    int   start     = -1;
    QChar quoteChar = QLatin1Char('\0');

    for (int i = 0; i < text.size(); ++i)
    {
        const QChar ch = text.at(i);
        if ((ch == QLatin1Char('"') || ch == QLatin1Char('\'')) && (i == 0 || text.at(i - 1) != QLatin1Char('\\')))
        {
            if (start < 0)
            {
                start     = i;
                quoteChar = ch;
            }
            else if (ch == quoteChar)
            {
                if (cursorPos >= start && cursorPos <= i)
                    return {start, i};
                start = -1;
            }
        }
    }

    return {-1, -1};
}
