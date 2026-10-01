#ifndef METADATAMAPPER_H
#define METADATAMAPPER_H

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "MetadataMap.h"
#include "src/core/V8Container.h"
#include <QString>

namespace v8unpack {

/**
 * Сопоставляет записи контейнера 1С с метаданными.
 * На входе — распакованные данные (GUID + бинарные блоки),
 * на выходе — структура, пригодная для раскладки по каталогам.
 */
class MetadataMapper {
public:
    explicit MetadataMapper(const MetadataMap &map);

    struct MappedEntry {
        QString typeName;      // "Catalogs", "Documents", ...
        QString objectName;    // "Номенклатура", ...
        QString objectGuid;    // исходный GUID
        QString relativePath;  // путь относительно корня вывода
        QByteArray data;       // данные записи
    };

    /// Преобразовать записи контейнера в список MappedEntry.
    /// Для записей, не найденных в карте, используется fallback:
    /// typeName = "Unknown", objectName = GUID.
    QVector<MappedEntry> map(const QVector<V8Container::Entry> &entries) const;

private:
    const MetadataMap &m_map;
};

} // namespace v8


#endif // METADATAMAPPER_H
