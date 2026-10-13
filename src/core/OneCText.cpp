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

// ─── Локализованный HTML (справка объекта, HTML-макет) ───────────────

namespace {

const char kBom[] = "\xef\xbb\xbf";

bool isSpace(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

bool isBase64Char(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
        || (c >= '0' && c <= '9') || c == '+' || c == '/' || c == '=';
}

/// Данные похожи на HTML: после BOM и пробелов идёт угловая скобка.
bool looksLikeHtml(const QByteArray &data)
{
    int i = 0;
    if (data.startsWith(kBom))
        i = 3;
    while (i < data.size() && isSpace(data.at(i)))
        ++i;
    return i < data.size() && data.at(i) == '<';
}

} // namespace

bool parseLocalizedHtml(const QByteArray &raw, QVector<LocalizedItem> *items)
{
    if (!items)
        return false;
    items->clear();

    int i = 0;
    if (raw.startsWith(kBom))
        i = 3;

    auto skipSpace = [&]() { while (i < raw.size() && isSpace(raw.at(i))) ++i; };
    auto take = [&](char c) {
        skipSpace();
        if (i >= raw.size() || raw.at(i) != c)
            return false;
        ++i;
        return true;
    };

    // {5,<N>,
    if (!take('{'))
        return false;
    if (i >= raw.size() || raw.at(i) != '5')
        return false;
    ++i;
    if (!take(','))
        return false;

    skipSpace();
    int count = 0;
    bool digits = false;
    while (i < raw.size() && raw.at(i) >= '0' && raw.at(i) <= '9') {
        count = count * 10 + (raw.at(i) - '0');
        ++i;
        digits = true;
        if (count > 64)
            return false;                    // защита от мусора
    }
    if (!digits || count <= 0)
        return false;
    if (!take(','))
        return false;

    // <N> пар «"язык", {#base64:<данные>}»
    for (int k = 0; k < count; ++k) {
        skipSpace();
        if (i >= raw.size() || raw.at(i) != '"')
            return false;
        ++i;
        QByteArray lang;
        while (i < raw.size() && raw.at(i) != '"')
            lang.append(raw.at(i++));
        if (i >= raw.size() || lang.isEmpty())
            return false;
        ++i;                                 // закрывающая кавычка
        if (!take(','))
            return false;

        skipSpace();
        const QByteArray open = raw.mid(i, 8);
        if (!open.startsWith("{#base64"))
            return false;
        i += 8;                              // "{#base64" и, если есть, ':'
        if (i < raw.size() && raw.at(i) == ':')
            ++i;

        QByteArray b64;
        while (i < raw.size() && raw.at(i) != '}') {
            const char c = raw.at(i);
            if (isBase64Char(c))
                b64.append(c);
            else if (!isSpace(c))
                return false;                // посторонний символ внутри данных
            ++i;
        }
        if (!take('}'))
            return false;

        LocalizedItem item;
        item.lang = QString::fromUtf8(lang);
        item.data = QByteArray::fromBase64(b64);
        if (!looksLikeHtml(item.data))
            return false;                    // не HTML — элемент не наш
        items->append(item);
    }

    // Хвост структуры: ",<число>}"
    if (!take(','))
        return false;
    skipSpace();
    bool tail = false;
    while (i < raw.size() && raw.at(i) >= '0' && raw.at(i) <= '9') {
        ++i;
        tail = true;
    }
    if (!tail || !take('}'))
        return false;

    return true;
}

QByteArray buildLocalizedHtml(const QVector<LocalizedItem> &items)
{
    // Длина строки base64 — как пишет платформа, 64 символа.
    constexpr int kLine = 64;

    QByteArray out;
    out.append(kBom);
    out.append("{5,");
    out.append(QByteArray::number(items.size()));
    for (const LocalizedItem &it : items) {
        out.append(",\"");
        out.append(it.lang.toUtf8());
        out.append("\",\r\n{#base64:");
        const QByteArray b64 = it.data.toBase64();
        for (int p = 0; p < b64.size(); p += kLine) {
            if (p)
                out.append("\r\n");
            out.append(b64.mid(p, kLine));
        }
        out.append('}');
    }
    out.append(",0}");
    return out;
}

} // namespace OneCText
} // namespace v8unpack
