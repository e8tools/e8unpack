// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "UnpackManifest.h"

#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QDateTime>
#include <QDebug>

namespace v8unpack {

QString UnpackManifest::pathFor(const QString& outputDir)
{
    return QDir(outputDir).filePath(QStringLiteral("%1/%2").arg(QString::fromLatin1(kDirName), QString::fromLatin1(kFileName)));
}

bool UnpackManifest::exists(const QString& outputDir)
{
    return QFile::exists(pathFor(outputDir));
}

bool UnpackManifest::load(const QString& outputDir, QString* errorOut)
{
    const QString path = pathFor(outputDir);
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (errorOut) *errorOut = QStringLiteral("Не удалось открыть %1").arg(path);
        return false;
    }

    QJsonParseError perr{};
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorOut) *errorOut = QStringLiteral("Некорректный JSON: %1").arg(perr.errorString());
        return false;
    }

    const QJsonObject root = doc.object();
    const int version = root.value(QStringLiteral("version")).toInt(0);
    if (version != kCurrentVersion) {
        if (errorOut) *errorOut = QStringLiteral("Неподдерживаемая версия manifest: %1").arg(version);
        return false;
    }

    m_sourceContainer = root.value(QStringLiteral("sourceContainer")).toString();
    m_createdAt       = root.value(QStringLiteral("createdAt")).toString();

    m_entries.clear();
    for (const QJsonValue& v : root.value(QStringLiteral("entries")).toArray()) {
        const QJsonObject o = v.toObject();
        ManifestEntry e;
        e.originalName  = o.value(QStringLiteral("original")).toString();
        e.diskPath      = o.value(QStringLiteral("path")).toString();
        e.compressed    = o.value(QStringLiteral("compressed")).toBool(true);
        e.rawSize       = static_cast<qint64>(o.value(QStringLiteral("rawSize")).toDouble(0));
        e.originalIndex = o.value(QStringLiteral("originalIndex")).toInt(-1);
        if (!e.originalName.isEmpty() && !e.diskPath.isEmpty())
            m_entries.append(e);
    }
    return true;
}

bool UnpackManifest::save(const QString& outputDir, QString* errorOut) const
{
    QDir dir(outputDir);
    if (!dir.mkpath(QString::fromLatin1(kDirName))) {
        if (errorOut) *errorOut = QStringLiteral("Не удалось создать %1").arg(kDirName);
        return false;
    }

    QJsonArray arr;
    for (const ManifestEntry& e : m_entries) {
        QJsonObject o;
        o[QStringLiteral("original")]      = e.originalName;
        o[QStringLiteral("path")]          = e.diskPath;
        o[QStringLiteral("compressed")]    = e.compressed;
        o[QStringLiteral("rawSize")]       = static_cast<double>(e.rawSize);
        o[QStringLiteral("originalIndex")] = e.originalIndex;
        arr.append(o);
    }

    QJsonObject root;
    root[QStringLiteral("version")]         = kCurrentVersion;
    root[QStringLiteral("sourceContainer")] = m_sourceContainer;
    root[QStringLiteral("createdAt")]       = m_createdAt.isEmpty()
        ? QDateTime::currentDateTimeUtc().toString(Qt::ISODate)
        : m_createdAt;
    root[QStringLiteral("entries")]         = arr;

    QSaveFile f(pathFor(outputDir));
    if (!f.open(QIODevice::WriteOnly)) {
        if (errorOut) *errorOut = QStringLiteral("Не удалось записать %1").arg(f.fileName());
        return false;
    }
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!f.commit()) {
        if (errorOut) *errorOut = QStringLiteral("commit() не удался для %1").arg(f.fileName());
        return false;
    }
    return true;
}

const ManifestEntry* UnpackManifest::findByDiskPath(const QString& diskPath) const
{
    const QString needle = QDir::fromNativeSeparators(diskPath);
    for (const auto& e : m_entries)
        if (QDir::fromNativeSeparators(e.diskPath) == needle) return &e;
    return nullptr;
}

const ManifestEntry* UnpackManifest::findByOriginalName(const QString& name) const
{
    for (const auto& e : m_entries)
        if (e.originalName == name) return &e;
    return nullptr;
}

} // namespace v8