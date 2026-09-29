// SPDX-License-Identifier: GPL-3.0-or-later
#include "browser_view.h"
#include "browser_controller.h"
#include "navigation.h"
#include <QQuickWindow>
#include <QWaylandSurface>
#include <QUuid>
#include <cmath>
namespace QindaWeb {
BrowserView::BrowserView(BrowserController *controller)
    : QObject(controller), m_controller(controller),
      m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    m_commands.setContext(&m_commandContext);
    m_shortcuts.setRegistry(&m_commands);
    installCommands();
}
void BrowserView::setWindow(QQuickWindow *window, QWaylandQuickOutput *output)
{
    m_window = window;
    m_output = output;
    m_shortcuts.setWindow(window);
    connect(window, &QWindow::activeChanged, this, [this] {
        if (m_window && m_window->isActive()) m_controller->activate(this);
    });
    emit changed();
}
void BrowserView::setSurface(QWaylandXdgSurface *surface)
{
    m_surface = surface;
    connect(surface->toplevel(), &QWaylandXdgToplevel::titleChanged, this, [this] {
        if (m_surface) setTitle(m_surface->toplevel()->title());
    });
    connect(surface->surface(), &QObject::destroyed, this, [this] {
        m_surface = nullptr;
        emit changed();
    });
    resizeContent(m_contentSize.width(), m_contentSize.height());
    emit changed();
}
void BrowserView::setContextId(const QString &context)
{
    m_contextId = context;
    m_commands.invalidate();
    emit changed();
}
void BrowserView::setUrl(const QUrl &url) { m_url = url; emit changed(); }
void BrowserView::setTitle(const QString &title)
{
    m_title = title;
    for (const QString &suffix : {QStringLiteral(" — Mozilla Firefox"), QStringLiteral(" — QindaFox")})
        if (m_title.endsWith(suffix)) m_title.chop(suffix.size());
    if (m_title.isEmpty()) m_title = QStringLiteral("New page");
    emit changed();
}
void BrowserView::setLoading(bool value) { m_loading = value; emit changed(); }
void BrowserView::setMessage(const QString &message) { m_message = message; emit changed(); }
bool BrowserView::navigate(const QString &input)
{
    QString error;
    const QUrl target = navigationUrl(input, &error);
    if (!target.isValid() || target.isEmpty()) { setMessage(error); return false; }
    m_controller->navigate(this, target);
    return true;
}
void BrowserView::resizeContent(qreal width, qreal height)
{
    if (!std::isfinite(width) || !std::isfinite(height) || width < 1 || height < 1) return;
    const QSize size(qRound(width), qRound(height));
    m_contentSize = size;
    if (!m_surface || !m_surface->toplevel()) return;
    const QList<QWaylandXdgToplevel::State> states{QWaylandXdgToplevel::ActivatedState};
    m_surface->toplevel()->sendConfigure(size, states);
}
void BrowserView::confirmSplit(const QString &direction) { m_controller->confirmSplit(this, direction); }
void BrowserView::cancelSplit() { m_controller->cancelSplit(this); }
}
