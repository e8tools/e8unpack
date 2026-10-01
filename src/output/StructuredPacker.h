#ifndef STRUCTUREDPACKER_H
#define STRUCTUREDPACKER_H

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <QString>
#include <filesystem>
#include "src/output/UnpackManifest.h"
#include <QFileInfo>   

namespace v8unpack {

/**
 * Собирает .cf-контейнер из «человеческой» структуры каталогов
 * (той, что создаёт StructuredUnpacker).
 *
 * Схема:
 *   1. Обходим дерево и раскладываем все файлы в плоский временный
 *      каталог .tmp_pack/, где имена — GUID-ы или служебные (root, version).
 *   2. Вызываем v8unpack::BuildCfFile() — он собирает .cf из плоской папки.
 *   3. Удаляем временный каталог.
 */
class StructuredPacker {
public:
    /// @param inputDir  — каталог с человекочитаемой структурой
    /// @param outputFile — путь к создаваемому .cf
    /// @param noDeflate  — не сжимать данные
    StructuredPacker(const QString& inputDir,
                     const QString& outputFile,
                     bool noDeflate = false);

    bool run();

private:
    UnpackManifest m_manifest;    
    bool m_manifestLoaded = false; 

    bool loadManifest();
    bool packFromManifest();
    bool packFromDirectoryFallback();

    bool flattenToTemp();
    void cleanupTemp();

    QString m_inputDir;
    QString m_outputFile;
    QString m_tempDir;
    bool    m_noDeflate;
};

} // namespace v8

#endif // STRUCTUREDPACKER_H
