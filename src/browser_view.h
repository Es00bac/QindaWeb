// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <QPointer>
#include <QUrl>
#include <QWaylandXdgSurface>
#include <QWaylandQuickOutput>
#include <qindatk/command_registry.h>
#include <qindatk/command_context.h>
#include <qindatk/shortcut_map.h>
class QQuickWindow;
namespace QindaWeb {
class BrowserController;
class GlobalMenuExport;
class BrowserView final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString title READ title NOTIFY changed)
    Q_PROPERTY(QUrl url READ url NOTIFY changed)
    Q_PROPERTY(bool loading READ loading NOTIFY changed)
    Q_PROPERTY(bool ready READ ready NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(bool chromeVisible READ chromeVisible WRITE setChromeVisible NOTIFY changed)
    Q_PROPERTY(bool globalMenuAvailable READ globalMenuAvailable NOTIFY changed)
    Q_PROPERTY(QWaylandXdgSurface *shellSurface READ shellSurface NOTIFY changed)
    Q_PROPERTY(QWaylandQuickOutput *output READ output NOTIFY changed)
    Q_PROPERTY(QindaTK::CommandRegistry *commands READ commands CONSTANT)
    Q_PROPERTY(QindaTK::ShortcutMap *shortcuts READ shortcuts CONSTANT)
public:
    explicit BrowserView(BrowserController *controller);
    ~BrowserView() override;
    QString id() const { return m_id; }
    QString title() const { return m_title; }
    QUrl url() const { return m_url; }
    bool loading() const { return m_loading; }
    bool ready() const { return !m_contextId.isEmpty(); }
    QString message() const { return m_message; }
    bool chromeVisible() const { return m_chromeVisible; }
    bool globalMenuAvailable() const;
    QString globalMenuObjectPath() const;
    void setChromeVisible(bool visible);
    void stopMenuExport();
    QWaylandXdgSurface *shellSurface() const { return m_surface; }
    QWaylandQuickOutput *output() const { return m_output; }
    QindaTK::CommandRegistry *commands() { return &m_commands; }
    QindaTK::ShortcutMap *shortcuts() { return &m_shortcuts; }
    QQuickWindow *window() const { return m_window; }
    QString contextId() const { return m_contextId; }
    void setWindow(QQuickWindow *window, QWaylandQuickOutput *output);
    void setSurface(QWaylandXdgSurface *surface);
    void setContextId(const QString &context);
    void setUrl(const QUrl &url);
    void setLoading(bool value);
    void setMessage(const QString &message);
    void setTitle(const QString &title);
    Q_INVOKABLE bool navigate(const QString &input);
    Q_INVOKABLE void resizeContent(qreal width, qreal height);
    Q_INVOKABLE void confirmSplit(const QString &direction);
    Q_INVOKABLE void cancelSplit();
signals:
    void changed();
    void focusAddressRequested();
    void placementRequested();
private:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void installCommands();
    BrowserController *m_controller;
    const QString m_id;
    QString m_title = QStringLiteral("New page");
    QUrl m_url = QUrl(QStringLiteral("about:blank"));
    QString m_message = QStringLiteral("Starting Gecko…");
    QString m_contextId;
    bool m_loading = true;
    bool m_chromeVisible = true;
    std::unique_ptr<GlobalMenuExport> m_globalMenu;
    QSize m_contentSize{960, 600};
    QPointer<QQuickWindow> m_window;
    QPointer<QWaylandXdgSurface> m_surface;
    QPointer<QWaylandQuickOutput> m_output;
    QindaTK::CommandContext m_commandContext;
    QindaTK::CommandRegistry m_commands;
    QindaTK::ShortcutMap m_shortcuts;
};
}
