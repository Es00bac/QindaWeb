// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QString>
namespace QindaWeb {
bool prepareProfile(const QString &directory, QString *error);
QString firefoxExecutable(const QString &overridePath = {});
}
