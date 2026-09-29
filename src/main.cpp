// SPDX-License-Identifier: GPL-3.0-or-later
#include "browser_controller.h"
#include "browser_view.h"
#include "navigation.h"
#include "profile.h"
#include "application_instance.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QCommandLineParser>
#include <QStandardPaths>
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
    parser.addOption({QStringLiteral("hide-controls"), QStringLiteral("Start new pages with browser controls hidden")});
    parser.addPositionalArgument(QStringLiteral("urls"), QStringLiteral("Addresses or search terms"), QStringLiteral("[urls…]"));
    parser.process(app);
    const QString profile = parser.isSet(QStringLiteral("profile")) ? parser.value(QStringLiteral("profile")) :
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/firefox");
    if (!QDir().mkpath(profile)) { std::fprintf(stderr, "qindaweb: cannot create the profile directory\n"); return 1; }
    const QStringList inputs = parser.positionalArguments().isEmpty() ? QStringList{QStringLiteral("about:blank")} : parser.positionalArguments();
    const bool hideControls = parser.isSet(QStringLiteral("hide-controls"));
    QString error;
    ApplicationInstance instance(profile);
    const auto instanceResult = instance.start(inputs, hideControls, &error);
    if (instanceResult == ApplicationInstance::Result::Forwarded) return 0;
    if (instanceResult == ApplicationInstance::Result::Failed) {
        std::fprintf(stderr, "qindaweb: %s\n", qPrintable(error));
        return 2;
    }
    qmlRegisterUncreatableType<BrowserView>("QindaWeb", 1, 0, "BrowserView", QStringLiteral("Owned by the browser"));
    qmlRegisterUncreatableType<BrowserController>("QindaWeb", 1, 0, "BrowserController", QStringLiteral("Owned by the application"));
    QQmlApplicationEngine qml;
    BrowserController browser(&qml);
    auto openUrls = [&](const QStringList &urls, bool hidden) {
        for (const auto &input : urls) {
            QString failure;
            const QUrl url = navigationUrl(input, &failure);
            if (url.isEmpty()) { std::fprintf(stderr, "qindaweb: %s\n", qPrintable(failure)); continue; }
            if (auto *view = browser.newView(nullptr, {}, url)) view->setChromeVisible(!hidden);
        }
    };
    QObject::connect(&instance, &ApplicationInstance::openRequested, &browser, openUrls);
    if (!browser.start(firefoxExecutable(parser.value(QStringLiteral("firefox"))), profile, &error)) {
        std::fprintf(stderr, "qindaweb: %s\n", qPrintable(error));
        return 1;
    }
    openUrls(inputs, hideControls);
    instance.setReady();
    if (browser.views().isEmpty()) return 2;
    return app.exec();
}
