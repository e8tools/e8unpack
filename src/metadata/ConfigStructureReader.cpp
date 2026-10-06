// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
#include "ConfigStructureReader.h"
#include "src/core/MetadataTypes.h"
#include "src/metadata/SectionTypes.h"     // ← добавить это

#include <QFile>
#include <QSet>
#include <QDebug>
#include <system_error>

namespace v8unpack {
namespace fs = boost::filesystem;

// ═══ Forward declarations анонимных helper-ов ═══
namespace {
    bool matchSection(tree* node, QString& sectionGuid, int& count);
    void collectSections(tree* node, QVector<SectionInfo>& result);
}

// ═══════════════════════════════════════════════════════════════════════════
//  Вспомогательные функции
// ═══════════════════════════════════════════════════════════════════════════
namespace {

/// Открыть файл и распарсить его через parse_1Cstream.
/// Возвращает nullptr при ошибке.
tree* loadTree(const fs::path& file, const QString& path)
{
    QFile f(QString::fromStdWString(file.wstring()));   // ✅
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "ConfigStructureReader: не удалось открыть"
                   << QString::fromStdWString(file.wstring());
        return nullptr;
    }
    return parse_1Cstream(&f, path);
}


namespace {

/// Извлечь GUID из узла вида {2, <guid>, <base64>}.
/// Возвращает пустую строку, если структура не совпала.
QString tryExtractGuidFromNode(tree* node)
{
    if (!node || node->get_num_subnode() < 2)
        return {};

    tree* first  = node->get_subnode(0);
    tree* second = node->get_subnode(1);

    if (first && second
        && first->get_type() == nd_number
        && first->get_value() == QLatin1String("2")
        && second->get_type() == nd_guid)
    {
        return second->get_value();
    }
    return {};
}

} // namespace


/// Извлечь GUID из узла root.
/// Формат файла root: {2, <guid>, <base64-строка>}
/// Нас интересует второй потомок (индекс 1).
QString extractConfigGuid(tree* root)
{
    if (!root)
        return {};

    // 1) Прямой случай: сам root — это {2, guid, base64}
    if (const QString g = tryExtractGuidFromNode(root); !g.isEmpty())
        return g;

    // 2) Обёртка: root — пустой nd_list, а {2, guid, base64} — его первый потомок
    if (root->get_num_subnode() >= 1) {
        if (const QString g = tryExtractGuidFromNode(root->get_subnode(0)); !g.isEmpty())
            return g;
    }

    // 3) Диагностика — если ничего не нашли, покажем, что вообще лежит в дереве
    qWarning() << "extractConfigGuid: не удалось найти GUID. "
               << "root has" << root->get_num_subnode() << "subnodes";
    for (int i = 0; i < root->get_num_subnode(); ++i) {
        tree* c = root->get_subnode(i);
        qWarning() << "  child" << i
                   << "type" << static_cast<int>(c->get_type())
                   << "subnodes" << c->get_num_subnode()
                   << "value" << c->get_value().left(40);
    }

    return {};
}


/// Проверить, что узел — это список вида {type_guid, count, ...guids}.
/// Возвращает true и заполняет параметры.
bool matchObjectGroup(tree* node, QString& typeGuid, int& count)
{
    if (!node || node->get_num_subnode() < 2)
        return false;

    tree* first  = node->get_subnode(0);
    tree* second = node->get_subnode(1);

    if (!first || !second)
        return false;

    if (first->get_type() != nd_guid || second->get_type() != nd_number)
        return false;

    typeGuid = first->get_value();
    count    = second->get_value().toInt();
    return count > 0;
}

/// Рекурсивный обход дерева и сбор всех групп.
void collectGroups(tree* node, QVector<ObjectGroup>& result, int depth = 0)
{
    if (!node) return;

    const int n = node->get_num_subnode();

    QString typeGuid;
    int count = 0;
    if (matchObjectGroup(node, typeGuid, count)) {
        // Проверяем, что тип нам известен.
        // Это отсеивает случайные совпадения.
        const QString norm = normalizeGuid(typeGuid);
        const QString typeName = metadataTypes().value(norm);

        if (!typeName.isEmpty()) {
            ObjectGroup group;
            group.typeGuid = typeGuid;
            group.typeName = typeName;

            // Собираем GUID-ы объектов — они идут подряд после count.
            for (int i = 2; i < n && i < 2 + count; ++i) {
                tree* obj = node->get_subnode(i);
                if (obj && obj->get_type() == nd_guid)
                    group.objectGuids.append(obj->get_value());
            }

            if (!group.objectGuids.isEmpty()) {
                result.append(group);
                // Не идём глубже — объекты уже собраны.
                return;
            }
        }
    }

    // Продолжаем обход
    for (int i = 0; i < n; ++i)
        collectGroups(node->get_subnode(i), result, depth + 1);
}

/// Найти в дереве узел вида {1,0,<objectGuid>} и вернуть
/// значение следующего за ним sibling-узла (это и есть имя).
QString findNameByGuid(tree* node, const QString& objectGuid)
{
    if (!node) return {};

    // Проверяем: узел имеет ровно 3 потомка [1, 0, guid].
    if (node->get_num_subnode() == 3) {
        tree* c0 = node->get_subnode(0);
        tree* c1 = node->get_subnode(1);
        tree* c2 = node->get_subnode(2);

        if (c0 && c1 && c2
            && c0->get_type() == nd_number && c0->get_value() == QLatin1String("1")
            && c1->get_type() == nd_number && c1->get_value() == QLatin1String("0")
            && c2->get_type() == nd_guid
            && c2->get_value().compare(objectGuid, Qt::CaseInsensitive) == 0)
        {
            // Имя — следующий sibling.
            // В дереве 1С: {1,0,guid},"Имя", ...
            tree* next = node->get_next();
            if (next && next->get_type() == nd_string)
                return next->get_value();

            // Иногда имя лежит не прямо следующим, а среди следующих
            // sibling-ов до первого узла-контейнера.
            for (tree* t = next; t; t = t->get_next()) {
                if (t->get_type() == nd_string) {
                    const QString v = t->get_value();
                    // Первая строка обычно — имя объекта.
                    if (!v.isEmpty())
                        return v;
                }
                // Дальше пошли другие узлы — стоп.
                if (t->get_type() == nd_list)
                    break;
            }
        }
    }

    // Рекурсивный поиск
    for (int i = 0; i < node->get_num_subnode(); ++i) {
        const QString found = findNameByGuid(node->get_subnode(i), objectGuid);
        if (!found.isEmpty())
            return found;
    }
    return {};
}

} // namespace

