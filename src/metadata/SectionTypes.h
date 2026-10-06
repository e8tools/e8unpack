#ifndef SECTIONTYPES_H
#define SECTIONTYPES_H

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <QString>
#include <QHash>

namespace v8unpack {

// Человекочитаемые имена секций внутри объектов
constexpr auto sec_Forms            = "Формы";
constexpr auto sec_Attributes       = "Реквизиты";
constexpr auto sec_TabularSections  = "ТабличныеЧасти";
constexpr auto sec_Commands         = "Команды";
constexpr auto sec_Templates        = "Макеты";

// GUID-ы секций внутри объектов метаданных
constexpr auto GUID_Section_Forms           = "fdf816d2-1ead-11d5-b975-0050bae0a95d";
constexpr auto GUID_Section_Templates       = "3daea016-69b7-4ed4-9453-127911372fe6";  // Макеты
constexpr auto GUID_Section_Commands        = "4fe87c89-9ad4-43f6-9fdb-9dc83b3879c6";  // Команды
constexpr auto GUID_Section_TabularSections = "932159f9-95b2-4e76-a8dd-8849fe5c5ded";  // Табличные
//constexpr auto GUID_Section_Attributes      = "3daea016-69b7-4ed4-9453-127911372fe6";
//constexpr auto GUID_Section_TabularSections = "932159f9-95b2-4e76-a8dd-8849fe5c5ded";
// (дополняйте по мере обнаружения других секций)

/// Справочник GUID-ов секций → человекочитаемое имя.
const QHash<QString, QString>& sectionTypes();

} // namespace v8

#endif // SECTIONTYPES_H