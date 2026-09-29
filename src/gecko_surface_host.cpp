// SPDX-License-Identifier: GPL-3.0-or-later
#include "gecko_surface_host.h"
#include <QWaylandXdgShell>
#include <QWaylandQuickOutput>
#include <QWaylandOutputMode>
#include <QWaylandXdgDecorationManagerV1>
#include <QWaylandTextInputManagerV3>
#include <QWaylandViewporter>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QFile>
#include <cmath>
namespace QindaWeb {
GeckoSurfaceHost::GeckoSurfaceHost(QObject *parent)
    : QWaylandQuickCompositor(parent),
      m_displayDirectory(QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation) +
                         QStringLiteral("/qindaweb-display-XXXXXX"))
{
    auto *shell = new QWaylandXdgShell(this);
    connect(shell, &QWaylandXdgShell::toplevelCreated, this,
        [this](QWaylandXdgToplevel *, QWaylandXdgSurface *surface) { emit toplevelCreated(surface); });
    auto *decorations = new QWaylandXdgDecorationManagerV1;
    decorations->setExtensionContainer(this);
    decorations->setPreferredMode(QWaylandXdgToplevel::ServerSideDecoration);
    new QWaylandTextInputManagerV3(this);
    new QWaylandViewporter(this);
    setUseHardwareIntegrationExtension(false);
}
QString GeckoSurfaceHost::displayName() const
{
    return m_displayDirectory.path() + QStringLiteral("/wayland");
}
bool GeckoSurfaceHost::start(QString *error)
{
    if (!m_displayDirectory.isValid()) {
        if (error) *error = QStringLiteral("Cannot create Gecko's private Wayland display.");
        return false;
    }
    setSocketName(displayName().toUtf8());
    create();
    if (!isCreated()) {
        if (error) *error = QStringLiteral("Cannot start Gecko's private Wayland display.");
        return false;
    }
    return true;
}
QWaylandQuickOutput *GeckoSurfaceHost::addWindow(QQuickWindow *window)
{
    auto *output = new QWaylandQuickOutput(this, window);
    output->setParent(window);
    output->setManufacturer(QStringLiteral("QindaQt"));
    output->setModel(QStringLiteral("QindaWeb content"));
    output->setSizeFollowsWindow(true);
    output->setScaleFactor(qMax(1, static_cast<int>(std::ceil(window->devicePixelRatio()))));
    const QWaylandOutputMode mode(window->size(), 60000);
    output->addMode(mode, true);
    output->setCurrentMode(mode);
    return output;
}
}
