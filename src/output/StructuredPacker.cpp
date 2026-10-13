// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
#include "StructuredPacker.h"
#include "src/core/MetadataTypes.h"
#include "src/metadata/SectionTypes.h"
#include "src/core/OneCText.h"
#include "V8File.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QRegularExpression>
#include <QSet>
#include <system_error>
#include <algorithm>
#include <boost/filesystem/fstream.hpp>

namespace v8unpack {
namespace fs = boost::filesystem;

StructuredPacker::StructuredPacker(const QString& inputDir,
                                   const QString& outputFile,
                                   bool noDeflate)
    : m_inputDir(inputDir)
    , m_outputFile(outputFile)
    , m_tempDir(QDir(inputDir).filePath(".tmp_pack"))
    , m_noDeflate(noDeflate)
{}

static const QSet<QString>& knownFolders()
{
    static QSet<QString> s;
    if (!s.isEmpty()) return s;

    for (auto it = metadataTypes().begin(); it != metadataTypes().end(); ++it) {
        const QString& path = it.value();
        const int slash = path.lastIndexOf(QLatin1Char('/'));
        s.insert(slash >= 0 ? path.mid(slash + 1) : path);
        if (slash > 0) s.insert(path.left(slash));
    }
    for (auto it = sectionTypes().begin(); it != sectionTypes().end(); ++it)
        s.insert(it.value());

    s.insert(QStringLiteral("Конфигурация"));
    return s;
}

static bool isGuidLike(const QString& name)
{
    static const QRegularExpression re(
        R"(^[0-9a-fA-F]{8}-?[0-9a-fA-F]{4}-?[0-9a-fA-F]{4}-?[0-9a-fA-F]{4}-?[0-9a-fA-F]{12}(\.\d+)?$)");
    
    return re.match(name).hasMatch();
}

// Кладём в .tmp_pack жёсткую ссылку вместо копии байтов: сборщик контейнера
// (.tmp_pack -> BuildCfFile) только читает эти файлы, а исходное дерево
// остаётся нетронутым. На больших конфигурациях это убирает перекачку
// нескольких гигабайт на каждой сборке; если ссылку создать нельзя
// (другая файловая система, отсутствие поддержки) — обычное копирование.
static void linkOrCopy(const fs::path& src, const fs::path& dst,
                       boost::system::error_code& ec)
{
    ec.clear();

    if (fs::is_directory(src, ec)) {
        ec.clear();
        fs::create_directories(dst, ec);
        if (ec)
            return;
        fs::directory_iterator it(src, ec), end;
        for (; !ec && it != end; it.increment(ec)) {
            linkOrCopy(it->path(), dst / it->path().filename(), ec);
        }
        return;
    }

    ec.clear();
    fs::create_hard_link(src, dst, ec);
    if (ec) {
        ec.clear();
        fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
    }
}

bool StructuredPacker::flattenToTemp()
{
    boost::system::error_code ec;
    fs::remove_all(m_tempDir.toStdWString(), ec);          // ✅
    fs::create_directories(m_tempDir.toStdWString(), ec);  // ✅

    const fs::path srcRoot(m_inputDir.toStdWString());     // ✅
    const fs::path tmpRoot(m_tempDir.toStdWString());      // ✅

    int copied = 0;

    fs::recursive_directory_iterator it(srcRoot, fs::directory_options::skip_permission_denied, ec);
    fs::recursive_directory_iterator end;

    for (; it != end; it.increment(ec)) {
        if (ec) break;

        if (!fs::is_regular_file(it->path(), ec))
            continue;

        const fs::path& src = it->path();

        auto rel = fs::relative(src, srcRoot, ec);
        
        if (ec) 
            continue;
        
        const QString relStr = QString::fromStdWString(rel.generic_wstring());
        
        if (relStr.startsWith(QLatin1String(".tmp_pack")))
            continue;

        const QString fileName = QString::fromStdWString(src.filename().wstring());

        const bool isService = (fileName == QStringLiteral("root")
                                || fileName == QStringLiteral("version")
                                || fileName == QStringLiteral("versions"));

        if (!isService && !isGuidLike(fileName))
            continue;

        fs::path dst = tmpRoot / fileName.toStdWString();  // ✅

        if (fs::exists(dst, ec)) {
            int n = 1;
            fs::path alt;
            do {
                alt = dst;
                alt += (L"." + std::to_wstring(n++));      // ✅
            } while (fs::exists(alt, ec));
            dst = alt;
        }

        linkOrCopy(src, dst, ec);
        if (!ec) {
            ++copied;
            if (isService) {
                qDebug() << "StructuredPacker: служебный файл найден:"
                         << QString::fromStdWString(rel.generic_wstring())
                         << "->" << fileName;
            }
        } else {
            qWarning() << "Не удалось скопировать"
                       << QString::fromStdWString(src.wstring())
                       << "->" << QString::fromStdWString(dst.wstring())
                       << ":" << QString::fromStdString(ec.message());
        }
    }

    qDebug() << "StructuredPacker: скопировано файлов:" << copied;

    {
        QStringList names;
        for (const auto& e : fs::directory_iterator(tmpRoot, ec))
            names << QString::fromStdWString(             // ✅
                e.path().filename().wstring());
        names.sort();
        qDebug() << "StructuredPacker: содержимое .tmp_pack:"
                 << names.join(QStringLiteral(", "));
    }

    const bool hasRoot = fs::exists(tmpRoot / L"root", ec);      // ✅
    if (!hasRoot) {
        qWarning() << "StructuredPacker: в .tmp_pack нет файла root.";
        return false;
    }

    const bool hasVersion = fs::exists(tmpRoot / L"version", ec); // ✅
    if (!hasVersion) {
        qWarning() << "StructuredPacker: в .tmp_pack нет файла version.";
    }

    return true;
}

void StructuredPacker::cleanupTemp()
{
    boost::system::error_code ec;
    fs::remove_all(m_tempDir.toStdWString(), ec);   // ✅
}

// ────────────────────────────────────────────────────────────────
//  Точка входа. Выбирает стратегию: manifest или fallback.
// ────────────────────────────────────────────────────────────────
bool StructuredPacker::run()
{
    // Пытаемся загрузить manifest — если он есть, собираем строго по нему.
    m_manifestLoaded = loadManifest();

    if (m_manifestLoaded) {
        qInfo() << "StructuredPacker: найден manifest ("
                << m_manifest.entries().size() << "записей) — сборка по нему.";
        if (!packFromManifest()) {
            cleanupTemp();
            return false;
        }
    } else {
        qWarning() << "StructuredPacker: manifest не найден — "
                      "сборка пойдёт по старой схеме. "
                      "Человекочитаемые имена файлов будут пропущены.";
        if (!packFromDirectoryFallback()) {
            // packFromDirectoryFallback уже делает cleanupTemp при ошибке
            return false;
        }
    }

    return true;
}

bool StructuredPacker::loadManifest()
{
    QString err;
    if (!m_manifest.load(m_inputDir, &err)) {
        qDebug() << "StructuredPacker: manifest не загружен:" << err;
        return false;
    }
    return true;
}

bool StructuredPacker::packFromManifest()
{
    // 1. Готовим чистый временный каталог — тот же, что использует fallback
    boost::system::error_code ec;
    fs::remove_all(m_tempDir.toStdWString(), ec);
    QDir().mkpath(m_tempDir);
    if (!QFileInfo::exists(m_tempDir)) {
        qWarning() << "StructuredPacker: не удалось создать" << m_tempDir;
        return false;
    }

    // 2. Сортируем записи по originalIndex — порядок важен для 1С
    QVector<ManifestEntry> ordered = m_manifest.entries();
    std::sort(ordered.begin(), ordered.end(),
              [](const ManifestEntry& a, const ManifestEntry& b) {
                  return a.originalIndex < b.originalIndex;
              });

    const fs::path inputRoot(m_inputDir.toStdWString());
    const fs::path tmpRoot(m_tempDir.toStdWString());

    int copied = 0;
    for (const ManifestEntry& e : ordered) {
        const QString srcAbs = QDir(m_inputDir).filePath(e.diskPath);
        const fs::path src = srcAbs.toStdWString();
        const fs::path dst = tmpRoot / e.originalName.toStdWString();

        boost::system::error_code e2;
        if (!fs::exists(src, e2)) {
            qWarning() << "StructuredPacker: пропуск — файл не найден:"
                       << e.diskPath
                       << "(ожидался как" << e.originalName << ")";
            continue;
        }

        // Создаём подкаталоги, если originalName вдруг содержит '/'
        fs::create_directories(dst.parent_path(), e2);

        e2.clear();
        if (!e.moduleKind.isEmpty()) {
            // Restore the original compound 1C module entry from a flat .bsl file.
            fs::create_directories(dst, e2);
            if (!e2) {
                QFile info(QString::fromStdWString((dst / "info").wstring()));
                if (!info.open(QIODevice::WriteOnly)) {
                    e2 = boost::system::errc::make_error_code(boost::system::errc::io_error);
                } else {
                    info.write(e.moduleInfo);
                    info.close();
                    QFile sourceFile(QString::fromStdWString(src.wstring()));
                    QFile text(QString::fromStdWString((dst / "text").wstring()));
                    if (!sourceFile.open(QIODevice::ReadOnly) || !text.open(QIODevice::WriteOnly)) {
                        e2 = boost::system::errc::make_error_code(boost::system::errc::io_error);
                    } else {
                        // Файл модуля пишется с BOM (как в выгрузке платформы), а BOM
                        // элемента восстанавливается по манифесту — из файла его убираем.
                        QByteArray moduleBytes = sourceFile.readAll();
                        if (moduleBytes.startsWith(QByteArray::fromHex("efbbbf")))
                            moduleBytes.remove(0, 3);
                        if (e.moduleTextHadBom)
                            text.write(QByteArray::fromHex("efbbbf"));
                        text.write(moduleBytes);
                        sourceFile.close();
                        text.close();
                    }
                }
            }
        } else if (!e.formModulePath.isEmpty()) {
            // Описание формы: модуль формы лежал отдельным файлом
            // (см. StructuredUnpacker::extractFormModules) — возвращаем его
            // третьим элементом верхнего уровня, остальной текст не меняется.
            const QString formAbs   = QDir(m_inputDir).filePath(e.diskPath);
            const QString moduleAbs = QDir(m_inputDir).filePath(e.formModulePath);

            QFile formFile(formAbs);
            QFile moduleFile(moduleAbs);
            if (!formFile.open(QIODevice::ReadOnly) || !moduleFile.open(QIODevice::ReadOnly)) {
                qWarning() << "StructuredPacker: не открыть описание формы или её модуль:"
                           << e.diskPath << e.formModulePath;
                e2 = boost::system::errc::make_error_code(boost::system::errc::io_error);
            } else {
                // BOM определяем по сырым байтам: QString::fromUtf8 его не сохраняет.
                QByteArray rawForm = formFile.readAll();
                const bool bom = rawForm.startsWith(QByteArray::fromHex("efbbbf"));
                if (bom)
                    rawForm.remove(0, 3);
                const QString body = QString::fromUtf8(rawForm);

                // Редактор мог добавить BOM и в файл модуля — отбрасываем его.
                QByteArray rawModule = moduleFile.readAll();
                if (rawModule.startsWith(QByteArray::fromHex("efbbbf")))
                    rawModule.remove(0, 3);
                const QString module = QString::fromUtf8(rawModule);

                int     begin = 0;
                int     end   = 0;
                QString slot;
                if (!OneCText::topLevelStringSpan(body, 2, &begin, &end, &slot)) {
                    qWarning() << "StructuredPacker: в описании формы нет элемента модуля:"
                               << e.diskPath;
                    e2 = boost::system::errc::make_error_code(boost::system::errc::invalid_argument);
                } else {
                    const QString updated = (bom ? QString(QChar(0xFEFF)) : QString())
                                          + body.left(begin) + OneCText::toElement(module)
                                          + body.mid(end);
                    QFile dstFile(QString::fromStdWString(dst.wstring()));
                    if (!dstFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                        qWarning() << "StructuredPacker: не записать описание формы:"
                                   << QString::fromStdWString(dst.wstring());
                        e2 = boost::system::errc::make_error_code(boost::system::errc::io_error);
                    } else {
                        dstFile.write(updated.toUtf8());
                        dstFile.close();
                    }
                }
                formFile.close();
                moduleFile.close();
            }
        } else if (!e.localizedFiles.isEmpty()) {
            // Справка/HTML-макет: на диске элемент представлен файлами .html по
            // языкам. Если содержимое не менялось — берём исходные байты элемента
            // (тогда контейнер получается побайтово прежним), иначе собираем
            // структуру заново из файлов.
            static const char bom[] = "\xef\xbb\xbf";

            QVector<OneCText::LocalizedItem> original;
            bool same = OneCText::parseLocalizedHtml(e.localizedOriginal, &original)
                     && original.size() == e.localizedFiles.size();
            if (same) {
                for (int i = 0; i < original.size(); ++i) {
                    QFile hf(QDir(m_inputDir).filePath(e.localizedFiles.at(i)));
                    if (!hf.open(QIODevice::ReadOnly)) { same = false; break; }
                    QByteArray current = hf.readAll();
                    hf.close();
                    // Редактор мог добавить или снять BOM — это не изменение данных.
                    if (current.startsWith(bom)) current.remove(0, 3);
                    QByteArray stored = original.at(i).data;
                    if (stored.startsWith(bom)) stored.remove(0, 3);
                    if (current != stored) { same = false; break; }
                }
            }

            QByteArray payload;
            if (same) {
                payload = e.localizedOriginal;
            } else {
                QVector<OneCText::LocalizedItem> rebuilt;
                rebuilt.reserve(e.localizedFiles.size());
                for (int i = 0; i < e.localizedFiles.size(); ++i) {
                    QFile hf(QDir(m_inputDir).filePath(e.localizedFiles.at(i)));
                    OneCText::LocalizedItem item;
                    item.lang = i < original.size()
                              ? original.at(i).lang
                              : QFileInfo(e.localizedFiles.at(i)).completeBaseName();
                    if (hf.open(QIODevice::ReadOnly)) {
                        item.data = hf.readAll();
                        hf.close();
                    }
                    rebuilt.append(item);
                }
                payload = OneCText::buildLocalizedHtml(rebuilt);
                qInfo() << "StructuredPacker: справка или HTML-макет изменены —"
                           " собираю элемент заново:" << e.originalName;
            }

            QFile out(QString::fromStdWString(dst.wstring()));
            if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)
                || out.write(payload) != payload.size()) {
                qWarning() << "StructuredPacker: не записать элемент справки:"
                           << e.originalName;
                e2 = boost::system::errc::make_error_code(boost::system::errc::io_error);
            } else {
                out.close();
            }
        } else {
            // Составной элемент (каталог info+text) или обычный файл — в .tmp_pack
            // кладём ссылку, а не копию: байты этих файлов не переносятся.
            linkOrCopy(src, dst, e2);
        }
        
