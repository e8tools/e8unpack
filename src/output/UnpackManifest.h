#ifndef UNPACKMANIFEST_H
#define UNPACKMANIFEST_H

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <QString>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

namespace v8unpack {

// Одна запись manifest'а — соответствие "оригинальное имя в контейнере"
// ↔ "человекочитаемый путь на диске" + метаданные для обратной сборки.
struct ManifestEntry {
    QString originalName;   // как было в .cf: "f7dae919-...-b35f.0", "root", "version"
    QString diskPath;       // относительный путь: "Конфигурация/МодульОбычногоПриложения"
    bool    compressed = true;  // был ли элемент сжат при распаковке
    qint64  rawSize    = 0;     // размер данных до сжатия (для валидации)
    int     originalIndex = -1; // порядок записи в контейнере (1С критичен к порядку)
    QString moduleKind;     // recognized configuration module type for flat BSL path
    QByteArray moduleInfo;  // original compound element info bytes
    bool moduleTextHadBom = true;
};

class UnpackManifest
{
public:
    static constexpr int kCurrentVersion = 1;
    static constexpr const char* kDirName  = ".e8unpack";
    static constexpr const char* kFileName = "manifest.json";

    // Полный путь до manifest.json внутри output_dir
    static QString pathFor(const QString& outputDir);

    // Есть ли manifest в каталоге
    static bool exists(const QString& outputDir);

    // Загрузка/сохранение
    bool load(const QString& outputDir, QString* errorOut = nullptr);
    bool save(const QString& outputDir, QString* errorOut = nullptr) const;

    // Список записей
    const QVector<ManifestEntry>& entries() const { return m_entries; }
    QVector<ManifestEntry>&       entries()       { return m_entries; }

    // Метаданные контейнера
    QString sourceContainer() const { return m_sourceContainer; }
    void setSourceContainer(const QString& name) { m_sourceContainer = name; }

    // Найти запись по diskPath (для packer'а, если он обходит файлы)
    const ManifestEntry* findByDiskPath(const QString& diskPath) const;

    // Найти запись по originalName (для отладки)
    const ManifestEntry* findByOriginalName(const QString& name) const;

private:
    QVector<ManifestEntry> m_entries;
    QString m_sourceContainer;
    QString m_createdAt;
};

} // namespace v8

#endif // UNPACKMANIFEST_H
