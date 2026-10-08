// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
#include "StructuredUnpacker.h"
#include "src/core/MetadataTypes.h"
#include "V8File.h"   // v8unpack::Parse
#include "src/metadata/ConfigStructureReader.h"
#include "src/output/UnpackManifest.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QDebug>
#include <system_error>
#include <vector>

namespace v8unpack {
namespace fs = boost::filesystem;

static bool looksLikeGuid(const QString &name)
{
    static const QRegularExpression re(
        R"(^[0-9a-fA-F]{8}-?[0-9a-fA-F]{4}-?[0-9a-fA-F]{4}-?[0-9a-fA-F]{4}-?[0-9a-fA-F]{12}$)");
    return re.match(name).hasMatch();
}

StructuredUnpacker::StructuredUnpacker(const QString &inputFile,
                                       const QString &outputDir,
                                       const MetadataMap *map)
    : m_inputFile(inputFile)
    , m_outputDir(outputDir)
    , m_tempDir(QDir(outputDir).filePath(".tmp_unpack"))
    , m_map(map)
{}

bool StructuredUnpacker::isServiceName(const QString &name)
{
    static const QStringList kService = {
        "root", "version", "versions",
        "_config", "config", "configinfo",
        "metadata", "ConfigDumpInfo"
    };
    if (!looksLikeGuid(name))
        return true;
    
    return kService.contains(name);
}

bool StructuredUnpacker::loadObjectNames()
{
    QString configDir = m_tempDir;

    const QString serviceDir    = QDir(m_tempDir).filePath(serviceFolderName());
    const QString rootInService = QDir(serviceDir).filePath(QStringLiteral("root"));
    const QString rootInTemp    = QDir(m_tempDir).filePath(QStringLiteral("root"));

    if (QFileInfo::exists(rootInService))
        configDir = serviceDir;
    else if (QFileInfo::exists(rootInTemp))
        configDir = m_tempDir;
    else {
        configDir = findRootDir();
        if (configDir.isEmpty()) {
            qWarning() << "StructuredUnpacker: файл root не найден в" << m_tempDir;
            return false;
        }
    }

    qDebug() << "StructuredUnpacker: файлы структуры в" << configDir;

    // ✅ Передаём путь как wstring, чтобы кириллица не портилась
    ConfigStructureReader reader(configDir.toStdWString());

    if (!reader.loadRoot()) { 
        qWarning() << "... root fail"; 
        return false; 
    }
    
    if (!reader.loadObjectGroups()) { 
        qWarning() << "... groups fail"; 
        return false; 
    }

    m_configGuid   = reader.configGuid();
    m_nameCache    = reader.resolveAllNames();
    m_groups_cache = reader.groups();

    // Nested subsystems are listed in each subsystem's own metadata file.
    const QVector<ObjectGroup> nestedGroups = reader.resolveNestedSubsystems();
    m_subsystemParent = reader.resolveSubsystemParents();
    for (const auto& group : nestedGroups) {
        m_groups_cache.append(group);
        for (const auto& objGuid : group.objectGuids) {
            const QString key = objGuid.toLower();
            const QString name = reader.resolveName(objGuid);
            if (!name.isEmpty())
                m_nameCache.insert(key, name);
        }
    }

    m_typeOf.clear();
    for (const auto& g : m_groups_cache) {
        for (const auto& objGuid : g.objectGuids)
            m_typeOf.insert(objGuid.toLower(), g.typeName);
    }

    qDebug() << "StructuredUnpacker: имён:" << m_nameCache.size()
             << "типов:" << m_typeOf.size()
             << "configGuid:" << m_configGuid;

    return !m_nameCache.isEmpty();
}

QString StructuredUnpacker::objectName(const QString &guid) const
{
    const QString norm = normalizeGuid(guid);

    auto it = m_nameCache.find(norm);
    if (it != m_nameCache.end() && !it.value().isEmpty())
        return it.value();

    if (m_map && !m_map->isEmpty()) {
        auto info = m_map->lookup(guid);
        if (info.has_value())
            return info->objectName;
    }

    return guid;
}

QString StructuredUnpacker::findRootDir() const
{
    boost::system::error_code ec;
    const fs::path tempRoot(m_tempDir.toStdWString());   

    if (!fs::exists(tempRoot, ec))
        return {};

    fs::recursive_directory_iterator it(tempRoot, fs::directory_options::skip_permission_denied, ec);
    fs::recursive_directory_iterator end;

    for (; it != end; it.increment(ec)) {
        if (ec) break;

        if (fs::is_regular_file(it->path(), ec)
            && it->path().filename() == L"root")          
        {
            return QString::fromStdWString(it->path().parent_path().wstring());
        }
    }
    return {};
}

QString StructuredUnpacker::baseGuidFromFileName(const QString &fileName)
{
    const int dotPos = fileName.lastIndexOf(QLatin1Char('.'));
    if (dotPos > 0) {
        bool ok = false;
        fileName.mid(dotPos + 1).toInt(&ok);
        if (ok)
            return fileName.left(dotPos);
    }
    return fileName;
}

bool StructuredUnpacker::indexChildElements()
{
    m_childIndex.clear();

    if (m_configGuid.isEmpty()) {
        qWarning() << "StructuredUnpacker: configGuid не определён";
        return false;
    }

    const QString configDir    = QDir(m_tempDir).filePath(serviceFolderName());
    const QString configDirAlt = m_tempDir;

    const QString effectiveDir = QFileInfo::exists(QDir(configDir).filePath("root")) ? configDir : configDirAlt;

    ConfigStructureReader reader(effectiveDir.toStdWString());
    // Группы нужны и для секций, и для поиска владельцев элементов
    // по текстам объектов (проход 2).
    if (!reader.loadRoot())
        qWarning() << "StructuredUnpacker: indexChildElements: root не прочитан";
    if (!reader.loadObjectGroups())
        qWarning() << "StructuredUnpacker: indexChildElements: группы не прочитаны";

    for (const auto& g : m_groups_cache) {
        for (const QString& objGuid : g.objectGuids) {
            const auto sections = reader.resolveSections(objGuid);

            for (const auto& sec : sections) {
                for (const QString& childGuid : sec.elementGuids) {
                    ChildInfo info;
                    info.parentGuid  = objGuid;
                    info.parentName  = m_nameCache.value(objGuid.toLower(), objGuid);
                    info.parentType  = g.typeName;
                    info.sectionName = sec.sectionName;

                    m_childIndex.insert(childGuid.toLower(), info);
                }
            }
        }
    }

    // ── Проход 1а: имена элементов секций (нужны для имён подкаталогов) ──
    {
        QStringList childGuids;
        for (auto it = m_childIndex.cbegin(); it != m_childIndex.cend(); ++it)
            childGuids.append(it.key());

        const auto found = reader.findElementOwners(childGuids);
        for (auto it = found.cbegin(); it != found.cend(); ++it) {
            if (!it.value().name.isEmpty())
                m_nameCache.insert(it.key(), it.value().name);
        }
    }

    // ── Проход 2: элементы, на которые объект ссылается идентификационным
    // узлом {1,0,<GUID>},"<Имя>", но которых нет в списках секций
    // (например, обработчики команд). Владельца ищем по текстам объектов.
    {
        const QSet<QString> candidates = unplacedMemberGuids();
        if (!candidates.isEmpty()) {
            QStringList list;
            for (const QString& g : candidates)
                list.append(g);

            const QHash<QString, ConfigStructureReader::ElementOwner> owners =
                reader.findElementOwners(list);

            for (auto it = owners.cbegin(); it != owners.cend(); ++it) {
                const QString childGuid = it.key();
                const ConfigStructureReader::ElementOwner& eo = it.value();

                if (eo.ownerGuid.isEmpty())
                    continue;                       // владелец неизвестен — оставляем как есть
                if (eo.ownerGuid.compare(m_configGuid, Qt::CaseInsensitive) == 0)
                    continue;                       // элементы самой конфигурации не переносим
                if (m_childIndex.contains(childGuid) || m_typeOf.contains(childGuid))
                    continue;                       // уже размещён или объект верхнего уровня

                ChildInfo info;
                info.parentGuid  = eo.ownerGuid;
                info.parentName  = m_nameCache.value(eo.ownerGuid, eo.ownerGuid);
                info.parentType  = m_typeOf.value(eo.ownerGuid, serviceFolderName());
                // Обработчики команд объекта/формы — в подкаталог «Команды»,
                // прочие неразобранные элементы — в «Элементы».
                info.sectionName = eo.name.startsWith(QStringLiteral("Команда"), Qt::CaseInsensitive)
                                       ? QStringLiteral("Команды")
                                       : QStringLiteral("Элементы");

                m_childIndex.insert(childGuid, info);
                if (!eo.name.isEmpty())
                    m_nameCache.insert(childGuid, eo.name);
            }
        }
    }

    qDebug() << "StructuredUnpacker: дочерних элементов проиндексировано:"
             << m_childIndex.size();

    return !m_childIndex.isEmpty();
}

QSet<QString> StructuredUnpacker::unplacedMemberGuids() const
{
    QSet<QString> result;

    boost::system::error_code ec;
    const fs::path tempRoot(m_tempDir.toStdWString());

    for (const auto& entry : fs::directory_iterator(tempRoot, ec)) {
        if (ec) break;

        const QString name = QString::fromStdWString(entry.path().filename().wstring());
        const QString base = baseGuidFromFileName(name);
        if (!looksLikeGuid(base))
            continue;

        const QString key = base.toLower();
        if (key == m_configGuid.toLower()) continue;
        if (m_typeOf.contains(key) || m_childIndex.contains(key)) continue;
        if (isServiceName(base)) continue;

        result.insert(key);
    }
    return result;
}

QString StructuredUnpacker::targetRelativePath(const fs::path &rel, const QString &guid, const QString &parentGuid) const
{
    const int depth = static_cast<int>(std::distance(rel.begin(), rel.end()));

    // ── 1. Верхний уровень
    if (depth == 1) {
        if (guid.isEmpty())
            return serviceFolderName() + "/" + QString::fromStdWString(rel.filename().wstring());   

        const auto &types = metadataTypes();
        
        auto it = types.find(normalizeGuid(guid));
        
        if (it != types.end())
            return it.value();
        
        return serviceFolderName() + "/" + guid;
    }

    // ── 2. Второй уровень
    if (depth == 2 && !parentGuid.isEmpty()) {
        const auto &types = metadataTypes();
        auto typeIt = types.find(normalizeGuid(parentGuid));
        const QString typeName = (typeIt != types.end()) ? typeIt.value() : serviceFolderName();

        return typeName + "/" + objectName(guid);
    }

    // ── 3+ Глубже
    auto it = rel.begin();
    const QString typeGuid = QString::fromStdWString(it->wstring()); ++it;   
    const QString objGuid  = (it != rel.end()) ? QString::fromStdWString(it->wstring()) : QString();

    if (objGuid.isEmpty())
        return QString::fromStdWString(rel.generic_wstring());              

    const auto &types = metadataTypes();
    
    auto typeIt = types.find(normalizeGuid(typeGuid));
    
    const QString typeName = (typeIt != types.end()) ? typeIt.value() : serviceFolderName();

    const QString objName = objectName(objGuid);

    fs::path tail;
    for (++it; it != rel.end(); ++it)
        tail /= *it;

    QString result = typeName + "/" + objName;
    if (!tail.empty())
        result += "/" + QString::fromStdWString(tail.generic_wstring());    // ✅

    return result;
}

bool StructuredUnpacker::unpackToTemp()
{
    boost::system::error_code ec;
    fs::remove_all(m_tempDir.toStdWString(), ec);   

    QDir().mkpath(m_tempDir);

    std::vector<std::string> filter;
    
    // API v8unpack принимает std::string. Передаём UTF-8,
    // а внутри V8File.cpp пути открываются через boost::filesystem::fstream.
    int ret = v8unpack::Parse(m_inputFile.toStdString(), m_tempDir.toStdString(), filter);

    return ret == v8unpack::V8UNPACK_OK;
}

bool StructuredUnpacker::flattenModuleEntries()
{
    // Типы модулей самой конфигурации: по суффиксу элемента контейнера
    // (.0/.5/.6/.7), а если суффикс неизвестен — по первой строке комментария.
    static const QHash<QString, QString> moduleNames = {
        {QStringLiteral("Модуль управляемого приложения"), QStringLiteral("Модуль управляемого приложения")},
        {QStringLiteral("Модуль сеанса"), QStringLiteral("Модуль сеанса")},
        {QStringLiteral("Модуль внешнего соединения"), QStringLiteral("Модуль внешнего соединения")},
        {QStringLiteral("Модуль обычного приложения"), QStringLiteral("Модуль обычного приложения")}
    };
    static const QHash<QString, QString> moduleKindsBySuffix = {
        {QStringLiteral("0"), QStringLiteral("Модуль обычного приложения")},
        {QStringLiteral("5"), QStringLiteral("Модуль внешнего соединения")},
        {QStringLiteral("6"), QStringLiteral("Модуль управляемого приложения")},
        {QStringLiteral("7"), QStringLiteral("Модуль сеанса")}
    };

    for (auto& entry : m_manifest.entries()) {
        const QString oldPath = QDir::fromNativeSeparators(entry.diskPath);
        if (oldPath.isEmpty() || entry.originalName.isEmpty())
            continue;

        // Любой текст модуля 1С лежит составным элементом "<GUID>.<суффикс>"
        // с парой файлов info/text внутри.
        const QString baseGuid = baseGuidFromFileName(entry.originalName);
        if (baseGuid == entry.originalName)
            continue;                       // без суффикса — не составной элемент

        const QString sourceDir = QDir(m_outputDir).filePath(oldPath);
        if (!QFileInfo(sourceDir).isDir())
            continue;

        QFile textFile(QDir(sourceDir).filePath(QStringLiteral("text")));
        if (!textFile.open(QIODevice::ReadOnly))
            continue;
        const QByteArray rawText = textFile.readAll();
        textFile.close();

        QByteArray infoBytes;
        QFile infoFile(QDir(sourceDir).filePath(QStringLiteral("info")));
        if (infoFile.open(QIODevice::ReadOnly)) {
            infoBytes = infoFile.readAll();
            infoFile.close();
        }

        QString text = QString::fromUtf8(rawText);
        if (text.startsWith(QChar(0xFEFF)))
            text.remove(0, 1);

        const int dotPos = entry.originalName.lastIndexOf(QLatin1Char('.'));
        const QString suffix    = dotPos >= 0 ? entry.originalName.mid(dotPos + 1) : QString();
        const QString parentRel = QFileInfo(oldPath).path();      // каталог объекта

        QString kind;
        QString newPath;

        if (parentRel == serviceFolderName()) {
            // ── Модули самой конфигурации ─────────────────────────────
            kind = moduleKindsBySuffix.value(suffix);
            if (kind.isEmpty()) {
                const QStringList lines = text.split(QLatin1Char('\n'));
                for (const QString& line : lines) {
                    const QString candidate = line.trimmed();
                    if (candidate.isEmpty())
                        continue;
                    if (candidate.startsWith(QStringLiteral("//"))) {
                        const QString comment = candidate.mid(2).trimmed();
                        for (auto it = moduleNames.cbegin(); it != moduleNames.cend(); ++it) {
                            if (comment.startsWith(it.key())) {
                                kind = it.value();
                                break;
                            }
                        }
                    }
                    break;
                }
            }
            if (kind.isEmpty())
                continue;
            newPath = parentRel + QLatin1Char('/') + kind + QStringLiteral(".bsl");
        } else {
            // ── Модули объектов метаданных (в т.ч. общих модулей) ──────
            // Каталог объекта уже назван его именем, полученным из метаданных:
            //   "Общие/Общие модули/Основной", "Справочники/Номенклатура", ...
            // Файл модуля получает то же имя, что и сам объект.
            const QString moduleName = parentRel.section(QLatin1Char('/'), -1, -1);
            if (moduleName.isEmpty())
                continue;
            kind    = moduleName;
            newPath = parentRel + QLatin1Char('/') + moduleName + QStringLiteral(".bsl");
            if (QFileInfo::exists(QDir(m_outputDir).filePath(newPath))) {
                // У объекта несколько модулей — различаем их по суффиксу элемента.
                newPath = parentRel + QLatin1Char('/') + moduleName
                          + QLatin1Char('.') + suffix + QStringLiteral(".bsl");
            }
        }

        const QString targetPath = QDir(m_outputDir).filePath(newPath);
        if (QFileInfo::exists(targetPath)) {
            qWarning() << "StructuredUnpacker: BSL destination already exists:" << targetPath;
            continue;
        }

        QFile bslFile(targetPath);
        if (!bslFile.open(QIODevice::WriteOnly)) {
            qWarning() << "StructuredUnpacker: cannot write BSL file:" << targetPath;
            continue;
        }
        bslFile.write(text.toUtf8());
        bslFile.close();

        boost::system::error_code ec;
        fs::remove_all(fs::path(sourceDir.toStdWString()), ec);
        if (ec) {
            qWarning() << "StructuredUnpacker: cannot remove old module directory:" << sourceDir;
            QFile::remove(targetPath);
            continue;
        }

        entry.diskPath         = newPath;
        entry.moduleKind       = kind;
        entry.moduleInfo       = infoBytes;
        entry.moduleTextHadBom = rawText.startsWith(QByteArray::fromHex("efbbbf"));
        entry.rawSize          = QFileInfo(targetPath).size();
        qInfo() << "StructuredUnpacker: module flattened:" << entry.originalName << "->" << newPath;
    }
    return true;
}

bool StructuredUnpacker::buildAndApplyPlan()
{
    boost::system::error_code ec;
    const fs::path tempRoot(m_tempDir.toStdWString());      
    const fs::path targetRoot(m_outputDir.toStdWString());  

    if (!fs::exists(tempRoot, ec))
        return false;

    // ── 0. Заранее создаём все каталоги верхнего уровня из справочника
    {
        fs::create_directories(targetRoot / serviceFolderName().toStdWString(), ec);

        const auto& types = metadataTypes();

        for (auto it = types.begin(); it != types.end(); ++it) {
            
            const QString& typeName = it.value();
            
            if (typeName.isEmpty())
                continue;

            fs::create_directories(targetRoot / typeName.toStdWString(), ec);   

        }
    }

    struct MoveItem { fs::path src; fs::path dst; };
    std::vector<MoveItem> plan;

    for (const auto& entry : fs::directory_iterator(tempRoot, ec)) {
        if (ec) break;

        const fs::path& src = entry.path();
        const QString name = QString::fromStdWString(src.filename().wstring());

        QString targetRel;
        const QString baseGuid = baseGuidFromFileName(name);
        const QString key = baseGuid.toLower();

        if (name == QLatin1String("root")
            || name == QLatin1String("version")
            || name == QLatin1String("versions")
            || name == m_configGuid)
        {
            targetRel = serviceFolderName() + "/" + name;
        }
        else if (m_typeOf.contains(key)) {
            const QString typeName = m_typeOf.value(key);
            const QString objName  = m_nameCache.value(key, baseGuid);
            QString objectPath = typeName + "/" + objName;
            if (normalizeGuid(typeName) == normalizeGuid(QStringLiteral("Общие/Подсистемы"))) {
                QString parentKey = m_subsystemParent.value(key);
                QStringList chain;
                chain.append(objName);
                QSet<QString> visited;
                visited.insert(key);
                while (!parentKey.isEmpty() && !visited.contains(parentKey)) {
                    visited.insert(parentKey);
                    const QString parentName = m_nameCache.value(parentKey);
                    if (parentName.isEmpty()) break;
                    chain.prepend(parentName);
                    parentKey = m_subsystemParent.value(parentKey);
                }
                objectPath = typeName + "/" + chain.join("/");
            }
            targetRel = objectPath + "/" + name;
        }
        else if (m_childIndex.contains(key)) {
            const ChildInfo info = m_childIndex.value(key);
            // Элемент кладём в подкаталог с его именем — как общие формы:
            //   <Тип>/<Имя объекта>/Формы/<Имя формы>/<GUID элемента>
            const QString childName = m_nameCache.value(key);
            const QString folder = childName.isEmpty() ? QString() : childName + "/";
            targetRel = info.parentType + "/" + info.parentName + "/"
                        + info.sectionName + "/" + folder + name;
        }
        else {
            targetRel = serviceFolderName() + "/" + name;
        }

        fs::path dst = targetRoot / targetRel.toStdWString();   
        plan.push_back({ src, dst });
    }

    // ── Перенос файлов + параллельная запись manifest'а ──────────
    // Сбрасываем счётчик на случай повторного вызова run()
    m_manifest.entries().clear();
    m_nextOriginalIndex = 0;

    for (const auto& item : plan) {
        boost::system::error_code e2;
        fs::create_directories(item.dst.parent_path(), e2);

        fs::path actualDst = item.dst;
        bool     moved     = false;

        if (fs::exists(item.dst, e2)) {
            fs::path alt = item.dst;
            int n = 1;
            while (fs::exists(alt, e2)) {
                alt = item.dst;
                alt += (L"." + std::to_wstring(n++));       
            }
            e2.clear();
            fs::rename(item.src, alt, e2);
            if (!e2) {
                moved     = true;
                actualDst = alt;
            }
        } else {
            e2.clear();
            fs::rename(item.src, item.dst, e2);
            if (e2) {
                e2.clear();
                fs::copy(item.src, item.dst, fs::copy_options::recursive | fs::copy_options::overwrite_existing, e2);
                if (!e2) {
                    boost::system::error_code rmEc;
                    fs::remove_all(item.src, rmEc);
                    moved = true;
                }
            } else {
                moved = true;
            }
        }

        if (!moved) {
            qWarning() << "Не удалось перенести"
                       << QString::fromStdWString(item.src.wstring())   
                       << "->" << QString::fromStdWString(item.dst.wstring())
                       << ":" << QString::fromStdString(e2.message());
            continue;
        }

        ++m_moved;

        // ── Записываем запись manifest'а ─────────────────────────
        ManifestEntry me;
        me.originalName  = QString::fromStdWString(item.src.filename().wstring());
        me.originalIndex = m_nextOriginalIndex++;

        // diskPath — относительно m_outputDir (то, что нужно packer'у)
        boost::system::error_code relEc;
        const fs::path rel = fs::relative(actualDst, targetRoot, relEc);
        if (!relEc && !rel.empty()) {
            me.diskPath = QDir::fromNativeSeparators(QString::fromStdWString(rel.generic_wstring()));
        } else {
            // fallback — абсолютный путь (на случай странных ФС)
            me.diskPath = QDir::fromNativeSeparators(QString::fromStdWString(actualDst.wstring()));
        }

        // Размер данных (распакованных). Сжатие при сборке packer
        // определит сам, но rawSize полезен для валидации.
        boost::system::error_code sizeEc;
        
        const auto sz = fs::file_size(actualDst, sizeEc);
        me.rawSize = sizeEc ? 0 : static_cast<qint64>(sz);

        // По умолчанию считаем, что элемент был сжат — так его и упакуем.
        // Если в будущем V8Container начнёт отдавать реальный флаг — подставим его.
        me.compressed = true;

        m_manifest.entries().append(me);
    }

    qDebug() << "StructuredUnpacker: перенесено файлов:" << m_moved
             << "записей в manifest:" << m_manifest.entries().size();

    return true;
}

void StructuredUnpacker::cleanupTemp()
{
    boost::system::error_code ec;
    fs::remove_all(m_tempDir.toStdWString(), ec);   
}

bool StructuredUnpacker::run()
{
    QDir().mkpath(m_outputDir);

    if (!unpackToTemp())
        return false;

    if (!loadObjectNames())
        qWarning() << "StructuredUnpacker: имена объектов не загружены";

    if (!indexChildElements())
        qWarning() << "StructuredUnpacker: дочерние элементы не проиндексированы";

    if (!buildAndApplyPlan()) {
        cleanupTemp();
        return false;
    }

    flattenModuleEntries();

    // ── Сохраняем manifest ───────────────────────────────────────
    // Делаем это ПОСЛЕ успешного переноса всех файлов и ДО cleanupTemp(),
    // чтобы при желании можно было дописать в manifest данные,
    // если в будущем появится такая необходимость.
    m_manifest.setSourceContainer(QFileInfo(m_inputFile).fileName());

    QString manifestError;
    if (!m_manifest.save(m_outputDir, &manifestError)) {
        // Не фатально: файлы распакованы, пользователь может собрать
        // контейнер и по старой схеме (fallback в StructuredPacker).
        qWarning() << "StructuredUnpacker: не удалось сохранить manifest:"
                   << manifestError;
    } else {
        qInfo() << "StructuredUnpacker: manifest записан ("
                << m_manifest.entries().size() << "записей) ->"
                << UnpackManifest::pathFor(m_outputDir);
    }

    cleanupTemp();
    return true;
}

} // namespace v8