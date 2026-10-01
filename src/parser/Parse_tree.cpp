//---------------------------------------------------------------------------
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
#include "Parse_tree.h"
#include <QRegularExpression>
#include <QTextStream>
#include <QDebug>

#include <QStringDecoder>



//---------------------------------------------------------------------------
namespace {

inline bool is_ascii_digit(QChar ch) {
    return ch >= u'0' && ch <= u'9';
}

inline bool is_hex_digit(QChar ch) {
    return (ch >= u'0' && ch <= u'9')
        || (ch >= u'a' && ch <= u'f')
        || (ch >= u'A' && ch <= u'F');
}

inline bool is_base64_digit(QChar ch) {
    return (ch >= u'a' && ch <= u'z')
        || (ch >= u'A' && ch <= u'Z')
        || (ch >= u'0' && ch <= u'9')
        || ch == u'+' || ch == u'=' || ch == u'/' || ch == u'\r' || ch == u'\n';
}

bool is_number_fast(const QString& value) {
    const int len = value.length();
    if (len == 0) return false;
    int i = 0;
    if (value[i] == u'-') {
        ++i;
        if (i > len - 1) return false;
    }
    for (; i < len; ++i) {
        if (!is_ascii_digit(value[i])) return false;
    }
    return true;
}

bool is_number_exp_fast(const QString& value) {
    const int len = value.length();
    if (len == 0) return false;
    int i = 0;
    if (value[i] == u'-') {
        ++i;
        if (i > len - 1) return false;
    }
    bool has_digits = false;
    while (i < len && is_ascii_digit(value[i])) {
        has_digits = true;
        ++i;
    }
    if (!has_digits) return false;
    if (i < len && value[i] == u'.') {
        ++i;
        while (i < len && is_ascii_digit(value[i])) ++i;
    }
    if (i < len && (value[i] == u'e' || value[i] == u'E')) {
        ++i;
        if (i < len && value[i] == u'-') ++i;
        if (i >= len || !is_ascii_digit(value[i])) return false;
        while (i < len && is_ascii_digit(value[i])) ++i;
    }
    return i >= len;
}

bool is_guid_fast(const QString& value) {
    static const int expected_len = 36;
    if (value.length() != expected_len) return false;
    for (int i = 0; i < expected_len; ++i) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (value[i] != u'-') return false;
        } else if (!is_hex_digit(value[i])) {
            return false;
        }
    }
    return true;
}

bool is_link_fast(const QString& value) {
    const int len = value.length();
    if (len < 34) return false;
    int i = 0;
    if (!is_ascii_digit(value[i])) return false;
    while (i < len && is_ascii_digit(value[i])) ++i;
    if (i >= len || value[i] != u':') return false;
    ++i;
    if (len - i != 32) return false;
    for (; i < len; ++i) {
        if (!is_hex_digit(value[i])) return false;
    }
    return true;
}

bool is_base64_fast(const QString& value, int start_index) {
    for (int i = start_index; i < value.length(); ++i) {
        if (!is_base64_digit(value[i])) return false;
    }
    return true;
}

QString tohex(int n) {
    return QString::number(n, 16).toUpper();
}

} // namespace

// Регулярные выражения (аналог boost::wregex)
static const QRegularExpression exp_number(QStringLiteral("^-?\\d+$"));
static const QRegularExpression exp_number_exp(QStringLiteral("^-?\\d+(\\.?\\d*)?((e|E)-?\\d+)?$"));
static const QRegularExpression exp_guid(
    QStringLiteral("^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"));
static const QRegularExpression exp_binary(QStringLiteral("^#base64:[0-9a-zA-Z\\+=\\r\\n\\/]*$"));
static const QRegularExpression exp_binary2(QStringLiteral("^[0-9a-zA-Z\\+=\\r\\n\\/]+$"));
static const QRegularExpression exp_link(QStringLiteral("^[0-9]+:[0-9a-fA-F]{32}$"));
static const QRegularExpression exp_binary_d(QStringLiteral("^#data:[0-9a-zA-Z\\+=\\r\\n\\/]*$"));

