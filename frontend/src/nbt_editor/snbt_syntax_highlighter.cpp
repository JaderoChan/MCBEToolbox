#include "snbt_syntax_highlighter.h"

#include <qfont.h>
#include <qtextdocument.h>

#include <components/color_scheme_manager.h>

SnbtSyntaxHighlighter::SnbtSyntaxHighlighter(QTextDocument* document)
    : QSyntaxHighlighter(document)
{
    rules_.append({QRegularExpression(QStringLiteral(R"(("(?:[^"\\]|\\.)*"|[A-Za-z0-9_\-\.\+]+)(?=\s*:))")), KeyRole});
    rules_.append({QRegularExpression(QStringLiteral(R"("(?:[^"\\]|\\.)*"(?!\s*:))")), StringRole});
    rules_.append({QRegularExpression(QStringLiteral(R"('(?:[^'\\]|\\.)*'(?!\s*:))")), StringRole});
    rules_.append({QRegularExpression(QStringLiteral(R"(\b-?\d+\.\d+[fFdD]?\b|\b-?\d+[bBsSlLfFdD]?\b)")), NumberRole});
    rules_.append({QRegularExpression(QStringLiteral(R"(\btrue\b|\bfalse\b)")), BooleanRole});
    rules_.append({QRegularExpression(QStringLiteral(R"([{}\[\]:,])")), PunctuationRole});

    rebuildFormats();

    connect(
        &ColorSchemeManager::getInstance(), &ColorSchemeManager::loadedColorSchemeChanged,
        this, [this]()
        {
            rebuildFormats();
            rehighlight();
        }
    );
}

QColor SnbtSyntaxHighlighter::color(TokenRole role) const
{
    return colorOverrides_.value(static_cast<int>(role), defaultColor(role));
}

void SnbtSyntaxHighlighter::setColor(TokenRole role, const QColor& color)
{
    colorOverrides_.insert(static_cast<int>(role), color);
    rebuildFormats();
    rehighlight();
}

void SnbtSyntaxHighlighter::resetColor(TokenRole role)
{
    colorOverrides_.remove(static_cast<int>(role));
    rebuildFormats();
    rehighlight();
}

QColor SnbtSyntaxHighlighter::defaultColor(TokenRole role) const
{
    const bool dark = ColorSchemeManager::getInstance().getLoadedColorScheme() == ColorSchemeManager::COLOR_SCHEME_DARK;
    switch (role)
    {
        case KeyRole:         return dark ? QColor(130, 180, 255) : QColor(0, 80, 180);
        case StringRole:      return dark ? QColor(180, 210, 140) : QColor(0, 120, 0);
        case NumberRole:      return dark ? QColor(220, 160, 100) : QColor(180, 90, 0);
        case BooleanRole:     return dark ? QColor(200, 140, 220) : QColor(130, 0, 130);
        case PunctuationRole: return dark ? QColor(180, 180, 180) : QColor(90, 90, 90);
    }
    return QColor();
}

void SnbtSyntaxHighlighter::rebuildFormats()
{
    formats_.clear();
    for (int role = static_cast<int>(KeyRole); role <= static_cast<int>(PunctuationRole); ++role)
    {
        QTextCharFormat format;
        format.setForeground(color(static_cast<TokenRole>(role)));
        formats_.insert(role, format);
    }
}

void SnbtSyntaxHighlighter::highlightBlock(const QString& text)
{
    for (const auto& rule : rules_)
    {
        auto it = rule.pattern.globalMatch(text);
        while (it.hasNext())
        {
            const auto match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), formats_.value(static_cast<int>(rule.role)));
        }
    }
}
