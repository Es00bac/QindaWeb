// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "bidi_client.h"
#include <QObject>
#include <QProcess>
#include <QPointer>
#include <QQueue>
#include <QHash>
#include <QUrl>
#include <QTimer>
class QWaylandXdgSurface;
namespace QindaWeb {
class GeckoSurfaceHost;
class BrowserView;
// One Firefox parent owns the profile and isolated Gecko content processes.
// Native windows and view lifetimes remain in BrowserController. Creation is
// serialized so each BiDi context and Wayland toplevel are paired once.
class FirefoxEngine final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString identity READ identity NOTIFY identityChanged)
public:
    explicit FirefoxEngine(GeckoSurfaceHost *host, QObject *parent = nullptr);
    ~FirefoxEngine() override;
    bool start(const QString &program, const QString &profile, QString *error);
    void addView(BrowserView *view, const QUrl &url);
    void adoptReadyView(BrowserView *view, const QString &context, const QUrl &url, QWaylandXdgSurface *surface);
    void closeView(BrowserView *view);
    void navigate(BrowserView *view, const QUrl &url);
    void reload(BrowserView *view);
    void activate(BrowserView *view);
    void traverse(BrowserView *view, int delta);
    void stop();
    QString identity() const { return m_identity; }
    BidiClient *control() { return &m_bidi; }
signals:
    void identityChanged();
    void failed(const QString &message);
    void ready();
    void unsolicitedWindow(const QString &context, const QUrl &url, QWaylandXdgSurface *surface, const QString &title);
    void viewClosed(BrowserView *view);
private:
    struct Creation {
        QPointer<BrowserView> view;
        QUrl url;
        QString context;
        QString marker;
        QString originalTitle;
        bool submitted = false;
        bool marked = false;
        bool navigate = true;
        bool unsolicited = false;
        bool marking = false;
        bool remarkPending = false;
    };
    void processOutput();
    void establishSession();
    void nextCreation();
    void pairCreation();
    void markCreation();
    void rejectCreation(const QString &error);
    void offerCandidates();
    void engineEvent(const QString &method, const QJsonObject &params);
    void fail(const QString &message);
    void viewCommand(BrowserView *view, const QString &method, QJsonObject params);
    GeckoSurfaceHost *m_host;
    QProcess m_process;
    BidiClient m_bidi;
    QHash<QString, QPointer<BrowserView>> m_views;
    QQueue<Creation> m_creation;
    QQueue<QPointer<QWaylandXdgSurface>> m_surfaces;
    QByteArray m_output;
    QTimer m_startupTimer;
    QTimer m_pairTimer;
    QHash<QString, QUrl> m_candidates;
    QStringList m_bootstrapContexts;
    QString m_identity = QStringLiteral("Gecko / Firefox (starting)");
    bool m_connected = false;
    bool m_ready = false;
    bool m_stopping = false;
};
}
