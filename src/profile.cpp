// SPDX-License-Identifier: GPL-3.0-or-later
#include "profile.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
namespace QindaWeb {
static bool writeOwnedFile(const QString &path, const QByteArray &bytes, QString *error)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        if (error) *error = QStringLiteral("Cannot write QindaWeb profile: ") + file.errorString();
        return false;
    }
    return true;
}
bool prepareProfile(const QString &directory, QString *error)
{
    if (!QDir().mkpath(directory + QStringLiteral("/chrome"))) {
        if (error) *error = QStringLiteral("Cannot create the QindaWeb Firefox profile.");
        return false;
    }
    QFile css(QStringLiteral(":/data/userChrome.css"));
    if (!css.open(QIODevice::ReadOnly)) {
        if (error) *error = QStringLiteral("The browser chrome resource is missing.");
        return false;
    }
    // Do not disable sandboxing, certificate verification, site isolation,
    // accessibility or web security. Avoid automation's recommended test prefs.
    const QByteArray prefs = R"(// Managed by QindaWeb; this is its own profile.
user_pref("toolkit.legacyUserProfileCustomizations.stylesheets", true);
user_pref("remote.prefs.recommended", false);
user_pref("browser.startup.page", 0);
user_pref("browser.startup.homepage", "about:blank");
user_pref("browser.sessionstore.resume_from_crash", false);
user_pref("browser.shell.checkDefaultBrowser", false);
user_pref("browser.aboutwelcome.enabled", false);
user_pref("browser.startup.homepage_override.mstone", "ignore");
user_pref("browser.tabs.drawInTitlebar", false);
user_pref("browser.tabs.inTitlebar", 0);
user_pref("browser.link.open_newwindow", 2);
user_pref("browser.link.open_newwindow.restriction", 0);
)";
    return writeOwnedFile(directory + QStringLiteral("/user.js"), prefs, error) &&
           writeOwnedFile(directory + QStringLiteral("/chrome/userChrome.css"), css.readAll(), error);
}
QString firefoxExecutable(const QString &overridePath)
{
    if (!overridePath.isEmpty()) {
        const QFileInfo file(overridePath);
        return file.isFile() && file.isExecutable() ? file.absoluteFilePath() : QString();
    }
    for (const QString &path : {QStringLiteral("/opt/qindafox/firefox"),
                               QStandardPaths::findExecutable(QStringLiteral("firefox")),
                               QStandardPaths::findExecutable(QStringLiteral("firefox-bin"))})
        if (QFileInfo(path).isFile() && QFileInfo(path).isExecutable()) return path;
    return {};
}
}