//---------------------------------------------------------------------------
tree::tree(const QString& _value, const node_type _type, tree* _parent)
{
    value       = _value;
    type        = _type;
    parent      = _parent;
    num_subnode = 0;
    index       = 0;

    if (parent) {
        parent->num_subnode++;
        prev = parent->last;
        if (prev) {
            prev->next = this;
            index = prev->index + 1;
        } else {
            parent->first = this;
        }
        parent->last = this;
    } else {
        prev = nullptr;
    }

    next  = nullptr;
    first = nullptr;
    last  = nullptr;
}

//---------------------------------------------------------------------------
tree::~tree()
{
    while (last) delete last;

    if (prev) prev->next = next;
    if (next) next->prev = prev;

    if (parent) {
        if (parent->first == this) parent->first = next;
        if (parent->last  == this) parent->last  = prev;
        parent->num_subnode--;
    }
}

//---------------------------------------------------------------------------
tree* tree::add_child(const QString& _value, const node_type _type)
{
    return new tree(_value, _type, this);
}

//---------------------------------------------------------------------------
tree* tree::add_child()
{
    return new tree(QString(), nd_empty, this);
}

//---------------------------------------------------------------------------
tree* tree::add_node()
{
    return new tree(QString(), nd_empty, this->parent);
}

//---------------------------------------------------------------------------
QString& tree::get_value() { return value; }

//---------------------------------------------------------------------------
node_type tree::get_type() { return type; }

//---------------------------------------------------------------------------
void tree::set_value(const QString& v, const node_type t)
{
    value = v;
    type  = t;
}

//---------------------------------------------------------------------------
int tree::get_num_subnode() { return num_subnode; }

//---------------------------------------------------------------------------
tree* tree::get_subnode(int _index)
{
    if (_index >= num_subnode) return nullptr;
    tree* t = first;
    while (_index) {
        t = t->next;
        --_index;
    }
    return t;
}

//---------------------------------------------------------------------------
tree* tree::get_subnode(const QString& node_name)
{
    tree* t = first;
    while (t) {
        if (t->value == node_name) return t;
        t = t->next;
    }
    return nullptr;
}

//---------------------------------------------------------------------------
tree* tree::get_next()   { return next; }
tree* tree::get_parent() { return parent; }
tree* tree::get_first()  { return first; }
tree* tree::get_last()   { return last; }

//---------------------------------------------------------------------------
tree& tree::operator[](int _index)
{
    if (!this) return *this;    // -V704
    tree* ret = first;
    while (_index) {
        if (ret) ret = ret->next;
        --_index;
    }
    return *ret;
}

//---------------------------------------------------------------------------
void tree::outtext(QString& text)
{
    node_type lt = nd_empty;

    if (num_subnode) {
        if (!text.isEmpty()) text += QStringLiteral("\r\n");
        text += u'{';
        tree* t = first;
        while (t) {
            t->outtext(text);
            lt = t->type;
            t = t->next;
            if (t) text += u',';
        }
        if (lt == nd_list) text += QStringLiteral("\r\n");
        text += u'}';
    } else {
        switch (type) {
        case nd_string:
            text += u'"';
            text += QString(value).replace(QStringLiteral("\""), QStringLiteral("\"\""));
            text += u'"';
            break;
        case nd_number:
        case nd_number_exp:
        case nd_guid:
        case nd_list:
        case nd_binary:
        case nd_binary2:
        case nd_link:
        case nd_binary_d:
            text += value;
            break;
        default:
            break;
        }
    }
}

//---------------------------------------------------------------------------
QString tree::path()
{
    QString p;
    tree* t;
    if (!this) return QStringLiteral(":??");   // -V704
    for (t = this; t->parent; t = t->parent) {
        p = QStringLiteral(":") + QString::number(t->index) + p;
    }
    return p;
}

