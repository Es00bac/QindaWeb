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
#include <QWaylandSeat>
#include <QTemporaryDir>
#include <QDir>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QJsonDocument>
#include <QFile>
#include <qindaqt/shell/global_menu/dbusmenu/dbusmenu_wire.h>
namespace MenuWire = QindaQt::Shell::GlobalMenu::DbusMenu;
class KeyTrace : public QObject {
public:
    QStringList events;
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (event->type() == QEvent::KeyPress) {
            const auto *key = static_cast<QKeyEvent *>(event);
            events.append(QStringLiteral("%1:%2/%3/%4").arg(watched->objectName()).arg(key->key()).arg(key->nativeScanCode()).arg(key->nativeVirtualKey()));
        }
        return false;
    }
};
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
        if (!QTest::qWaitFor([&] { return done; }, 16000)) return QStringLiteral("Evaluation timed out");
        return value;
    }
    QDBusMessage menuCall(QindaWeb::BrowserView *view, const QString &method, const QVariantList &arguments) {
        // A distinct connection exercises the published D-Bus wire endpoint.
        auto bus = QDBusConnection::connectToBus(QDBusConnection::SessionBus, QStringLiteral("qindaweb-menu-test"));
        auto request = QDBusMessage::createMethodCall(QDBusConnection::sessionBus().baseService(),
            view->globalMenuObjectPath(), QStringLiteral("com.canonical.dbusmenu"), method);
        request.setArguments(arguments);
        QDBusPendingCallWatcher pending(bus.asyncCall(request, 5000));
        if (!QTest::qWaitFor([&] { return pending.isFinished(); }, 6000)) return {};
        return pending.reply();
    }
    QList<MenuWire::LayoutItem> flatten(const MenuWire::LayoutItem &root) {
        QList<MenuWire::LayoutItem> items{root};
        for (const auto &child : root.children) {
            const QVariant node = child.metaType() == QMetaType::fromType<QDBusVariant>()
                ? child.value<QDBusVariant>().variant() : child;
            items.append(flatten(qdbus_cast<MenuWire::LayoutItem>(node)));
        }
        return items;
    }
