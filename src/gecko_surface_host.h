// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QWaylandQuickCompositor>
#include <QTemporaryDir>
class QWaylandQuickOutput;
class QWaylandXdgSurface;
class QQuickWindow;
namespace QindaWeb {
// Gecko connects to a private internal display. Only application-owned Qt
// toplevels connect to the desktop; Gecko surfaces never enter the WM SDK.
class GeckoSurfaceHost final : public QWaylandQuickCompositor {
    Q_OBJECT
public:
    explicit GeckoSurfaceHost(QObject *parent = nullptr);
    bool start(QString *error);
    QString displayName() const;
    QWaylandQuickOutput *addWindow(QQuickWindow *window);
signals:
    void toplevelCreated(QWaylandXdgSurface *surface);
private:
    QTemporaryDir m_displayDirectory;
};
}
