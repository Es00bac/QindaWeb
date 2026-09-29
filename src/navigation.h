// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QUrl>
namespace QindaWeb {
// User input becomes an explicit URL or an encoded search. Runtime chrome,
// script and data schemes are never handed to Firefox by the address field.
QUrl navigationUrl(const QString &input, QString *error = nullptr);
}
