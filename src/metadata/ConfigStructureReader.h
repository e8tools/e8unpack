#ifndef CONFIGSTRUCTUREREADER_H
#define CONFIGSTRUCTUREREADER_H

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <QString>
#include <QVector>
#include <QHash>
#include <boost/filesystem.hpp>

#include "src/parser/Parse_tree.h"

namespace fs = boost::filesystem;

namespace v8unpack {

/**
 * Группа объектов одного типа метаданных.
 * Пример: {cf4abea6-...,4,guid1,guid2,guid3,guid4}
 */
struct ObjectGroup {
    QString typeGuid;                  // GUID типа (например, cf4abea6-...)
    QString typeName;                  // "Справочники", "Документы", ...
    QVector<QString> objectGuids;      // GUID-ы конкретных объектов
};

// ── Сначала объявляем SectionInfo вне класса ────────────────────
struct SectionInfo {
    QString sectionGuid;              // GUID секции (форм, реквизитов, ...)
    QString sectionName;              // "Формы", "Реквизиты", ...
    QVector<QString> elementGuids;    // GUID-ы элементов секции
};

/**
 * Читает иерархию конфигурации 1С из распакованного каталога.
 *
 * Алгоритм:
 *   1. Открываем файл `root` — он содержит GUID файла со структурой.
 *   2. Открываем этот файл — он содержит список групп
 *      {type_guid, count, obj_guid1, obj_guid2, ...}.
 *   3. Для каждого obj_guid открываем файл с этим именем и находим
 *      в дереве узел вида {1,0,<obj_guid>} — следующий за ним
 *      sibling-узел содержит имя объекта.
 */
class ConfigStructureReader {
public:
    /// @param configDir — каталог, где лежат root, версии и файлы метаданных
    ///                    (обычно <outputDir>/Конфигурация)
    explicit ConfigStructureReader(const fs::path& configDir);

    /// Шаг 1: прочитать root и получить GUID файла структуры конфигурации.
    bool loadRoot();

    /// Шаг 2: прочитать файл структуры и получить список групп объектов.
    bool loadObjectGroups();

    /// Шаг 3: для каждого объекта найти его имя.
    /// Возвращает карту objectGuid (lowercase) → имя.
    QHash<QString, QString> resolveAllNames();

    /// Извлечь имя одного объекта по его GUID.
    /// Открывает файл <configDir>/<guid>, парсит и находит узел {1,0,<guid>}.
    QString resolveName(const QString& objectGuid) const;

    /// Recursively resolve objects listed inside subsystem objects.
    QVector<ObjectGroup> resolveNestedSubsystems() const;
    QHash<QString, QString> resolveSubsystemParents() const;

    // ── Доступ к данным ────────────────────────────────────────────
    const QString& configGuid() const { return m_configGuid; }
    const QVector<ObjectGroup>& groups() const { return m_groups; }

    /// Извлечь все секции из файла объекта (форм, реквизитов, табличных частей).
    /// @param objGuid — GUID объекта конфигурации
    QVector<SectionInfo> resolveSections(const QString& objGuid) const;

    /// Владелец и имя элемента, найденные по узлу {1,0,<GUID>},"<Имя>".
    struct ElementOwner {
        QString ownerGuid;   // GUID объекта-владельца (пусто — элемент сам себе объект)
        QString name;        // имя элемента
    };

    /// Найти имена (и владельцев) элементов по их GUID.
    /// Сначала смотрим собственный файл <configDir>/<GUID>, затем —
    /// тексты объектов конфигурации (там лежат, например, обработчики команд).
    QHash<QString, ElementOwner> findElementOwners(const QStringList& elementGuids) const;

private:
    fs::path m_configDir;
    QString m_configGuid;              // GUID файла структуры конфигурации
    QVector<ObjectGroup> m_groups;
};

} // namespace v8

#endif // CONFIGSTRUCTUREREADER_H