// ═══════════════════════════════════════════════════════════════════════════
//  ConfigStructureReader
// ═══════════════════════════════════════════════════════════════════════════

ConfigStructureReader::ConfigStructureReader(const fs::path& configDir)
    : m_configDir(configDir)
{}

bool ConfigStructureReader::loadRoot()
{
    const fs::path rootFile = m_configDir / "root";

    boost::system::error_code ec;
    if (!fs::exists(rootFile, ec)) {
        qWarning() << "ConfigStructureReader: файл root не найден:"
                   << QString::fromStdString(rootFile.string());
        return false;
    }

    tree* root = loadTree(rootFile, QStringLiteral("root"));
    if (!root)
        return false;

    m_configGuid = extractConfigGuid(root);
    delete root;

    if (m_configGuid.isEmpty()) {
        qWarning() << "ConfigStructureReader: не удалось извлечь GUID "
                      "файла структуры из root";
        return false;
    }

    qDebug() << "ConfigStructureReader: config GUID =" << m_configGuid;
    return true;
}

bool ConfigStructureReader::loadObjectGroups()
{
    m_groups.clear();

    if (m_configGuid.isEmpty()) {
        qWarning() << "ConfigStructureReader: сначала вызовите loadRoot()";
        return false;
    }

    const fs::path structFile = m_configDir / m_configGuid.toStdWString();

    boost::system::error_code ec;
    if (!fs::exists(structFile, ec)) {
        qWarning() << "ConfigStructureReader: файл структуры не найден:"
                   << QString::fromStdString(structFile.string());
        return false;
    }

    tree* root = loadTree(structFile, QStringLiteral("config-structure"));
    if (!root)
        return false;

    collectGroups(root, m_groups);
    delete root;

    qDebug() << "ConfigStructureReader: найдено групп:" << m_groups.size();
    for (const auto& g : m_groups) {
        qDebug() << "  " << g.typeName
                 << "(" << g.typeGuid << ")"
                 << "объектов:" << g.objectGuids.size();
    }

    return !m_groups.isEmpty();
}

QHash<QString, QString> ConfigStructureReader::resolveSubsystemParents() const
{
    QHash<QString, QString> parents;
    const QString subsystemType = normalizeGuid(QLatin1String(GUID_Subsystems));
    QSet<QString> visited;
    QVector<QString> pending;
    for (const auto& group : m_groups)
        if (normalizeGuid(group.typeGuid) == subsystemType)
            pending += group.objectGuids;
    while (!pending.isEmpty()) {
        const QString parentGuid = pending.takeLast();
        const QString parentKey = normalizeGuid(parentGuid);
        if (visited.contains(parentKey)) continue;
        visited.insert(parentKey);
        const fs::path objFile = m_configDir / parentGuid.toStdWString();
        boost::system::error_code ec;
        if (!fs::exists(objFile, ec)) continue;
        tree* objTree = loadTree(objFile, QStringLiteral("subsystem-parents:") + parentGuid);
        if (!objTree) continue;
        QVector<ObjectGroup> childGroups;
        collectGroups(objTree, childGroups);
        delete objTree;
        for (const auto& group : childGroups) {
            if (normalizeGuid(group.typeGuid) != subsystemType) continue;
            for (const auto& childGuid : group.objectGuids) {
                parents.insert(normalizeGuid(childGuid), parentKey);
                pending += childGuid;
            }
        }
    }
    return parents;
}

