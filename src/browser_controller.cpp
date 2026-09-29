// SPDX-License-Identifier: GPL-3.0-or-later
#include "browser_controller.h"
#include "browser_view.h"
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QGuiApplication>
#include <QTimer>
namespace QindaWeb {
BrowserController::BrowserController(QQmlApplicationEngine *qml, QObject *parent)
    : QObject(parent), m_qml(qml), m_engine(&m_host)
{
    connect(&m_engine, &FirefoxEngine::identityChanged, this, &BrowserController::engineIdentityChanged);
    connect(&m_placement, &PlacementClient::finished, this,
        [this](quint32 id, bool accepted, const QString &, const QString &message) {
            const auto view = m_placements.take(id);
            if (view) view->setMessage(accepted ? QString() : message);
        });
}
BrowserController::~BrowserController()
{
    m_closing = true;
    const auto live = views();
    for (auto *view : live) {
        if (view->window()) delete view->window();
        delete view;
    }
}
bool BrowserController::start(const QString &program, const QString &profile, QString *error)
{
    return m_host.start(error) && m_engine.start(program, profile, error);
}
QList<BrowserView *> BrowserController::views() const
{
    QList<BrowserView *> result;
    for (auto view : m_views) if (view) result.append(view);
    return result;
}
BrowserView *BrowserController::newView(BrowserView *source, const QString &placement, const QUrl &url)
{
    auto *view = new BrowserView(this);
    QQmlComponent component(m_qml, QUrl(QStringLiteral("qrc:/qml/BrowserWindow.qml")));
    QObject *object = component.createWithInitialProperties({{QStringLiteral("view"), QVariant::fromValue(view)},
                                                              {QStringLiteral("browser"), QVariant::fromValue(this)}});
    auto *window = qobject_cast<QQuickWindow *>(object);
    if (!window) {
        qWarning().noquote() << component.errorString();
        delete object;
        delete view;
        return nullptr;
    }
    QQmlEngine::setObjectOwnership(window, QQmlEngine::CppOwnership);
    window->QObject::setParent(this);
    view->setWindow(window, m_host.addWindow(window));
    m_views.insert(view->id(), view);
    connect(window, &QQuickWindow::closing, this, [this, view](QQuickCloseEvent *) {
        if (!m_closing) QTimer::singleShot(0, this, [this, guard = QPointer<BrowserView>(view)] {
            if (guard) closeView(guard);
        });
    });
    window->show();
    m_engine.addView(view, url);
    if (source && !placement.isEmpty()) submitPlacement(source, view, placement);
    emit viewCreated(view);
    return view;
}
void BrowserController::submitPlacement(BrowserView *source, BrowserView *created, const QString &placement)
{
    if (!source || !created || !source->window() || !created->window()) return;
    QString error;
    const quint32 id = m_placement.place(source->window(), created->window(), placement, &error);
    if (id) m_placements.insert(id, created);
    else created->setMessage(error);
}
void BrowserController::beginSplit(BrowserView *source)
{
    if (!source || m_splitCandidates.contains(source->id())) return;
    auto *created = newView();
    if (!created) return;
    m_splitCandidates.insert(source->id(), created);
    source->window()->requestActivate();
    emit source->placementRequested();
}
void BrowserController::confirmSplit(BrowserView *source, const QString &direction)
{
    if (!source || !QStringList{QStringLiteral("left"), QStringLiteral("right"),
        QStringLiteral("up"), QStringLiteral("down")}.contains(direction)) return;
    const auto created = m_splitCandidates.take(source->id());
    if (created) submitPlacement(source, created, direction);
}
void BrowserController::cancelSplit(BrowserView *source)
{
    if (!source) return;
    const auto created = m_splitCandidates.take(source->id());
    if (created) created->setMessage(QStringLiteral("The new page is in a separate window."));
}
void BrowserController::navigate(BrowserView *view, const QUrl &url) { m_engine.navigate(view, url); }
void BrowserController::reload(BrowserView *view) { m_engine.reload(view); }
void BrowserController::traverse(BrowserView *view, int delta) { m_engine.traverse(view, delta); }
void BrowserController::activate(BrowserView *) { }
void BrowserController::closeView(BrowserView *view)
{
    if (!view || !m_views.contains(view->id())) return;
    m_views.remove(view->id());
    m_splitCandidates.remove(view->id());
    m_engine.closeView(view);
    if (view->window()) view->window()->deleteLater();
    view->deleteLater();
    if (m_views.isEmpty()) {
        m_engine.stop();
        QGuiApplication::quit();
    }
}
}
