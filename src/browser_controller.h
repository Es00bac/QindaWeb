// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "gecko_surface_host.h"
#include "firefox_engine.h"
#include "placement_client.h"
#include <QObject>
#include <QPointer>
#include <QHash>
#include <QUrl>
class QQmlApplicationEngine;
namespace QindaWeb {
class BrowserView;
class BrowserController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString engineIdentity READ engineIdentity NOTIFY engineIdentityChanged)
    Q_PROPERTY(bool placementAvailable READ placementAvailable CONSTANT)
public:
    explicit BrowserController(QQmlApplicationEngine *qml, QObject *parent = nullptr);
    ~BrowserController() override;
    bool start(const QString &program, const QString &profile, QString *error);
    BrowserView *newView(BrowserView *source = nullptr, const QString &placement = {},
                         const QUrl &url = QUrl(QStringLiteral("about:blank")));
    void beginSplit(BrowserView *source);
    void confirmSplit(BrowserView *source, const QString &direction);
    void cancelSplit(BrowserView *source);
    void navigate(BrowserView *view, const QUrl &url);
    void reload(BrowserView *view);
    void traverse(BrowserView *view, int delta);
    void closeView(BrowserView *view);
    void activate(BrowserView *view);
    QString engineIdentity() const { return m_engine.identity(); }
    bool placementAvailable() const { return m_placement.available(); }
    FirefoxEngine *engine() { return &m_engine; }
    QList<BrowserView *> views() const;
signals:
    void engineIdentityChanged();
    void viewCreated(BrowserView *view);
private:
    void submitPlacement(BrowserView *source, BrowserView *created, const QString &placement);
    QQmlApplicationEngine *m_qml;
    GeckoSurfaceHost m_host;
    FirefoxEngine m_engine;
    PlacementClient m_placement;
    QHash<QString, QPointer<BrowserView>> m_views;
    QHash<QString, QPointer<BrowserView>> m_splitCandidates;
    QHash<quint32, QPointer<BrowserView>> m_placements;
    bool m_closing = false;
};
}