//---------------------------------------------------------------------------
node_type classification_value(const QString& value)
{
    if (value.length() == 0) return nd_empty;
    if (is_number_fast(value)) return nd_number;
    if (is_number_exp_fast(value)) return nd_number_exp;
    if (is_guid_fast(value)) return nd_guid;
    if (value.length() >= 8 && value.mid(0, 8) == QStringLiteral("#base64:")
        && is_base64_fast(value, 8)) return nd_binary;
    if (is_link_fast(value)) return nd_link;
    if (is_base64_fast(value, 0)) return nd_binary2;
    if (value.length() >= 6 && value.mid(0, 6) == QStringLiteral("#data:")
        && is_base64_fast(value, 6)) return nd_binary_d;
    return nd_unknown;
}


tree* parse_1Cstream(QIODevice* str, const QString& path)
{
    if (!str || !str->isOpen()) {
        qWarning() << "parse_1Cstream: поток не открыт";
        return nullptr;
    }

    const QByteArray raw = str->readAll();
    if (raw.isEmpty()) {
        qWarning() << "parse_1Cstream: пустой поток";
        return nullptr;
    }


    qDebug() << "parse_1Cstream:" << path
             << "size" << raw.size()
             << "first bytes" << raw.left(8).toHex(' ');

    QString text;

    // ── 1. UTF-8 с BOM: EF BB BF
    if (raw.size() >= 3
        && static_cast<uchar>(raw[0]) == 0xEF
        && static_cast<uchar>(raw[1]) == 0xBB
        && static_cast<uchar>(raw[2]) == 0xBF)
    {
        text = QString::fromUtf8(raw.constData() + 3, raw.size() - 3);
    }
    // ── 2. UTF-16LE с BOM: FF FE
    else if (raw.size() >= 2
        && static_cast<uchar>(raw[0]) == 0xFF
        && static_cast<uchar>(raw[1]) == 0xFE)
    {
        const int units = (raw.size() - 2) / 2;
        text = QString::fromUtf16(
            reinterpret_cast<const char16_t*>(raw.constData() + 2),
            units);
    }
    // ── 3. UTF-16BE с BOM: FE FF
    else if (raw.size() >= 2
        && static_cast<uchar>(raw[0]) == 0xFE
        && static_cast<uchar>(raw[1]) == 0xFF)
    {
        QByteArray swapped = raw.mid(2);
        for (int i = 0; i + 1 < swapped.size(); i += 2)
            std::swap(swapped[i], swapped[i + 1]);
        text = QString::fromUtf16(
            reinterpret_cast<const char16_t*>(swapped.constData()),
            swapped.size() / 2);
    }
    // ── 4. Без BOM: эвристика по содержимому
    else {
        // Проверяем, похоже ли на UTF-16LE: много нулевых байтов на чётных позициях
        int zeroHigh = 0;
        const int checkLen = qMin(raw.size() & ~1, 200);
        for (int i = 1; i < checkLen; i += 2) {
            if (raw[i] == 0) ++zeroHigh;
        }

        if (zeroHigh > checkLen / 4) {
            // Похоже на UTF-16LE без BOM
            text = QString::fromUtf16(
                reinterpret_cast<const char16_t*>(raw.constData()),
                raw.size() / 2);
        } else {
            // Пробуем как UTF-8 с проверкой корректности

            // Qt 6: используем QStringDecoder
            QStringDecoder decoder(QStringDecoder::Utf8);
            text = decoder(raw);

            if (decoder.hasError()) {
                // UTF-8 сломан — последний шанс, UTF-16LE
                text = QString::fromUtf16(
                    reinterpret_cast<const char16_t*>(raw.constData()),
                    raw.size() / 2);
            }
        }
    }

    if (text.isEmpty()) {
        qWarning() << "parse_1Cstream: не удалось декодировать текст,"
                   << "путь" << path
                   << "размер" << raw.size();
        return nullptr;
    }

    // Срезаем BOM U+FEFF, если он всё-таки просочился в QString
    if (text.startsWith(QChar(0xFEFF)))
        text.remove(0, 1);

    return parse_1Ctext(text, path);
}

