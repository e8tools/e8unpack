#ifndef V8UNPACKER_H
#define V8UNPACKER_H

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "V8Container.h"
#include <QString>

namespace v8unpack {

/**
 * Высокоуровневый фасад для распаковки контейнера.
 * В дальнейшем сюда можно добавить логику обхода вложенных контейнеров,
 * декодирования заголовков форм, извлечения модулей и т.д.
 */
class V8Unpacker {
public:
    explicit V8Unpacker(const QString &inputFile);
    bool unpackTo(const QString &outputDir);

private:
    QString m_inputFile;
};

} // namespace v8


#endif // V8UNPACKER_H
