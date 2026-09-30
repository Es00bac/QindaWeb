// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <QGuiApplication>
#include <QWaylandCompositor>
#include <QWaylandSurface>
#include <QWaylandClient>
#include <QWaylandSeat>
#include <QWaylandKeyboard>
#include <QTemporaryDir>
#include <wayland-client.h>
#include <wayland-server-core.h>
#include <poll.h>
#include <unistd.h>
#include <memory>
#include <vector>

// Two listeners on ONE connection reproduce GTK plus Mozilla's seat tracker.
// Every assertion observes real wire events; no compositor-private API is used.
class WireClient {
public:
    struct Keyboard {
        wl_keyboard *handle = nullptr;
        int maps = 0, enters = 0, leaves = 0, presses = 0, releases = 0, modifiers = 0;
        quint32 lastKey = 0;
    };
    explicit WireClient(QWaylandCompositor &host, int initialKeyboards)
        : display(wl_display_connect(host.socketName().constData())), initial(initialKeyboards), server(host) {
        if (!display) return;
        registry = wl_display_get_registry(display);
        static const wl_registry_listener listener{
            [](void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
                auto &self = *static_cast<WireClient *>(data);
                if (QByteArray(interface) == QByteArrayLiteral("wl_compositor") && !self.compositor) {
                    self.compositor = static_cast<wl_compositor *>(wl_registry_bind(registry, name, &wl_compositor_interface, qMin(version, 3u)));
                    self.surface = wl_compositor_create_surface(self.compositor);
                }
                if (QByteArray(interface) == QByteArrayLiteral("wl_seat") && !self.seat) {
                    self.seat = static_cast<wl_seat *>(wl_registry_bind(registry, name, &wl_seat_interface, qMin(version, 4u)));
                    static const wl_seat_listener seatListener{
                        [](void *data, wl_seat *, uint32_t capabilities) {
                            auto &self = *static_cast<WireClient *>(data);
                            if (!(capabilities & WL_SEAT_CAPABILITY_KEYBOARD) || self.initialized) return;
                            self.initialized = true;
                            for (int i = 0; i < self.initial; ++i) self.addKeyboard();
                        }, [](void *, wl_seat *, const char *) {}};
                    wl_seat_add_listener(self.seat, &seatListener, &self);
                }
            }, [](void *, wl_registry *, uint32_t) {}};
        wl_registry_add_listener(registry, &listener, this);
    }
    ~WireClient() {
        if (!display) return;
        for (auto &keyboard : keyboards) if (keyboard->handle) wl_keyboard_release(keyboard->handle);
        if (surface) wl_surface_destroy(surface);
        if (seat) wl_seat_destroy(seat);
        if (compositor) wl_compositor_destroy(compositor);
        wl_registry_destroy(registry);
        wl_display_flush(display);
        wl_display_disconnect(display);
    }
    Keyboard *addKeyboard() {
        auto value = std::make_unique<Keyboard>();
        value->handle = wl_seat_get_keyboard(seat);
        static const wl_keyboard_listener listener{
            [](void *data, wl_keyboard *, uint32_t, int32_t fd, uint32_t) {
                ++static_cast<Keyboard *>(data)->maps; close(fd);
            },
            [](void *data, wl_keyboard *, uint32_t, wl_surface *, wl_array *) { ++static_cast<Keyboard *>(data)->enters; },
            [](void *data, wl_keyboard *, uint32_t, wl_surface *) { ++static_cast<Keyboard *>(data)->leaves; },
            [](void *data, wl_keyboard *, uint32_t, uint32_t, uint32_t key, uint32_t state) {
                auto &value = *static_cast<Keyboard *>(data);
                if (state == WL_KEYBOARD_KEY_STATE_PRESSED) ++value.presses; else ++value.releases;
                value.lastKey = key;
            },
            [](void *data, wl_keyboard *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) { ++static_cast<Keyboard *>(data)->modifiers; },
            [](void *, wl_keyboard *, int32_t, int32_t) {}};
        wl_keyboard_add_listener(value->handle, &listener, value.get());
        auto *result = value.get();
        keyboards.push_back(std::move(value));
        return result;
    }
    void release(Keyboard *keyboard) {
        wl_keyboard_release(keyboard->handle);
        keyboard->handle = nullptr;
        wl_display_flush(display);
    }
    void pump() {
        if (!display) return;
        wl_display_flush(display);
        // This wire client and server share a GUI thread. Flush explicitly;
        // QTest's bounded polling need not reach the dispatcher's idle flush.
        server.processWaylandEvents();
        wl_display_flush_clients(server.display());
        wl_display_dispatch_pending(display);
        pollfd fd{wl_display_get_fd(display), POLLIN, 0};
        if (::poll(&fd, 1, 0) > 0 && (fd.revents & POLLIN)) wl_display_dispatch(display);
    }
    wl_display *display = nullptr;
    wl_registry *registry = nullptr;
    wl_compositor *compositor = nullptr;
    wl_seat *seat = nullptr;
    wl_surface *surface = nullptr;
    std::vector<std::unique_ptr<Keyboard>> keyboards;
private:
    int initial;
    bool initialized = false;
    QWaylandCompositor &server;
};