/*
//---------------------------------------------------------------------------
tree* parse_1Cstream(QIODevice* str, const QString& path)
{
    enum _state {
        s_value,                // ожидание начала значения
        s_delimitier,           // ожидание разделителя
        s_string,               // режим ввода строки
        s_quote_or_endstring,   // режим ожидания конца строки или двойной кавычки
        s_nonstring             // режим ввода значения не строки
    } state = s_value;

    QString curvalue;
    tree* ret;
    tree* t;
    int i;
    QChar sym;
    node_type nt;

    if (!str || !str->isOpen()) {
        qWarning() << "parse_1Cstream: поток не открыт";
        return nullptr;
    }

    // Читаем как UTF-16 (1С хранит текст в UTF-16LE)
    QTextStream reader(str);
    reader.setEncoding(QStringConverter::Utf16LE);

    ret = new tree(QString(), nd_list, nullptr);
    t = ret;

    for (i = 1; !reader.atEnd(); ++i) {
        reader >> sym;
        if (reader.atEnd()) break;

        switch (state) {
        case s_value:
            switch (sym.unicode()) {
            case u' ': case u'\t': case u'\r': case u'\n':
                break;
            case u'"':
                curvalue.clear();
                state = s_string;
                break;
            case u'{':
                t = new tree(QString(), nd_list, t);
                break;
            case u'}':
                if (t->get_first())
                    t->add_child(QString(), nd_empty);
                t = t->get_parent();
                if (!t) {
                    qWarning() << "Ошибка формата потока. Лишняя закрывающая скобка }."
                               << "Позиция" << i << "Путь" << path;
                    delete ret;
                    return nullptr;
                }
                state = s_delimitier;
                break;
            case u',':
                t->add_child(QString(), nd_empty);
                break;
            default:
                curvalue.clear();
                curvalue.append(sym);
                state = s_nonstring;
                break;
            }
            break;

        case s_delimitier:
            switch (sym.unicode()) {
            case u' ': case u'\t': case u'\r': case u'\n':
                break;
            case u',':
                state = s_value;
                break;
            case u'}':
                t = t->get_parent();
                if (!t) {
                    qWarning() << "Ошибка формата потока. Лишняя закрывающая скобка }."
                               << "Позиция" << i << "Путь" << path;
                    delete ret;
                    return nullptr;
                }
                break;
            default:
                qWarning() << "Ошибка формата потока. Ошибочный символ в режиме ожидания разделителя."
                           << "Символ" << sym << "Код символа" << tohex(sym.unicode())
                           << "Путь" << path;
                delete ret;
                return nullptr;
            }
            break;

        case s_string:
            if (sym == u'"') {
                state = s_quote_or_endstring;
            } else {
                curvalue.append(sym);
            }
            break;

        case s_quote_or_endstring:
            if (sym == u'"') {
                curvalue.append(sym);
                state = s_string;
            } else {
                t->add_child(curvalue, nd_string);
                switch (sym.unicode()) {
                case u' ': case u'\t': case u'\r': case u'\n':
                    state = s_delimitier;
                    break;
                case u',':
                    state = s_value;
                    break;
                case u'}':
                    t = t->get_parent();
                    if (!t) {
                        qWarning() << "Ошибка формата потока. Лишняя закрывающая скобка }."
                                   << "Позиция" << i << "Путь" << path;
                        delete ret;
                        return nullptr;
                    }
                    state = s_delimitier;
                    break;
                default:
                    qWarning() << "Ошибка формата потока. Ошибочный символ в режиме ожидания разделителя."
                               << "Символ" << sym << "Код символа" << tohex(sym.unicode())
                               << "Путь" << path;
                    delete ret;
                    return nullptr;
                }
            }
            break;

        case s_nonstring:
            switch (sym.unicode()) {
            case u',':
                curvalue = curvalue; // уже накоплено
                nt = classification_value(curvalue);
                if (nt == nd_unknown)
                    qWarning() << "Ошибка формата потока. Неизвестный тип значения."
                               << "Значение" << curvalue << "Путь" << path;
                t->add_child(curvalue, nt);
                state = s_value;
                break;
            case u'}':
                nt = classification_value(curvalue);
                if (nt == nd_unknown)
                    qWarning() << "Ошибка формата потока. Неизвестный тип значения."
                               << "Значение" << curvalue << "Путь" << path;
                t->add_child(curvalue, nt);
                t = t->get_parent();
                if (!t) {
                    qWarning() << "Ошибка формата потока. Лишняя закрывающая скобка }."
                               << "Позиция" << i << "Путь" << path;
                    delete ret;
                    return nullptr;
                }
                state = s_delimitier;
                break;
            default:
                curvalue.append(sym);
                break;
            }
            break;

        default:
            qWarning() << "Ошибка формата потока. Неизвестный режим разбора."
                       << "Режим разбора" << tohex(state) << "Путь" << path;
            delete ret;
            return nullptr;
        }
    }

    if (state == s_nonstring) {
        nt = classification_value(curvalue);
        if (nt == nd_unknown)
            qWarning() << "Ошибка формата потока. Неизвестный тип значения."
                       << "Значение" << curvalue << "Путь" << path;
        t->add_child(curvalue, nt);
    } else if (state == s_quote_or_endstring) {
        t->add_child(curvalue, nd_string);
    } else if (state != s_delimitier) {
        qWarning() << "Ошибка формата потока. Незавершенное значение"
                   << "Режим разбора" << tohex(state) << "Путь" << path;
        delete ret;
        return nullptr;
    }

    if (t != ret) {
        qWarning() << "Ошибка формата потока. Не хватает закрывающих скобок } в конце текста разбора."
                   << "Путь" << path;
        delete ret;
        return nullptr;
    }

    return ret;
}

*/