QVector<ObjectGroup> ConfigStructureReader::resolveNestedSubsystems() const
{
    QVector<ObjectGroup> result;
    const QString subsystemType = normalizeGuid(QLatin1String(GUID_Subsystems));
    QSet<QString> visited;
    QVector<QString> pending;

    for (const auto& group : m_groups) {
        if (normalizeGuid(group.typeGuid) == subsystemType)
            pending += group.objectGuids;
    }

    while (!pending.isEmpty()) {
        const QString parentGuid = pending.takeLast();
        const QString parentKey = normalizeGuid(parentGuid);
        if (visited.contains(parentKey))
            continue;
        visited.insert(parentKey);

        const fs::path objFile = m_configDir / parentGuid.toStdWString();
        boost::system::error_code ec;
        if (!fs::exists(objFile, ec))
            continue;

        tree* objTree = loadTree(objFile, QStringLiteral("nested-subsystem:") + parentGuid);
        if (!objTree)
            continue;

        QVector<ObjectGroup> childGroups;
        collectGroups(objTree, childGroups);
        delete objTree;

        for (const auto& childGroup : childGroups) {
            if (childGroup.objectGuids.isEmpty())
                continue;
            result.append(childGroup);
            if (normalizeGuid(childGroup.typeGuid) == subsystemType)
                pending += childGroup.objectGuids;
        }
    }
    return result;
}

QString ConfigStructureReader::resolveName(const QString& objectGuid) const
{
    if (objectGuid.isEmpty())
        return {};

    const fs::path objFile = m_configDir / objectGuid.toStdWString();

    boost::system::error_code ec;
    if (!fs::exists(objFile, ec)) {
        qWarning() << "ConfigStructureReader: файл объекта не найден:"
                   << QString::fromStdString(objFile.string());
        return {};
    }

    tree* objTree = loadTree(objFile, QStringLiteral("object:") + objectGuid);
    if (!objTree)
        return {};

    const QString name = findNameByGuid(objTree, objectGuid);
    delete objTree;

    return name;
}

QVector<SectionInfo> ConfigStructureReader::resolveSections(const QString &objGuid) const
{
    QVector<SectionInfo> result;

    if (objGuid.isEmpty())
        return result;

    const fs::path objFile = m_configDir / objGuid.toStdWString();

    boost::system::error_code ec;
    if (!fs::exists(objFile, ec))
        return result;

    tree* objTree = loadTree(objFile, QStringLiteral("sections:") + objGuid);
    if (!objTree)
        return result;

    collectSections(objTree, result);
    delete objTree;

    return result;

}

QHash<QString, QString>
ConfigStructureReader::resolveAllNames()
{
    QHash<QString, QString> result;

    for (const auto& group : m_groups) {
        for (const QString& objGuid : group.objectGuids) {
            const QString norm = normalizeGuid(objGuid);
            const QString name = resolveName(objGuid);

            if (!name.isEmpty()) {
                result.insert(norm, name);
            } else {
                // Fallback: если имя не нашли — используем GUID
                result.insert(norm, objGuid);
                qWarning() << "ConfigStructureReader: имя не найдено для"
                           << objGuid << "(" << group.typeName << ")";
            }
        }
    }

    qDebug() << "ConfigStructureReader: разрешено имён:"
             << result.size();
    return result;
}

namespace {

/// Проверить, что узел — это секция {section_guid, count, elem_guid1, ...}.
bool matchSection(tree* node, QString& sectionGuid, int& count)
{
    if (!node || node->get_num_subnode() < 2)
        return false;

    tree* first  = node->get_subnode(0);
    tree* second = node->get_subnode(1);

    if (!first || !second)
        return false;

    if (first->get_type() != nd_guid || second->get_type() != nd_number)
        return false;

    // Проверяем, что GUID — известная секция
    sectionGuid = first->get_value();
    if (!sectionTypes().contains(normalizeGuid(sectionGuid)))
        return false;

    count = second->get_value().toInt();
    return count > 0;
}

/// Рекурсивно собрать все секции внутри объекта.
void collectSections(tree* node, QVector<SectionInfo>& result)
{
    if (!node) return;

    const int n = node->get_num_subnode();

    QString sectionGuid;
    int count = 0;
    if (matchSection(node, sectionGuid, count)) {
        SectionInfo sec;
        sec.sectionGuid = sectionGuid;
        sec.sectionName = sectionTypes().value(normalizeGuid(sectionGuid));

        for (int i = 2; i < n && i < 2 + count; ++i) {
            tree* elem = node->get_subnode(i);
            if (elem && elem->get_type() == nd_guid)
                sec.elementGuids.append(elem->get_value());
        }

        if (!sec.elementGuids.isEmpty()) {
            result.append(sec);
            // Не углубляемся внутрь секции — там уже перечислены её элементы.
            return;
        }
    }

    for (int i = 0; i < n; ++i)
        collectSections(node->get_subnode(i), result);
}

} // namespace


} // namespace v8