class KeyboardTest : public QObject {
    Q_OBJECT
private slots:
    void everyBindingReceivesKeyboardState() {
        QTemporaryDir runtime;
        QVERIFY(runtime.isValid());
        QWaylandCompositor host;
        host.setUseHardwareIntegrationExtension(false);
        host.setSocketName(runtime.filePath(QStringLiteral("wayland")).toUtf8());
        QList<QWaylandSurface *> surfaces;
        connect(&host, &QWaylandCompositor::surfaceCreated, this, [&](QWaylandSurface *surface) { surfaces.append(surface); });
        host.create();
        QVERIFY(host.isCreated());
        WireClient first(host, 2);
        QVERIFY(first.display);
        const auto wait = [&](auto predicate) {
            return QTest::qWaitFor([&] { first.pump(); return predicate(); }, 3000);
        };
        QVERIFY(wait([&] { return surfaces.size() == 1 && first.keyboards.size() == 2 && first.keyboards[0]->maps && first.keyboards[1]->maps; }));
        QStringList bindings;
        wl_client_for_each_resource(surfaces[0]->client()->client(), [](wl_resource *resource, void *data) {
            if (QByteArray(wl_resource_get_class(resource)) == QByteArrayLiteral("wl_keyboard"))
                static_cast<QStringList *>(data)->append(QString::number(wl_resource_get_id(resource)));
            return WL_ITERATOR_CONTINUE;
        }, &bindings);
        QCOMPARE(bindings.size(), 2); // Explicit same-wl_client identity proof.
        auto *seat = host.defaultSeat();
        auto *keyboard = seat->keyboard();
        QVERIFY(seat->setKeyboardFocus(surfaces[0]));
        QVERIFY(wait([&] { return first.keyboards[0]->enters == 1 && first.keyboards[1]->enters == 1; }));
        keyboard->sendKeyPressEvent(54);
        keyboard->sendKeyReleaseEvent(54);
        QVERIFY(wait([&] { return first.keyboards[0]->presses == 1 && first.keyboards[1]->presses == 1 && first.keyboards[0]->releases == 1 && first.keyboards[1]->releases == 1; }));
        QCOMPARE(first.keyboards[0]->lastKey, 46u);
        QCOMPARE(first.keyboards[1]->lastKey, 46u);
        const int oldModifiers = first.keyboards[0]->modifiers;
        keyboard->sendKeyModifiers(surfaces[0]->client(), host.nextSerial());
        QVERIFY(wait([&] { return first.keyboards[0]->modifiers == oldModifiers + 1 && first.keyboards[1]->modifiers == oldModifiers + 1; }));
        auto *late = first.addKeyboard();
        QVERIFY(wait([&] { return late->maps == 1 && late->enters == 1; }));
        first.release(late);
        QTest::qWait(50);
        QCOMPARE(keyboard->focusClient(), surfaces[0]->client());
        keyboard->sendKeyPressEvent(54);
        QVERIFY(wait([&] { return first.keyboards[0]->presses == 2 && first.keyboards[1]->presses == 2; }));
        keyboard->sendKeyReleaseEvent(54);
        WireClient other(host, 1);
        QVERIFY(other.display);
        QVERIFY(QTest::qWaitFor([&] { first.pump(); other.pump(); return surfaces.size() == 2 && other.keyboards.size() == 1 && other.keyboards[0]->maps; }, 3000));
        QVERIFY(seat->setKeyboardFocus(surfaces[1]));
        QVERIFY(QTest::qWaitFor([&] { first.pump(); other.pump(); return first.keyboards[0]->leaves == 1 && first.keyboards[1]->leaves == 1 && other.keyboards[0]->enters == 1; }, 3000));
        keyboard->sendKeyPressEvent(54);
        QVERIFY(QTest::qWaitFor([&] { first.pump(); other.pump(); return other.keyboards[0]->presses == 1; }, 3000));
        QCOMPARE(first.keyboards[0]->presses, 2);
        QCOMPARE(first.keyboards[1]->presses, 2);
        keyboard->sendKeyReleaseEvent(54);
        other.release(other.keyboards[0].get());
        QTest::qWait(50);
        QVERIFY(!keyboard->focusClient());
        keyboard->sendKeyPressEvent(54); // Last binding released: no stale-resource access.
    }
};
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    KeyboardTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "tst_keyboard.moc"