        if (e2) {
            qWarning() << "StructuredPacker: ошибка копирования"
                       << e.diskPath << "->" << e.originalName
                       << ":" << QString::fromStdString(e2.message());
            continue;
        }
        ++copied;
    }

    if (copied == 0) {
        qWarning() << "StructuredPacker: ни одного файла не скопировано — "
                      "сборка отменена.";
        cleanupTemp();
        return false;
    }

    qDebug() << "StructuredPacker: скопировано по manifest" << copied
             << "из" << ordered.size() << "записей";

    // 3. Собираем контейнер из m_tempDir — тем же API, что и fallback.
    const int ret = v8unpack::BuildCfFile(m_tempDir.toStdString(), m_outputFile.toStdString(), m_noDeflate);

    cleanupTemp();

    if (ret != v8unpack::V8UNPACK_OK) {
        qWarning() << "StructuredPacker: BuildCfFile вернул код" << ret;
        return false;
    }

    return true;
}

// ────────────────────────────────────────────────────────────────
//  Полный конвейер сборки по старой схеме (без manifest).
//  Используется, если .e8unpack/manifest.json не найден — например,
//  когда каталог был распакован сторонним инструментом.
//
//  ВАЖНО: пропускает служебные каталоги .e8unpack и .tmp_pack,
//  чтобы они не попали в собранный .cf.
// ────────────────────────────────────────────────────────────────
bool StructuredPacker::packFromDirectoryFallback()
{
    if (!QFileInfo::exists(m_inputDir)) {
        qWarning() << "StructuredPacker: каталог не найден:" << m_inputDir;
        return false;
    }

    if (!flattenToTemp()) {
        cleanupTemp();
        return false;
    }

    const int ret = v8unpack::BuildCfFile(m_tempDir.toStdString(), m_outputFile.toStdString(), m_noDeflate);

    cleanupTemp();

    if (ret != v8unpack::V8UNPACK_OK) {
        qWarning() << "StructuredPacker: BuildCfFile вернул код" << ret;
        return false;
    }

    return true;
}

// ────────────────────────────────────────────────────────────────
//  Пропуск служебных каталогов при обходе.
//  Вызывается из flattenToTemp() ДО обработки каждой записи.
// ────────────────────────────────────────────────────────────────
//  Вариант A: если flattenToTemp использует recursive_directory_iterator,
//  добавьте в начало цикла такую проверку:
//
//      const auto rel = fs::relative(entry.path(), m_inputDir.toStdWString());
//      if (rel.empty()) continue;
//      const std::wstring first = rel.begin()->wstring();
//      if (first == L".e8unpack" || first == L".tmp_pack") continue;
//
//  Вариант B: если хочется инкапсулировать проверку — вот отдельный
//  вспомогательный метод:
//
static bool isServiceDir(const fs::path& rel)
{
    if (rel.empty()) 
        return false;
    
    const std::wstring first = rel.begin()->wstring();
    
    return first == L".e8unpack" || first == L".tmp_pack";
}

} // namespace v8