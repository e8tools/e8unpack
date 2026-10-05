#ifndef STRUCTUREDUNPACKER_H
#define STRUCTUREDUNPACKER_H

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "src/metadata/MetadataMap.h"
#include <QString>
#include <QVector>
#include "src/metadata/ConfigStructureReader.h"
#include <QHash>
#include "src/output/UnpackManifest.h"


namespace v8unpack {

/**
 * Распаковывает контейнер 1С сразу в структуру каталогов по метаданным.
 *
 * Схема работы:
 *   1. v8unpack::Parse распаковывает во временный подкаталог .tmp_unpack/
 *   2. Обходим дерево и вычисляем целевые пути:
 *      - <GUID типа>                 → <Имя типа>/       (Справочники, Документы, ...)
 *      - <GUID типа>/<GUID объекта>  → <Имя типа>/<Имя объекта>/
 *      - всё остальное               → Конфигурация/
 *   3. Переносим записи в целевую структуру
 *   4. Удаляем временный каталог
 */
class StructuredUnpacker {
public:
    StructuredUnpacker(const QString &inputFile,
                       const QString &outputDir,
                       const MetadataMap *map);

    /// Выполнить распаковку с раскладкой.
    /// @return true при успехе
    bool run();

    /// Имя служебного каталога для root/version/versions и т.п.
    static QString serviceFolderName() { return QStringLiteral("Конфигурация"); }

    int renamedCount() const { return m_moved; }

    struct ChildInfo {
        QString parentGuid;    // GUID родительского объекта конфигурации
        QString parentName;    // имя родителя ("Справочник1_Тестовый")
        QString parentType;    // тип родителя ("Справочники")
        QString sectionName;   // имя секции ("Формы", "Реквизиты", ...)
    };

    int manifestEntryCount() const { return m_manifest.entries().size(); }

private:
    bool unpackToTemp();
    bool buildAndApplyPlan();
    void cleanupTemp();

    /// Определить относительный целевой путь для записи.
    /// Возвращает пустую строку, если запись надо пропустить.
    QString targetRelativePath(const fs::path &rel,
                               const QString &guid,
                               const QString &parentGuid) const;

    static bool isServiceName(const QString &name);

    QString m_inputFile;
    QString m_outputDir;
    QString m_tempDir;
    const MetadataMap *m_map;

    int m_moved = 0;

    //QHash<QString, QString> m_nameCache;  // objectGuid → name
    /// Загрузить имена всех объектов через ConfigStructureReader.
    bool loadObjectNames();

    /// Определить имя объекта: сначала из m_nameCache,
    /// потом из MetadataMap, потом GUID.
    QString objectName(const QString& guid) const;
    /// Найти каталог, содержащий файл root, рекурсивно внутри m_tempDir.
    /// Возвращает путь или пустую строку.
    QString findRootDir() const;

    QHash<QString, QString> m_nameCache;    // object guid (lower) → имя
    QHash<QString, QString> m_typeOf;       // object guid (lower) → имя типа
    QString m_configGuid;                   // GUID файла структуры конфигурации

    /// Вернуть «базовый» GUID из имени файла:
    /// "e51aa14c-..." → "e51aa14c-..."
    /// "e51aa14c-....0" → "e51aa14c-..."
    static QString baseGuidFromFileName(const QString& fileName);

    QHash<QString, ChildInfo> m_childIndex;   // child guid(lower) → инфо о родителе

    /// Построить карту «дочерний элемент → родительский объект»
    /// на основе разбора секций каждого объекта.
    bool indexChildElements();

    QVector<ObjectGroup> m_groups_cache;

    UnpackManifest m_manifest;   
    int m_nextOriginalIndex = 0; // счётчик порядка
};

} // namespace v8

#endif // STRUCTUREDUNPACKER_H