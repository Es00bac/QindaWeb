// SPDX-License-Identifier: GPL-3.0-or-later
#include "browser_controller.h"
#include "browser_view.h"
#include "profile.h"
#include <QtTest>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QWaylandQuickShellSurfaceItem>
#include <QWaylandSurface>
#include <QTemporaryDir>
#include <QDir>
class BrowserTest : public QObject {
    Q_OBJECT
private:
    QString evaluate(QindaWeb::BrowserController &browser, QindaWeb::BrowserView *view, const QString &expression) {
        bool done = false;
        QString value;
        browser.engine()->control()->request(QStringLiteral("script.evaluate"),
            {{QStringLiteral("expression"), expression}, {QStringLiteral("awaitPromise"), false},
             {QStringLiteral("target"), QJsonObject{{QStringLiteral("context"), view->contextId()}}}},
            [&](const QJsonObject &result, const QString &error) {
                value = error.isEmpty() ? result.value(QStringLiteral("result")).toObject().value(QStringLiteral("value")).toString() : error;
                done = true;
            });
        QTest::qWaitFor([&] { return done; }, 16000);
        return value;
    }
private slots:
    void realGeckoAndNativeWindows() {
        QTemporaryDir profile;
        QVERIFY(profile.isValid());
        QQmlApplicationEngine qml;
        QindaWeb::BrowserController browser(&qml);
        QString error;
        QVERIFY2(browser.start(QindaWeb::firefoxExecutable(), profile.path(), &error), qPrintable(error));
        auto *view = browser.newView(nullptr, {}, QUrl::fromLocalFile(QStringLiteral(QINDAWEB_FIXTURE)));
        QVERIFY(view);
        QTRY_VERIFY_WITH_TIMEOUT(view->ready(), 35000);
        QTRY_VERIFY_WITH_TIMEOUT(view->shellSurface() && view->shellSurface()->surface()->hasContent(), 35000);
        QTRY_VERIFY_WITH_TIMEOUT(!view->loading(), 35000);
        QVERIFY2(browser.engineIdentity().startsWith(QStringLiteral("Gecko / Firefox ")), qPrintable(browser.engineIdentity()));
        QCOMPARE(evaluate(browser, view, QStringLiteral("document.title")), QStringLiteral("QindaWeb Gecko acceptance"));
        QVERIFY(evaluate(browser, view, QStringLiteral("navigator.userAgent")).contains(QStringLiteral("Gecko/")));
        auto *page = view->window()->findChild<QWaylandQuickShellSurfaceItem *>(QStringLiteral("geckoContent"));
        QVERIFY(page);
        QTest::qWait(700);
        QDir().mkpath(QStringLiteral("build/acceptance"));
        const auto pixels = view->window()->grabWindow();
        QVERIFY(!pixels.isNull());
        QVERIFY(pixels.save(QStringLiteral("build/acceptance/gecko-initial.png")));
        page->takeFocus();
        QTest::mouseClick(view->window(), Qt::LeftButton, {}, page->mapToScene(QPointF(70, 44)).toPoint());
        for (Qt::Key key : {Qt::Key_G, Qt::Key_E, Qt::Key_C, Qt::Key_K, Qt::Key_O})
            QTest::keyClick(view->window(), key);
        QTest::qWait(250);
        QCOMPARE(evaluate(browser, view, QStringLiteral("document.querySelector('#entry').value")), QStringLiteral("gecko"));
        QTest::keyClick(view->window(), Qt::Key_L, Qt::ControlModifier);
        QTRY_VERIFY(view->window()->activeFocusItem() && view->window()->activeFocusItem()->objectName() == QStringLiteral("browserAddress"));
        QTest::keyClick(view->window(), Qt::Key_T, Qt::ControlModifier);
        QTRY_COMPARE(browser.views().size(), 2);
        QindaWeb::BrowserView *second = nullptr;
        for (auto *candidate : browser.views()) if (candidate != view) second = candidate;
        QVERIFY(second);
        QTRY_VERIFY_WITH_TIMEOUT(second->ready() && second->shellSurface() && second->shellSurface()->surface()->hasContent(), 20000);
        QVERIFY(second->id() != view->id());
        QVERIFY(second->window() != view->window());
        browser.closeView(second);
        QTRY_COMPARE(browser.views().size(), 1);
        view->window()->requestActivate();
        QTest::keyClick(view->window(), Qt::Key_T, Qt::ControlModifier | Qt::AltModifier);
        QTRY_COMPARE(browser.views().size(), 2);
        auto *chooser = view->window()->findChild<QObject *>(QStringLiteral("browserPlacementChooser"));
        QVERIFY(chooser);
        QTRY_VERIFY(chooser->property("opened").toBool());
        QTest::keyClick(view->window(), Qt::Key_Right);
        QTRY_VERIFY(!chooser->property("opened").toBool());
        QVERIFY(view->window()->grabWindow().save(QStringLiteral("build/acceptance/gecko-final.png")));
        qInfo().noquote() << "Verified" << browser.engineIdentity() << "rendered pixels, pointer/key input, Ctrl+L/T, split picker, distinct window/view IDs";
    }
};
int main(int argc, char **argv) {
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("qindaweb-native-test"));
    qmlRegisterUncreatableType<QindaWeb::BrowserView>("QindaWeb", 1, 0, "BrowserView", QStringLiteral("Browser-owned"));
    qmlRegisterUncreatableType<QindaWeb::BrowserController>("QindaWeb", 1, 0, "BrowserController", QStringLiteral("App-owned"));
    BrowserTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "tst_browser.moc"
