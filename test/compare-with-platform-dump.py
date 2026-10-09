#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Сверка дерева, полученного `v8unpack -DE`, с выгрузкой платформы 1С.

Платформа (`Конфигуратор → Выгрузить конфигурацию в файлы`) раскладывает модули так:

    <Тип>/<Объект>/Ext/ObjectModule.bsl
    <Тип>/<Объект>/Forms/<Форма>/Ext/Form/Module.bsl
    <Тип>/<Объект>/Commands/<Команда>/Ext/CommandModule.bsl

v8unpack раскладывает их по-русски и называет файлы по роли модуля
(«Модуль объекта.bsl», «Модуль команды.bsl», «Модуль.bsl», «Модуль формы.bsl»).
Скрипт строит соответствие между двумя раскладками и сравнивает содержимое
побайтово — это проверка и полноты набора модулей, и корректности их текста.

Запуск:

    python3 test/compare-with-platform-dump.py <наше-дерево> <выгрузка-платформы> [--allow-missing-bom]

Код возврата 0 — все модули найдены и совпали, 1 — есть расхождения
(подробности печатаются), 2 — ошибка вызова.

--allow-missing-bom допускает отличие ровно в UTF-8 BOM в начале файла: так можно
проверять деревья, разложенные старыми версиями v8unpack (до 3.0.51).
"""
import os
import sys

BOM = b"\xef\xbb\xbf"

# Каталоги: как называет v8unpack -> как называет платформа.
TYPE_DIRS = {
    "Справочники": "Catalogs",
    "Документы": "Documents",
    "Журналы документов": "DocumentJournals",
    "Задачи": "Tasks",
    "Бизнес-процессы": "BusinessProcesses",
    "Обработки": "DataProcessors",
    "Отчеты": "Reports",
    "Перечисления": "Enums",
    "Константы": "Constants",
    "Планы видов расчета": "ChartsOfCalculationTypes",
    "Планы видов характеристик": "ChartsOfCharacteristicTypes",
    "Планы счетов": "ChartsOfAccounts",
    "Регистры сведений": "InformationRegisters",
    "Регистры накопления": "AccumulationRegisters",
    "Регистры расчета": "CalculationRegisters",
    "Регистры бухгалтерии": "AccountingRegisters",
}
# Подкаталоги внутри «Общие/».
COMMON_DIRS = {
    "Планы обмена": "ExchangePlans",
    "Критерии отбора": "FilterCriteria",
    "Хранилища настроек": "SettingsStorages",
    "Боты": "Bots",
    "HTTP-сервисы": "HTTPServices",
    "Web-сервисы": "WebServices",
    "WebSocket-клиенты": "WebSocketClients",
    "Сервисы интеграции": "IntegrationServices",
    "Общие модули": "CommonModules",
    "Общие команды": "CommonCommands",
    "Общие формы": "CommonForms",
}
# Имена модулей по роли: как называет v8unpack -> путь внутри объекта у платформы.
ROLE_MODULES = {
    "Модуль объекта.bsl": "Ext/ObjectModule.bsl",
    "Модуль менеджера.bsl": "Ext/ManagerModule.bsl",
    "Модуль набора записей.bsl": "Ext/RecordSetModule.bsl",
    "Модуль менеджера значения.bsl": "Ext/ValueManagerModule.bsl",
    "Модуль бота.bsl": "Ext/Module.bsl",
}
# Модули самой конфигурации.
CONFIG_MODULES = {
    "Модуль сеанса.bsl": "Ext/SessionModule.bsl",
    "Модуль обычного приложения.bsl": "Ext/OrdinaryApplicationModule.bsl",
    "Модуль управляемого приложения.bsl": "Ext/ManagedApplicationModule.bsl",
    "Модуль внешнего соединения.bsl": "Ext/ExternalConnectionModule.bsl",
}
# Типы, у которых модуль один и лежит прямо в каталоге объекта.
SINGLE_MODULE_DIRS = ("HTTPServices", "WebServices", "WebSocketClients",
                      "IntegrationServices", "CommonModules", "Bots")


def platform_path(rel):
    """Путь нашего файла -> путь того же модуля в выгрузке платформы (или None)."""
    parts = rel.split("/")
    if parts[0] == "Конфигурация":
        return CONFIG_MODULES.get(parts[1]) if len(parts) == 2 else None
    if parts[0] == "Общие":
        if len(parts) < 3 or parts[1] not in COMMON_DIRS:
            return None
        base, rest = COMMON_DIRS[parts[1]], parts[2:]
    else:
        if parts[0] not in TYPE_DIRS:
            return None
        base, rest = TYPE_DIRS[parts[0]], parts[1:]

    obj, tail = rest[0], rest[1:]
    if base == "CommonForms" and tail == ["Модуль формы.bsl"]:
        return "CommonForms/%s/Ext/Form/Module.bsl" % obj
    if len(tail) == 3 and tail[0] == "Формы" and tail[2] == "Модуль формы.bsl":
        return "%s/%s/Forms/%s/Ext/Form/Module.bsl" % (base, obj, tail[1])
    # Файл команды объекта: «Модуль команды.bsl» (с 3.0.51) или «<Команда>.bsl» (раньше).
    if len(tail) == 3 and tail[0] == "Команды" and tail[2] in (tail[1] + ".bsl", "Модуль команды.bsl"):
        return "%s/%s/Commands/%s/Ext/CommandModule.bsl" % (base, obj, tail[1])
    if len(tail) == 1:
        name = tail[0]
        if name in ROLE_MODULES:
            return "%s/%s/%s" % (base, obj, ROLE_MODULES[name])
        if name == "Модуль.bsl" and base in SINGLE_MODULE_DIRS:
            return "%s/%s/Ext/Module.bsl" % (base, obj)
        if name == "Модуль команды.bsl" and base == "CommonCommands":
            return "%s/%s/Ext/CommandModule.bsl" % (base, obj)
        # До 3.0.51 модуль назывался по имени объекта.
        if name == obj + ".bsl":
            if base in SINGLE_MODULE_DIRS:
                return "%s/%s/Ext/Module.bsl" % (base, obj)
            if base == "CommonCommands":
                return "%s/%s/Ext/CommandModule.bsl" % (base, obj)
    return None
def read(path):
    with open(path, "rb") as f:
        return f.read()


def main(argv):
    allow_missing_bom = "--allow-missing-bom" in argv
    args = [a for a in argv[1:] if not a.startswith("--")]
    if len(args) != 2:
        print(__doc__)
        return 2
    ours_root, plat_root = args
    for d in (ours_root, plat_root):
        if not os.path.isdir(d):
            print("нет каталога:", d)
            return 2

    pairs, unmapped = [], []
    for root, dirs, files in os.walk(ours_root):
        dirs[:] = [d for d in dirs if d != ".e8unpack"]
        for fn in files:
            if not fn.endswith(".bsl"):
                continue
            full = os.path.join(root, fn)
            rel = os.path.relpath(full, ours_root).replace(os.sep, "/")
            target = platform_path(rel)
            if target:
                pairs.append((rel, target, full))
            else:
                unmapped.append(rel)

    problems = []
    missing = [p for p in pairs if not os.path.isfile(os.path.join(plat_root, p[1]))]
    for rel, target, _ in missing:
        problems.append("нет файла платформы для %s (ожидался %s)" % (rel, target))

    same = bom_only = 0
    for rel, target, full in pairs:
        plat = os.path.join(plat_root, target)
        if not os.path.isfile(plat):
            continue
        a, b = read(full), read(plat)
        if a == b:
            same += 1
        elif allow_missing_bom and a == b[3:] and b.startswith(BOM) and not a.startswith(BOM):
            bom_only += 1
        else:
            problems.append("содержимое различается: %s  <->  %s (наш %d байт, платформы %d байт)"
                            % (rel, target, len(a), len(b)))

    for rel in unmapped:
        problems.append("не удалось сопоставить: %s" % rel)

    total = len(pairs) + len(unmapped)
    print("модулей в нашем дереве: %d, сопоставлено: %d" % (total, len(pairs)))
    print("идентичны побайтово: %d" % same)
    if allow_missing_bom:
        print("различаются только BOM (допущено опцией): %d" % bom_only)
    if problems:
        print("\nрасхождения (%d):" % len(problems))
        for p in problems[:40]:
            print("  ", p)
        if len(problems) > 40:
            print("   ... ещё %d" % (len(problems) - 40))
        return 1
    if len(pairs) == 0:
        print("не найдено ни одного модуля — проверьте пути")
        return 1
    print("\nВСЁ СОШЛОСЬ: все модули совпали с выгрузкой платформы")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
