// SPDX-License-Identifier: GPL-3.0-or-later
#include "global_menu_export.h"
#include "global_menu_source.h"
#include "browser_view.h"
#include <qindaqt/shell/global_menu/exporter/menu_exporter.h>
#include <qindaqt/shell/global_menu/dbusmenu/dbusmenu_server.h>
#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/registry.h>
#include <KWayland/Client/appmenu.h>
#include <KWayland/Client/surface.h>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QGuiApplication>
#include <QPointer>
#include <QTimer>
#include <QWindow>
#include <limits>
namespace QindaWeb {
namespace Exporter = QindaQt::Shell::GlobalMenu::Exporter;
namespace DbusMenu = QindaQt::Shell::GlobalMenu::DbusMenu;
class GlobalMenuExport::Impl final : public Exporter::ExportLineageSource {
public:
    Impl(GlobalMenuExport *owner, BrowserView *view, QWindow *window)
        : owner(owner), view(view), window(window), source(view), exporter(source, *this), server(owner)
    {
        static quint64 counter = 0;
        path = QStringLiteral("/org/qindaqt/QindaWeb/Menu/%1").arg(++counter);
        connect(view->commands(), &QindaTK::CommandRegistry::changed, owner, [this] { refresh(); });
        connect(view->shortcuts(), &QindaTK::ShortcutMap::changed, owner, [this] { refresh(); });
        connect(&server, &DbusMenu::DbusMenuServer::actionActivated, owner, [this](const QString &id) {
            // Re-resolve live command state; hidden chrome has no authority.
            if (this->view) (void)this->view->commands()->invoke(id);
        });
    }
    ~Impl()
    {
        if (appMenu) appMenu->release();
        if (x11Registered) {
            auto request = QDBusMessage::createMethodCall(QStringLiteral("com.canonical.AppMenu.Registrar"),
                QStringLiteral("/com/canonical/AppMenu/Registrar"), QStringLiteral("com.canonical.AppMenu.Registrar"), QStringLiteral("UnregisterWindow"));
            request.setArguments({x11Id});
            bus.asyncCall(request);
        }
        if (registered) bus.unregisterObject(path);
    }
    std::optional<Exporter::ExportLineage> lineageFor(const QUuid &) const override
    {
        return Exporter::ExportLineage{epoch, ++revision};
    }
    void setAvailable(bool value)
    {
        if (announced == value) return;
        announced = value;
        emit owner->availableChanged();
    }
    void refresh()
    {
        if (!registered) return;
        const auto result = exporter.refresh();
        if (result.outcome == Exporter::ExportOutcome::Published) {
            const auto tree = exporter.lastAccepted();
            published = tree && server.publish(*tree);
        } else if (result.outcome != Exporter::ExportOutcome::Unchanged) published = false;
        setAvailable(published && ((appMenu && appMenu->isValid()) || x11Registered));
    }
    void announceWayland()
    {
        if (appMenu || !manager || !window || !registered || !published) return;
        auto *surface = KWayland::Client::Surface::fromWindow(window);
        if (!surface) return;
        appMenu = manager->create(surface, owner);
        if (!appMenu || !appMenu->isValid()) return;
        appMenu->setAddress(bus.baseService(), path);
        setAvailable(true);
    }
    void start()
    {
        if (!window || !view || !bus.isConnected() || registered) return;
        DbusMenu::registerDbusMenuWireTypes();
        registered = bus.registerObject(path, &server, QDBusConnection::ExportScriptableContents);
        if (!registered) return;
        refresh();
        if (QGuiApplication::platformName().startsWith(QStringLiteral("wayland"))) {
            auto *connection = KWayland::Client::ConnectionThread::fromApplication(owner);
            if (!connection) return;
            registry = new KWayland::Client::Registry(owner);
            connect(registry, &KWayland::Client::Registry::appMenuAnnounced, owner, [this](quint32 name, quint32 version) {
                if (manager) return;
                manager = registry->createAppMenuManager(name, version, owner);
                connect(manager, &KWayland::Client::AppMenuManager::removed, owner, [this] {
                    if (appMenu) { appMenu->release(); appMenu->deleteLater(); appMenu = nullptr; }
                    manager->deleteLater(); manager = nullptr;
                    setAvailable(false);
                });
                announceWayland();
            });
            connect(window, &QWindow::visibleChanged, owner, [this] { announceWayland(); });
            connect(connection, &KWayland::Client::ConnectionThread::connectionDied, owner, [this] {
                if (appMenu) appMenu->destroy();
                if (manager) manager->destroy();
                if (registry) registry->destroy();
                setAvailable(false);
            });
            registry->create(connection);
            registry->setup();
        } else if (published && QGuiApplication::platformName() == QStringLiteral("xcb")) {
            const auto id = window->winId();
            if (!id || id > std::numeric_limits<quint32>::max()) return;
            x11Id = static_cast<quint32>(id);
            auto request = QDBusMessage::createMethodCall(QStringLiteral("com.canonical.AppMenu.Registrar"),
                QStringLiteral("/com/canonical/AppMenu/Registrar"), QStringLiteral("com.canonical.AppMenu.Registrar"), QStringLiteral("RegisterWindow"));
            request.setArguments({x11Id, QVariant::fromValue(QDBusObjectPath(path))});
            const auto result = bus.call(request);
            x11Registered = result.type() != QDBusMessage::ErrorMessage;
            setAvailable(x11Registered);
        }
    }
    GlobalMenuExport *owner;
    QPointer<BrowserView> view;
    QPointer<QWindow> window;
    GlobalMenuSource source;
    Exporter::MenuExporter exporter;
    DbusMenu::DbusMenuServer server;
    QDBusConnection bus = QDBusConnection::sessionBus();
    QUuid epoch = QUuid::createUuid();
    mutable quint64 revision = 0;
    QString path;
    KWayland::Client::Registry *registry = nullptr;
    KWayland::Client::AppMenuManager *manager = nullptr;
    KWayland::Client::AppMenu *appMenu = nullptr;
    bool registered = false;
    bool published = false;
    bool announced = false;
    bool x11Registered = false;
    quint32 x11Id = 0;
};
GlobalMenuExport::GlobalMenuExport(BrowserView *view, QWindow *window)
    : m_impl(std::make_unique<Impl>(this, view, window)) {}
GlobalMenuExport::~GlobalMenuExport() = default;
bool GlobalMenuExport::available() const { return m_impl->announced; }
QString GlobalMenuExport::objectPath() const { return m_impl->path; }
void GlobalMenuExport::start() { m_impl->start(); }
}
