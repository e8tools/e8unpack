#ifndef ONECTEXT_H
#define ONECTEXT_H

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <QString>

namespace v8unpack {
namespace OneCText {

/**
 * Разбор текстовой структуры 1С ({элемент, элемент, ...}) — ровно настолько,
 * насколько нужно, чтобы найти один строковый элемент верхнего уровня.
 *
 * Нужно для описаний форм: модуль формы лежит прямо в описании, третьим
 * элементом верхнего уровня ({4, {свойства}, "модуль", ...}). Чтобы держать
 * код отдельным файлом и при этом собирать контейнер байт-в-байт как исходный,
 * нужно уметь находить этот элемент и заменять его на месте, не трогая
 * остальной текст.
 */

/// Найти строковый элемент верхнего уровня с индексом @p index.
/// @param begin,end  полуинтервал [begin, end) в @p text, включая кавычки
/// @param value      значение строки (внутренние кавычки развёрнуты)
/// @return false, если элемент не найден или не является строкой
bool topLevelStringSpan(const QString &text, int index,
                        int *begin, int *end, QString *value);

/// Собрать элемент структуры 1С из значения: кавычки, внутренние кавычки
/// удваиваются.
QString toElement(const QString &value);

/// Ведущий BOM (U+FEFF) — его нужно снимать до разбора и возвращать при записи.
bool hasBom(const QString &text);
QString stripBom(const QString &text);

} // namespace OneCText
} // namespace v8unpack

#endif // ONECTEXT_H
