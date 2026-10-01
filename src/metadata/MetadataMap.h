#ifndef METADATAMAP_H
#define METADATAMAP_H

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <QString>
#include <QHash>
#include <QJsonObject>

#include <optional>

namespace v8unpack {

/**
 * Карта соответствия: тип метаданных → GUID объекта → человекочитаемое имя.
 * Заполняется из внешнего JSON-файла, сгенерированного v8_reader
 * (или любым другим парсером метаданных 1С).
 *
 * Формат JSON:
 * {
 *   "Catalogs": {
 *     "cf4abea6-37b2-11d4-940f-008048da11f9": {
 *       "type_guid": "...",
 *       "objects": {
 *         "154c4235-35f5-42c3-a0d5-07b9ea861f14": {
 *           "name": "Номенклатура",
 *           "synonym": "Номенклатура"
 *         }
 *       }
 *     }
 *   },
 *   ...
 * }
 */
class MetadataMap {
public:
    bool loadFromJson(const QString &filePath);
    bool isEmpty() const;

    /// Получить человекочитаемое имя объекта по GUID.
    /// @param typeName — тип метаданных (например, "Catalogs")
    /// @param objectGuid — GUID объекта
    /// @return имя или пустую строку, если не найдено
    QString objectName(const QString &typeName, const QString &objectGuid) const;

    /// Получить список всех типов метаданных, известных карте.
    QStringList typeNames() const;

    // в class MetadataMap, публичная секция
    struct ObjectInfo {
        QString typeName;    // "Catalogs"
        QString objectName;  // "Номенклатура"
    };

    /// Найти объект по GUID во всех известных типах.
    /// Возвращает пустой optional, если GUID не найден.
    std::optional<ObjectInfo> lookup(const QString &objectGuid) const;

private:
    QHash<QString, QHash<QString, QString>> m_index; // type → guid → name
};

} // namespace v8

#endif // METADATAMAP_H