//---------------------------------------------------------------------------
tree* parse_1Ctext(const QString& text, const QString& path)
{
    enum _state {
        s_value,
        s_delimitier,
        s_string,
        s_quote_or_endstring,
        s_nonstring
    } state = s_value;

    QString curvalue;
    tree* ret;
    tree* t;
    int len = text.length();
    int i;
    QChar sym;
    node_type nt;

    ret = new tree(QString(), nd_list, nullptr);
    t = ret;

    for (i = 0; i < len; ++i) {
        sym = text[i];
        if (sym.unicode() == 0) break;

        switch (state) {
        case s_value:
            switch (sym.unicode()) {
            case u' ': case u'\t': case u'\r': case u'\n':
                break;
            case u'"':
                curvalue.clear();
                state = s_string;
                break;
            case u'{':
                t = new tree(QString(), nd_list, t);
                break;
            case u'}':
                if (t->get_first())
                    t->add_child(QString(), nd_empty);
                t = t->get_parent();
                if (!t) {
                    qWarning() << "Ошибка формата потока. Лишняя закрывающая скобка }."
                               << "Позиция" << i << "Путь" << path;
                    delete ret;
                    return nullptr;
                }
                state = s_delimitier;
                break;
            case u',':
                t->add_child(QString(), nd_empty);
                break;
            default:
                curvalue.clear();
                curvalue.append(sym);
                state = s_nonstring;
                break;
            }
            break;

        case s_delimitier:
            switch (sym.unicode()) {
            case u' ': case u'\t': case u'\r': case u'\n':
                break;
            case u',':
                state = s_value;
                break;
            case u'}':
                t = t->get_parent();
                if (!t) {
                    qWarning() << "Ошибка формата потока. Лишняя закрывающая скобка }."
                               << "Позиция" << i << "Путь" << path;
                    delete ret;
                    return nullptr;
                }
                break;
            default:
                qWarning() << "Ошибка формата потока. Ошибочный символ в режиме ожидания разделителя."
                           << "Символ" << sym << "Код символа" << tohex(sym.unicode())
                           << "Путь" << path;
                delete ret;
                return nullptr;
            }
            break;

        case s_string:
            if (sym == u'"') {
                state = s_quote_or_endstring;
            } else {
                curvalue.append(sym);
            }
            break;

        case s_quote_or_endstring:
            if (sym == u'"') {
                curvalue.append(sym);
                state = s_string;
            } else {
                t->add_child(curvalue, nd_string);
                switch (sym.unicode()) {
                case u' ': case u'\t': case u'\r': case u'\n':
                    state = s_delimitier;
                    break;
                case u',':
                    state = s_value;
                    break;
                case u'}':
                    t = t->get_parent();
                    if (!t) {
                        qWarning() << "Ошибка формата потока. Лишняя закрывающая скобка }."
                                   << "Позиция" << i << "Путь" << path;
                        delete ret;
                        return nullptr;
                    }
                    state = s_delimitier;
                    break;
                default:
                    qWarning() << "Ошибка формата потока. Ошибочный символ в режиме ожидания разделителя."
                               << "Символ" << sym << "Код символа" << tohex(sym.unicode())
                               << "Путь" << path;
                    delete ret;
                    return nullptr;
                }
            }
            break;

        case s_nonstring:
            switch (sym.unicode()) {
            case u',':
                nt = classification_value(curvalue);
                if (nt == nd_unknown)
                    qWarning() << "Ошибка формата потока. Неизвестный тип значения."
                               << "Значение" << curvalue << "Путь" << path;
                t->add_child(curvalue, nt);
                state = s_value;
                break;
            case u'}':
                nt = classification_value(curvalue);
                if (nt == nd_unknown)
                    qWarning() << "Ошибка формата потока. Неизвестный тип значения."
                               << "Значение" << curvalue << "Путь" << path;
                t->add_child(curvalue, nt);
                t = t->get_parent();
                if (!t) {
                    qWarning() << "Ошибка формата потока. Лишняя закрывающая скобка }."
                               << "Позиция" << i << "Путь" << path;
                    delete ret;
                    return nullptr;
                }
                state = s_delimitier;
                break;
            default:
                curvalue.append(sym);
                break;
            }
            break;

        default:
            qWarning() << "Ошибка формата потока. Неизвестный режим разбора."
                       << "Режим разбора" << tohex(state) << "Путь" << path;
            delete ret;
            return nullptr;
        }
    }

    if (state == s_nonstring) {
        nt = classification_value(curvalue);
        if (nt == nd_unknown)
            qWarning() << "Ошибка формата потока. Неизвестный тип значения."
                       << "Значение" << curvalue << "Путь" << path;
        t->add_child(curvalue, nt);
    } else if (state == s_quote_or_endstring) {
        t->add_child(curvalue, nd_string);
    } else if (state != s_delimitier) {
        qWarning() << "Ошибка формата потока. Незавершенное значение"
                   << "Режим разбора" << tohex(state) << "Путь" << path;
        delete ret;
        return nullptr;
    }

    if (t != ret) {
        qWarning() << "Ошибка формата потока. Не хватает закрывающих скобок } в конце текста разбора."
                   << "Путь" << path;
        delete ret;
        return nullptr;
    }

    return ret;
}

