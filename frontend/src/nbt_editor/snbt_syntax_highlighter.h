#pragma once

#include <qcolor.h>
#include <qregularexpression.h>
#include <qsyntaxhighlighter.h>
#include <qtextformat.h>

class QTextDocument;

class SnbtSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    enum TokenRole
    {
        KeyRole,
        StringRole,
        NumberRole,
        PunctuationRole
    };

    explicit SnbtSyntaxHighlighter(QTextDocument* document);

    QColor color(TokenRole role) const;
    void   setColor(TokenRole role, const QColor& color);
    void   resetColor(TokenRole role);

protected:
    void highlightBlock(const QString& text) override;

private:
    QColor defaultColor(TokenRole role) const;
    void   rebuildFormats();

    QMap<int, QColor>          colorOverrides_;
    QMap<int, QTextCharFormat> formats_;
};
