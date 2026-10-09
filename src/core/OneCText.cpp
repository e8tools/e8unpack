// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "OneCText.h"

namespace v8unpack {
namespace OneCText {

namespace {

bool isTrivia(QChar c)
{
    return c == QLatin1Char(' ') || c == QLatin1Char('\t')
        || c == QLatin1Char('\r') || c == QLatin1Char('\n')
        || c == QLatin1Char(',');
}

class Scanner
{
public:
    explicit Scanner(const QString &text) : m_s(text), m_n(text.size()) {}

    bool topLevelString(int index, int *begin, int *end, QString *value)
    {
        skipTrivia();
        if (m_i >= m_n || m_s.at(m_i) != QLatin1Char('{'))
            return false;
        ++m_i;

        for (int k = 0; ; ++k) {
            skipTrivia();
            if (m_i >= m_n)
                return false;
            if (m_s.at(m_i) == QLatin1Char('}'))
                return false;                       // элементов меньше, чем нужно

            if (k == index) {
                if (m_s.at(m_i) != QLatin1Char('"'))
                    return false;                   // элемент не строка
                return readString(begin, end, value);
            }
            if (!skipValue())
                return false;
        }
    }

private:
    void skipTrivia()
    {
        while (m_i < m_n && isTrivia(m_s.at(m_i)))
            ++m_i;
    }

    /// Позиция на открывающей кавычке.
    bool readString(int *begin, int *end, QString *value)
    {
        const int start = m_i;
        ++m_i;
        QString buf;
        while (m_i < m_n) {
            const QChar c = m_s.at(m_i);
            if (c == QLatin1Char('"')) {
                if (m_i + 1 < m_n && m_s.at(m_i + 1) == QLatin1Char('"')) {
                    buf.append(QLatin1Char('"'));
                    m_i += 2;
                    continue;
                }
                ++m_i;
                if (begin) *begin = start;
                if (end)   *end   = m_i;
                if (value) *value = buf;
                return true;
            }
            buf.append(c);
            ++m_i;
        }
        return false;                               // строка не закрыта
    }

    /// Пропустить одно значение: список, строку или «голый» токен.
    bool skipValue()
    {
        skipTrivia();
        if (m_i >= m_n)
            return false;

        const QChar c = m_s.at(m_i);
        if (c == QLatin1Char('"'))
            return readString(nullptr, nullptr, nullptr);

        if (c == QLatin1Char('{')) {
            ++m_i;
            while (true) {
                skipTrivia();
                if (m_i >= m_n)
                    return false;
                if (m_s.at(m_i) == QLatin1Char('}')) {
                    ++m_i;
                    return true;
                }
                if (!skipValue())
                    return false;
            }
        }

        while (m_i < m_n && m_s.at(m_i) != QLatin1Char(',') && m_s.at(m_i) != QLatin1Char('}'))
            ++m_i;
        return true;
    }

    const QString &m_s;
    int m_i = 0;
    int m_n = 0;
};

} // namespace

bool topLevelStringSpan(const QString &text, int index,
                        int *begin, int *end, QString *value)
{
    Scanner sc(text);
    return sc.topLevelString(index, begin, end, value);
}

QString toElement(const QString &value)
{
    QString out;
    out.reserve(value.size() + 2);
    out.append(QLatin1Char('"'));
    for (const QChar &c : value) {
        if (c == QLatin1Char('"'))
            out.append(QLatin1Char('"'));
        out.append(c);
    }
    out.append(QLatin1Char('"'));
    return out;
}

bool hasBom(const QString &text)
{
    return !text.isEmpty() && text.at(0) == QChar(0xFEFF);
}

QString stripBom(const QString &text)
{
    return hasBom(text) ? text.mid(1) : text;
}

} // namespace OneCText
} // namespace v8unpack
