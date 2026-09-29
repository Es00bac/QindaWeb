// SPDX-License-Identifier: GPL-3.0-or-later
#include "bidi_client.h"
#include <QDateTime>
#include <QJsonDocument>
#include <utility>
namespace QindaWeb {
BidiClient::BidiClient(QObject *parent) : QObject(parent)
{
    connect(&m_socket, &QWebSocket::connected, this, &BidiClient::connected);
    connect(&m_socket, &QWebSocket::textMessageReceived, this, &BidiClient::receive);
    connect(&m_socket, &QWebSocket::disconnected, this, [this] {
        failPending(QStringLiteral("The Gecko control connection closed."));
        emit disconnected(QStringLiteral("The Gecko control connection closed."));
    });
    connect(&m_socket, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        failPending(m_socket.errorString());
        emit disconnected(m_socket.errorString());
    });
    m_deadlines.setInterval(500);
    connect(&m_deadlines, &QTimer::timeout, this, [this] {
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        const auto ids = m_pending.keys();
        for (quint64 id : ids) {
            if (m_pending.value(id).deadline > now) continue;
            auto callback = m_pending.take(id).callback;
            if (callback) callback({}, QStringLiteral("The Gecko command timed out."));
        }
    });
    m_deadlines.start();
}
void BidiClient::open(const QUrl &endpoint)
{
    // Accept only the loopback endpoint printed by our own Firefox child.
    if (endpoint.scheme() != QStringLiteral("ws") || endpoint.host() != QStringLiteral("127.0.0.1")) {
        emit disconnected(QStringLiteral("Firefox supplied an invalid control endpoint."));
        return;
    }
    m_socket.open(endpoint);
}
quint64 BidiClient::request(const QString &method, const QJsonObject &params, Callback callback)
{
    if (m_socket.state() != QAbstractSocket::ConnectedState) {
        if (callback) callback({}, QStringLiteral("Gecko is not connected."));
        return 0;
    }
    const quint64 id = ++m_nextId;
    m_pending.insert(id, {std::move(callback), QDateTime::currentMSecsSinceEpoch() + 15000});
    const QJsonObject message{{QStringLiteral("id"), static_cast<qint64>(id)},
                              {QStringLiteral("method"), method}, {QStringLiteral("params"), params}};
    m_socket.sendTextMessage(QString::fromUtf8(QJsonDocument(message).toJson(QJsonDocument::Compact)));
    return id;
}
void BidiClient::receive(const QString &message)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) return;
    const QJsonObject object = document.object();
    if (object.contains(QStringLiteral("id"))) {
        const auto id = object.value(QStringLiteral("id")).toInteger();
        auto it = m_pending.find(id);
        if (it == m_pending.end()) return;
        auto callback = std::move(it->callback);
        m_pending.erase(it);
        const QString error = object.value(QStringLiteral("type")).toString() == QStringLiteral("error")
            ? object.value(QStringLiteral("message")).toString(object.value(QStringLiteral("error")).toString())
            : QString();
        if (callback) callback(object.value(QStringLiteral("result")).toObject(), error);
    } else {
        emit event(object.value(QStringLiteral("method")).toString(),
                   object.value(QStringLiteral("params")).toObject());
    }
}
void BidiClient::failPending(const QString &reason)
{
    const auto pending = std::exchange(m_pending, {});
    for (const auto &request : pending) if (request.callback) request.callback({}, reason);
}
void BidiClient::close() { m_socket.close(); }
}
