// SPDX-License-Identifier: GPL-3.0-or-later
#include "browser_controller.h"
#include "browser_view.h"
#include "navigation.h"
#include "profile.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QCommandLineParser>
#include <QStandardPaths>
#include <QLockFile>
#include <QDir>
#include <cstdio>
int main(int argc, char **argv)
{
    using namespace QindaWeb;
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("qindaweb"));
    app.setApplicationDisplayName(QStringLiteral("QindaWeb"));
    app.setOrganizationName(QStringLiteral("QindaQt"));
    app.setApplicationVersion(QStringLiteral(QINDAWEB_VERSION));
    app.setDesktopFileName(QStringLiteral("org.qindaqt.QindaWeb"));
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Native QindaTK browser with Gecko content and desktop container placement"));
    parser.addHelpOption(); parser.addVersionOption();
    parser.addOption({QStringLiteral("firefox"), QStringLiteral("Firefox runtime executable (development override)"), QStringLiteral("path")});
    parser.addOption({QStringLiteral("profile"), QStringLiteral("QindaWeb-owned runtime profile directory"), QStringLiteral("directory")});
    parser.addPositionalArgument(QStringLiteral("urls"), QStringLiteral("Addresses or search terms"), QStringLiteral("[urls…]"));
    parser.process(app);
    const QString profile = parser.isSet(QStringLiteral("profile")) ? parser.value(QStringLiteral("profile")) :
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/firefox");
    QDir().mkpath(profile);
    QLockFile lock(profile + QStringLiteral("/qindaweb.lock"));
    if (!lock.tryLock(0)) {
        std::fprintf(stderr, "qindaweb: this QindaWeb profile is already in use\n");
        return 2;
    }
    qmlRegisterUncreatableType<BrowserView>("QindaWeb", 1, 0, "BrowserView", QStringLiteral("Owned by the browser"));
    qmlRegisterUncreatableType<BrowserController>("QindaWeb", 1, 0, "BrowserController", QStringLiteral("Owned by the application"));
    QQmlApplicationEngine qml;
    BrowserController browser(&qml);
    QString error;
    if (!browser.start(firefoxExecutable(parser.value(QStringLiteral("firefox"))), profile, &error)) {
        std::fprintf(stderr, "qindaweb: %s\n", qPrintable(error));
        return 1;
    }
    const QStringList inputs = parser.positionalArguments().isEmpty() ? QStringList{QStringLiteral("about:blank")} : parser.positionalArguments();
    for (const auto &input : inputs) {
        const QUrl url = navigationUrl(input, &error);
        if (url.isEmpty()) { std::fprintf(stderr, "qindaweb: %s\n", qPrintable(error)); continue; }
        if (!browser.newView(nullptr, {}, url)) return 1;
    }
    if (browser.views().isEmpty()) return 2;
    return app.exec();
}
