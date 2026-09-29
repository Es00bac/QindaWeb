// SPDX-License-Identifier: GPL-3.0-or-later
#include "navigation.h"
#include <QFileInfo>
#include <QUrlQuery>
#include <QRegularExpression>
namespace QindaWeb {
QUrl navigationUrl(const QString &input, QString *error)
{
    const QString text = input.trimmed();
    if (text.isEmpty()) return QUrl(QStringLiteral("about:blank"));
    // A hostname followed by a port is an address, not an executable scheme.
    static const QRegularExpression hostPort(QStringLiteral("^(localhost|[A-Za-z0-9.-]+\\.[A-Za-z0-9.-]+):([0-9]{1,5})(/|$)"));
    if (hostPort.match(text).hasMatch()) {
        const QUrl address(QStringLiteral("https://") + text);
        if (address.isValid() && address.port() > 0 && address.port() <= 65535) return address;
    }
    const QUrl parsed(text);
    if (!parsed.scheme().isEmpty()) {
        const QString scheme = parsed.scheme().toLower();
        if (scheme == QStringLiteral("http") || scheme == QStringLiteral("https") ||
            scheme == QStringLiteral("file") || text == QStringLiteral("about:blank")) {
            if (parsed.isValid() && (scheme == QStringLiteral("file") ||
                text == QStringLiteral("about:blank") || !parsed.host().isEmpty())) return parsed;
        }
        if (error) *error = QStringLiteral("Use an HTTP, HTTPS or file address.");
        return {};
    }
    if (QFileInfo(text).isAbsolute() && QFileInfo(text).isFile())
        return QUrl::fromLocalFile(text);
    if (!text.contains(QChar::Space) &&
        (text.contains(QLatin1Char('.')) || text == QStringLiteral("localhost") ||
         text.startsWith(QStringLiteral("localhost:")))) {
        const QUrl address(QStringLiteral("https://") + text);
        if (address.isValid() && !address.host().isEmpty()) return address;
    }
    QUrl search(QStringLiteral("https://duckduckgo.com/"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("q"), text);
    search.setQuery(query);
    return search;
}
}