private slots:
    void existingProfilesArePreserved() {
        QTemporaryDir profile;
        QFile existing(profile.filePath(QStringLiteral("user.js")));
        QVERIFY(existing.open(QIODevice::WriteOnly));
        const QByteArray original("// Existing user's Firefox settings\n");
        QCOMPARE(existing.write(original), original.size());
        existing.close();
        QString error;
        QVERIFY(!QindaWeb::prepareProfile(profile.path(), &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(existing.open(QIODevice::ReadOnly));
        QCOMPARE(existing.readAll(), original);
        QTemporaryDir owned;
        QVERIFY2(QindaWeb::prepareProfile(owned.path(), &error), qPrintable(error));
        QVERIFY2(QindaWeb::prepareProfile(owned.path(), &error), qPrintable(error));
    }
    void realGeckoAndNativeWindows() {
        QTemporaryDir profile;
        QVERIFY(profile.isValid());
        QQmlApplicationEngine qml;
        QindaWeb::BrowserController browser(&qml);
        connect(browser.engine(), &QindaWeb::FirefoxEngine::failed, this, [](const QString &message) {
            qWarning().noquote() << "Gecko startup/control failure:" << message;
        });
        QString error;
        QVERIFY2(browser.start(QindaWeb::firefoxExecutable(), profile.path(), &error), qPrintable(error));
        auto *view = browser.newView(nullptr, {}, QUrl::fromLocalFile(QStringLiteral(QINDAWEB_FIXTURE)));
        QVERIFY(view);
        QVERIFY2(QTest::qWaitFor([&] { return view->ready() || !view->loading(); }, 35000) && view->ready(), qPrintable(view->message()));
        QTRY_VERIFY_WITH_TIMEOUT(view->shellSurface() && view->shellSurface()->surface()->hasContent(), 35000);
        QTRY_VERIFY_WITH_TIMEOUT(!view->loading(), 35000);
        QVERIFY2(browser.engineIdentity().startsWith(QStringLiteral("Gecko / Firefox ")), qPrintable(browser.engineIdentity()));
        QCOMPARE(evaluate(browser, view, QStringLiteral("document.title")), QStringLiteral("QindaWeb Gecko acceptance"));
        QVERIFY(evaluate(browser, view, QStringLiteral("navigator.userAgent")).contains(QStringLiteral("Gecko/")));
        QCOMPARE(browser.views().size(), 1);
        auto *page = view->window()->findChild<QWaylandQuickShellSurfaceItem *>(QStringLiteral("geckoContent"));
        QVERIFY(page);
        QTest::qWait(700);
        QDir().mkpath(QStringLiteral("build/acceptance"));
        const auto pixels = view->window()->grabWindow();
        QVERIFY(!pixels.isNull());
        QVERIFY(pixels.save(QStringLiteral("build/acceptance/gecko-initial.png")));
        page->takeFocus();
        qInfo() << "Native item geometry" << page->size() << page->position() << page->surface()->destinationSize()
                << "window" << view->window()->size() << "DPR" << view->window()->devicePixelRatio();
        qInfo() << "Popup" << view->window()->findChild<QObject *>(QStringLiteral("browserPlacementChooser"))->property("visible")
                << "focus" << view->window()->activeFocusItem();
        QTest::mouseClick(view->window(), Qt::LeftButton, {}, page->mapToScene(page->mapFromSurface(QPointF(70, 44))).toPoint());
        QTest::qWait(200);
        qInfo().noquote() << "Fixture pointer trace" << evaluate(browser, view, QStringLiteral("JSON.stringify(window.testEvents)"));
        const bool nativeInput = qEnvironmentVariableIsSet("QINDAWEB_NATIVE_PLACEMENT");
        KeyTrace keys;
        view->window()->installEventFilter(&keys);
        page->installEventFilter(&keys);
        qInfo().noquote() << "Fixture document focus" << evaluate(browser, view, QStringLiteral("JSON.stringify({focused:document.hasFocus(),active:document.activeElement.id})"));
        qInfo() << "Fixture keyboard focus" << view->window()->isActive() << page->hasActiveFocus()
                << (page->surface()->compositor()->defaultSeat()->keyboardFocus() == page->surface());
        if (nativeInput) qInfo().noquote() << "QINDAWEB_FIXTURE_INPUT_READY";
        else for (Qt::Key key : {Qt::Key_G, Qt::Key_E, Qt::Key_C, Qt::Key_K, Qt::Key_O})
            QTest::keyClick(view->window(), key);
        // The native runner uses the compositor's existing C/N/V allowlist
        // through its normal keyboard chain, never host uinput or BiDi typing.
        const QString expectedInput = nativeInput ? QStringLiteral("cnv") : QStringLiteral("gecko");
        const bool typed = QTest::qWaitFor([&] { return evaluate(browser, view, QStringLiteral("document.querySelector('#entry').value")) == expectedInput; }, 5000);
        qInfo() << "Fixture native key trace" << keys.events;
        qInfo().noquote() << "Fixture document key trace" << evaluate(browser, view, QStringLiteral("JSON.stringify(window.testEvents)"));
        QVERIFY2(typed, "Gecko did not receive the expected fixture text");
        QTest::keyClick(view->window(), Qt::Key_H, Qt::ControlModifier | Qt::ShiftModifier);
        QTRY_VERIFY(!view->chromeVisible());
        auto *content = view->window()->findChild<QQuickItem *>(QStringLiteral("browserContent"));
        QVERIFY(content);
        QTRY_COMPARE(qRound(content->height()), view->window()->height());
        QTRY_COMPARE(qRound(page->height()), view->window()->height());
        QTRY_VERIFY_WITH_TIMEOUT([&] {
            const auto image = view->window()->grabWindow();
            return !image.isNull() && image.pixelColor(image.width() - 8, image.height() - 8) == QColor(QStringLiteral("#ecf6ff"));
        }(), 5000);
        QVERIFY(view->window()->grabWindow().save(QStringLiteral("build/acceptance/gecko-hidden.png")));
        const auto layoutReply = menuCall(view, QStringLiteral("GetLayout"), {0, -1, QStringList{}});
        QVERIFY2(layoutReply.type() == QDBusMessage::ReplyMessage, qPrintable(layoutReply.errorMessage()));
        QCOMPARE(layoutReply.arguments().size(), 2);
        const auto items = flatten(qdbus_cast<MenuWire::LayoutItem>(layoutReply.arguments().at(1)));
        QStringList labels;
        qint32 chromeAction = -1;
        for (const auto &item : items) {
            const QString label = item.properties.value(QStringLiteral("label")).toString();
            labels.append(label);
            if (label == QStringLiteral("Show browser controls")) {
                chromeAction = item.id;
                QCOMPARE(item.properties.value(QStringLiteral("toggle-state")).toInt(), 0);
            }
        }
        QVERIFY(labels.contains(QStringLiteral("File")));
        QVERIFY(labels.contains(QStringLiteral("Navigate")));
        QVERIFY(labels.contains(QStringLiteral("View")));
        QVERIFY(chromeAction > 0);
        QTest::keyClick(view->window(), Qt::Key_L, Qt::ControlModifier);
        QTRY_VERIFY(view->window()->activeFocusItem() && view->window()->activeFocusItem()->objectName() == QStringLiteral("browserAddress"));
        QVERIFY(!view->chromeVisible());
        QTRY_VERIFY(content->height() < view->window()->height());
        QTest::keyClick(view->window(), Qt::Key_Escape);
        QTRY_COMPARE(qRound(content->height()), view->window()->height());
        const auto eventReply = menuCall(view, QStringLiteral("Event"),
            {chromeAction, QStringLiteral("clicked"), QVariant::fromValue(QDBusVariant(0)), quint32(0)});
        QVERIFY2(eventReply.type() == QDBusMessage::ReplyMessage, qPrintable(eventReply.errorMessage()));
        QTRY_VERIFY(view->chromeVisible());
        qInfo() << "Native global-menu protocol detected:" << view->globalMenuAvailable();
        qInfo() << "Desktop theme published:" << browser.desktopThemeReady() << browser.desktopThemeError();
        page->takeFocus();
        const auto button = QJsonDocument::fromJson(evaluate(browser, view,
            QStringLiteral("JSON.stringify((() => { const r = document.querySelector('#popup').getBoundingClientRect(); return {x:r.x+r.width/2,y:r.y+r.height/2}; })())")).toUtf8()).object();
        QVERIFY(!button.isEmpty());
        QTest::mouseClick(view->window(), Qt::LeftButton, {},
            page->mapToScene(page->mapFromSurface(QPointF(button.value(QStringLiteral("x")).toDouble(), button.value(QStringLiteral("y")).toDouble()))).toPoint());
        qInfo().noquote() << "Fixture popup pointer trace" << evaluate(browser, view, QStringLiteral("JSON.stringify(window.testEvents)"));
        QTRY_COMPARE_WITH_TIMEOUT(browser.views().size(), 2, 20000);
        QindaWeb::BrowserView *popup = nullptr;
        for (auto *candidate : browser.views()) if (candidate != view) popup = candidate;
        QVERIFY(popup);
        QTRY_VERIFY_WITH_TIMEOUT(popup->ready(), 20000);
        QCOMPARE(evaluate(browser, popup, QStringLiteral("document.title")), QStringLiteral("QindaWeb Gecko acceptance"));
        QCOMPARE(evaluate(browser, popup, QStringLiteral("location.search")), QStringLiteral("?popup=1"));
        (void)evaluate(browser, popup, QStringLiteral("window.close(); 'closed'"));
        QTRY_COMPARE_WITH_TIMEOUT(browser.views().size(), 1, 10000);
        view->window()->requestActivate();
        page->takeFocus();
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
        QTest::keyClick(view->window(), Qt::Key_Return, Qt::ControlModifier | Qt::ShiftModifier);
        QTRY_COMPARE(browser.views().size(), 2);
        auto *chooser = view->window()->findChild<QObject *>(QStringLiteral("browserPlacementChooser"));
        QVERIFY(chooser);
        QTRY_VERIFY(chooser->property("opened").toBool());
        QTRY_VERIFY(view->window()->activeFocusItem() && view->window()->activeFocusItem()->objectName() == QStringLiteral("browserSplitChoices"));
        QTest::keyClick(view->window(), Qt::Key_Right);
        QTRY_VERIFY(!chooser->property("opened").toBool());
        QVERIFY(view->window()->grabWindow().save(QStringLiteral("build/acceptance/gecko-final.png")));
        qInfo().noquote() << "Verified" << browser.engineIdentity() << "rendered pixels, pointer/key input, hidden chrome, temporary address, D-Bus menu activation, Ctrl+L/T, split picker, distinct window/view IDs";
        QDBusConnection::disconnectFromBus(QStringLiteral("qindaweb-menu-test"));
    }
    void nativeContainerPlacement() {
        if (!qEnvironmentVariableIsSet("QINDAWEB_NATIVE_PLACEMENT"))
            QSKIP("Run in the agreed private compositor fixture for actual placement acceptance.");
        QTemporaryDir profile;
        QQmlApplicationEngine qml;
        QindaWeb::BrowserController browser(&qml);
        QSignalSpy placement(&browser, &QindaWeb::BrowserController::placementFinished);
        QString error;
        QVERIFY2(browser.start(QindaWeb::firefoxExecutable(), profile.path(), &error), qPrintable(error));
        auto *source = browser.newView(nullptr, {}, QUrl::fromLocalFile(QStringLiteral(QINDAWEB_FIXTURE)));
        QVERIFY(source);
        QTRY_VERIFY_WITH_TIMEOUT(source->ready() && !source->loading(), 35000);
        QTRY_VERIFY(browser.placementAvailable());
        QTRY_VERIFY(source->globalMenuAvailable());
        QCOMPARE(evaluate(browser, source, QStringLiteral("document.title = 'Native QindaWeb source'; 'named'")), QStringLiteral("named"));
        QTRY_COMPARE(source->title(), QStringLiteral("Native QindaWeb source"));
        source->window()->requestActivate();
        QTRY_VERIFY(source->window()->isActive());
        QTest::keyClick(source->window(), Qt::Key_T, Qt::ControlModifier);
        QTRY_COMPARE(browser.views().size(), 2);
        QindaWeb::BrowserView *tab = nullptr;
        for (auto *candidate : browser.views()) if (candidate != source) tab = candidate;
        QVERIFY(tab);
        qInfo() << "Native window roles/states" << source->window()->flags() << source->window()->windowStates()
                << tab->window()->flags() << tab->window()->windowStates();
        QTRY_COMPARE_WITH_TIMEOUT(placement.size(), 1, 10000);
        QCOMPARE(placement.at(0).at(0).toString(), tab->id());
        QVERIFY2(placement.at(0).at(1).toBool(), qPrintable(placement.at(0).at(3).toString()));
        const QString container = placement.at(0).at(2).toString();
        QVERIFY(!container.isEmpty());
        QTRY_VERIFY_WITH_TIMEOUT(tab->ready() && tab->shellSurface() && tab->shellSurface()->surface()->hasContent(), 20000);
        QTRY_VERIFY(!tab->loading());
        QCOMPARE(evaluate(browser, tab, QStringLiteral("document.title = 'Native QindaWeb tab'; 'named'")), QStringLiteral("named"));
        QTRY_COMPARE(tab->title(), QStringLiteral("Native QindaWeb tab"));
        qInfo().noquote() << "QINDAWEB_NATIVE_TAB_READY" << container;
        QTest::qWait(1000); // Retain live windows while the driver captures scene evidence.
        tab->window()->requestActivate();
        QTRY_VERIFY(tab->window()->isActive());
        qInfo().noquote() << "QINDAWEB_NATIVE_SPLIT_PENDING";
        QTest::keyClick(tab->window(), Qt::Key_Return, Qt::ControlModifier | Qt::ShiftModifier);
        QTRY_COMPARE(browser.views().size(), 3);
        auto *chooser = tab->window()->findChild<QObject *>(QStringLiteral("browserPlacementChooser"));
        QTRY_VERIFY(chooser->property("opened").toBool());
        QTest::keyClick(tab->window(), Qt::Key_Right);
        QTRY_COMPARE_WITH_TIMEOUT(placement.size(), 2, 10000);
        QVERIFY2(placement.at(1).at(1).toBool(), qPrintable(placement.at(1).at(3).toString()));
        QCOMPARE(placement.at(1).at(2).toString(), container);
        QindaWeb::BrowserView *split = nullptr;
        for (auto *candidate : browser.views()) if (candidate != source && candidate != tab) split = candidate;
        QVERIFY(split);
        QCOMPARE(placement.at(1).at(0).toString(), split->id());
        QTRY_VERIFY_WITH_TIMEOUT(split->ready() && split->shellSurface() && split->shellSurface()->surface()->hasContent(), 20000);
        QTRY_VERIFY(!split->loading());
        QCOMPARE(evaluate(browser, split, QStringLiteral("document.title = 'Native QindaWeb split'; 'named'")), QStringLiteral("named"));
        QTRY_COMPARE(split->title(), QStringLiteral("Native QindaWeb split"));
        qInfo().noquote() << "QINDAWEB_NATIVE_SPLIT_READY" << container;
        QTest::qWait(1000);
        split->window()->requestActivate();
        QTRY_VERIFY(split->window()->isActive());
        QTest::keyClick(split->window(), Qt::Key_H, Qt::ControlModifier | Qt::ShiftModifier);
        QTRY_VERIFY(!split->chromeVisible());
        auto *content = split->window()->findChild<QQuickItem *>(QStringLiteral("browserContent"));
        QVERIFY(content);
        QTRY_COMPARE(qRound(content->height()), split->window()->height());
        const auto menu = menuCall(split, QStringLiteral("GetLayout"), {0, -1, QStringList{}});
        QVERIFY2(menu.type() == QDBusMessage::ReplyMessage, qPrintable(menu.errorMessage()));
        QDBusConnection::disconnectFromBus(QStringLiteral("qindaweb-menu-test"));
        qInfo().noquote() << "QINDAWEB_NATIVE_GROUP_TAB_PENDING";
        QTest::keyClick(split->window(), Qt::Key_T, Qt::ControlModifier);
        QTRY_COMPARE(browser.views().size(), 4);
        QTRY_COMPARE_WITH_TIMEOUT(placement.size(), 3, 10000);
        QVERIFY2(placement.at(2).at(1).toBool(), qPrintable(placement.at(2).at(3).toString()));
        QCOMPARE(placement.at(2).at(2).toString(), container);
        QindaWeb::BrowserView *grouped = nullptr;
        for (auto *candidate : browser.views()) if (candidate != source && candidate != tab && candidate != split) grouped = candidate;
        QVERIFY(grouped);
        QVERIFY(!grouped->chromeVisible());
        QCOMPARE(placement.at(2).at(0).toString(), grouped->id());
        QTRY_VERIFY_WITH_TIMEOUT(grouped->ready() && !grouped->loading(), 20000);
        QCOMPARE(evaluate(browser, grouped, QStringLiteral("document.title = 'Native QindaWeb grouped tab'; 'named'")), QStringLiteral("named"));
        QTRY_COMPARE(grouped->title(), QStringLiteral("Native QindaWeb grouped tab"));
        qInfo().noquote() << "QINDAWEB_NATIVE_GROUP_TAB_READY" << container;
        QTest::qWait(1000);
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
