#include "snbt_syntax_highlighter.h"

#include <qfont.h>
#include <qpair.h>
#include <qtextdocument.h>

#include <components/color_scheme_manager.h>

SnbtSyntaxHighlighter::SnbtSyntaxHighlighter(QTextDocument* document)
    : QSyntaxHighlighter(document)
{
    rebuildFormats();

    connect(
        &ColorSchemeManager::instance(), &ColorSchemeManager::loadedColorSchemeChanged,
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
    const bool dark = ColorSchemeManager::instance().loadedColorScheme() == ColorSchemeManager::COLOR_SCHEME_DARK;
    switch (role)
    {
        case KeyRole:         return dark ? QColor(130, 180, 255) : QColor(0, 80, 180);
        case StringRole:      return dark ? QColor(180, 210, 140) : QColor(0, 120, 0);
        case NumberRole:      return dark ? QColor(220, 160, 100) : QColor(180, 90, 0);
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
    QList<QPair<int, int>> claimed;
    auto overlapsClaimed = [&claimed](int start, int end)
    {
        for (const auto& range : claimed)
            if (start < range.second && end > range.first)
                return true;
        return false;
    };

    static const QRegularExpression arrayPattern(QStringLiteral(R"(\[\s*[BILbil]\s*;[^\[\]]*\])"));
    {
        auto it = arrayPattern.globalMatch(text);
        while (it.hasNext())
        {
            const auto m     = it.next();
            const int  start = m.capturedStart();
            const int  end   = m.capturedEnd();
            setFormat(start, end - start, formats_.value(static_cast<int>(NumberRole)));
            claimed.append({start, end});
        }
    }

    static const QRegularExpression quotedPattern(QStringLiteral(R"("(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*')"));
    {
        auto it = quotedPattern.globalMatch(text);
        while (it.hasNext())
        {
            const auto m     = it.next();
            const int  start = m.capturedStart();
            const int  end   = m.capturedEnd();
            if (overlapsClaimed(start, end))
                continue;

            int next = end;
            while (next < text.size() && text.at(next).isSpace())
                ++next;
            const bool isKey = next < text.size() && text.at(next) == QLatin1Char(':');

            setFormat(start, end - start, formats_.value(static_cast<int>(isKey ? KeyRole : StringRole)));

            claimed.append({start, end});
        }
    }

    static const QRegularExpression keyPattern(QStringLiteral(R"((?:\\.|[^\s\\:])+(?=\s*:))"));
    {
        auto it = keyPattern.globalMatch(text);
        while (it.hasNext())
        {
            const auto m     = it.next();
            const int  start = m.capturedStart();
            const int  end   = m.capturedEnd();
            if (overlapsClaimed(start, end))
                continue;
            setFormat(start, end - start, formats_.value(static_cast<int>(KeyRole)));
            claimed.append({start, end});
        }
    }

    static const QRegularExpression numberPattern(QStringLiteral(R"((?<![\w.])-?\d+(?:\.\d+)?[bBsSlLfFdD]?(?![\w.]))"));
    static const QRegularExpression punctPattern(QStringLiteral(R"([{}\[\]:,])"));
    auto applyRemaining = [&](const QRegularExpression& pattern, TokenRole role)
    {
        auto it = pattern.globalMatch(text);
        while (it.hasNext())
        {
            const auto m     = it.next();
            const int  start = m.capturedStart();
            const int  end   = m.capturedEnd();
            if (overlapsClaimed(start, end))
                continue;
            setFormat(start, end - start, formats_.value(static_cast<int>(role)));
        }
    };
    applyRemaining(numberPattern, NumberRole);
    applyRemaining(punctPattern, PunctuationRole);
}