//---------------------------------------------------------------------------
bool test_parse_1Ctext(QIODevice* str, const QString& path)
{
    // Упрощённая проверка формата — по аналогии с оригиналом.
    // Возвращает true, если поток успешно парсится.
    tree* t = parse_1Cstream(str, path);
    if (t) {
        delete t;
        return true;
    }
    return false;
}

//---------------------------------------------------------------------------
QString outtext(tree* t)
{
    if (!t) return {};
    QString text;
    t->outtext(text);
    return text;
}

//---------------------------------------------------------------------------
tree* find_node_by_guid(tree* root, const QString& target_guid)
{
    if (!root) return nullptr;
    if (root->get_value().compare(target_guid, Qt::CaseInsensitive) == 0)
        return root;

    for (int i = 0; i < root->get_num_subnode(); ++i) {
        if (tree* found = find_node_by_guid(root->get_subnode(i), target_guid))
            return found;
    }
    return nullptr;
}

//---------------------------------------------------------------------------
tree* find_metadata_node_by_guid(tree* root, const QString& target_guid)
{
    if (!root) return nullptr;

    if (root->get_type() == nd_list
        && root->get_value().compare(target_guid, Qt::CaseInsensitive) == 0) {
        return root;
    }

    for (int i = 0; i < root->get_num_subnode(); ++i) {
        if (tree* found = find_metadata_node_by_guid(root->get_subnode(i), target_guid))
            return found;
    }
    return nullptr;
}