// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <QHash>
#include <QJsonObject>
#include <QWebSocket>
#include <QTimer>
#include <functional>
namespace QindaWeb {
// Private engine-control channel, GUI thread. Every request finishes exactly
// once on response, timeout or disconnect; never expose it as an agent API.
class BidiClient final : public QObject {
    Q_OBJECT
public:
    using Callback = std::function<void(const QJsonObject &, const QString &)>;
    explicit BidiClient(QObject *parent = nullptr);
    void open(const QUrl &endpoint);
    quint64 request(const QString &method, const QJsonObject &params, Callback callback = {});
    void close();
    void flush() { m_socket.flush(); }
signals:
    void connected();
    void disconnected(const QString &reason);
    void event(const QString &method, const QJsonObject &params);
private:
    struct Pending { Callback callback; qint64 deadline; };
    void receive(const QString &message);
    void failPending(const QString &reason);
    QWebSocket m_socket;
    QHash<quint64, Pending> m_pending;
    QTimer m_deadlines;
    quint64 m_nextId = 0;
};
}
