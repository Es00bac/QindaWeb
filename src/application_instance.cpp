// SPDX-License-Identifier: GPL-3.0-or-later
#include "application_instance.h"
#include <QCryptographicHash>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDir>
namespace QindaWeb {
namespace {
constexpr auto launchPath = "/org/qindaqt/QindaWeb/Launch";
bool bounded(const QStringList &urls)
{
    if (urls.isEmpty() || urls.size() > 32) return false;
    qsizetype total = 0;
    for (const auto &url : urls) {
        const auto bytes = url.toUtf8().size();
        if (bytes > 8192 || url.contains(QChar::Null)) return false;
        total += bytes;
    }
    return total <= 32768;
}
}
ApplicationInstance::ApplicationInstance(const QString &profile, QObject *parent)
    : QObject(parent), m_lock(profile + QStringLiteral("/qindaweb.lock"))
{
    const QString canonical = QDir(profile).canonicalPath();
    const QByteArray identity = (canonical.isEmpty() ? QDir(profile).absolutePath() : canonical).toUtf8();
    m_service = QStringLiteral("org.qindaqt.QindaWeb.Profile.p") +
                QString::fromLatin1(QCryptographicHash::hash(identity, QCryptographicHash::Sha256).toHex().left(32));
}
ApplicationInstance::~ApplicationInstance()
{
    if (!m_primary) return;
    auto bus = QDBusConnection::sessionBus();
    bus.unregisterObject(QString::fromLatin1(launchPath));
    bus.unregisterService(m_service);
}
ApplicationInstance::Result ApplicationInstance::start(const QStringList &urls, bool hideControls, QString *error)
{
    if (!bounded(urls)) {
        if (error) *error = QStringLiteral("Too many or oversized addresses.");
        return Result::Failed;
    }
    auto bus = QDBusConnection::sessionBus();
    if (!m_lock.tryLock(0)) {
        auto request = QDBusMessage::createMethodCall(m_service, QString::fromLatin1(launchPath),
            QStringLiteral("org.qindaqt.QindaWeb.Launch1"), QStringLiteral("OpenUrls"));
        request.setArguments({urls, hideControls});
        auto pending = bus.asyncCall(request, 4000);
        pending.waitForFinished();
        const auto reply = pending.reply();
        if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty() && reply.arguments().first().toBool())
            return Result::Forwarded;
        if (error) *error = QStringLiteral("This profile is in use, but its browser could not accept the launch.");
        return Result::Failed;
    }
    // Browsing works off-desktop without a session bus; forwarding then
    // reports a bounded failure rather than opening the locked profile twice.
    if (!bus.isConnected()) return Result::Primary;
    if (!bus.registerObject(QString::fromLatin1(launchPath), this, QDBusConnection::ExportScriptableSlots)) {
        if (error) *error = QStringLiteral("Cannot register the browser launch endpoint.");
        return Result::Failed;
    }
    if (!bus.registerService(m_service)) {
        bus.unregisterObject(QString::fromLatin1(launchPath));
        if (error) *error = QStringLiteral("Cannot register this browser profile's launch service.");
        return Result::Failed;
    }
    m_primary = true;
    return Result::Primary;
}
bool ApplicationInstance::OpenUrls(const QStringList &urls, bool hideControls)
{
    if (!bounded(urls) || m_queue.size() >= 8) return false;
    if (m_ready) emit openRequested(urls, hideControls);
    else m_queue.enqueue({urls, hideControls});
    return true;
}
void ApplicationInstance::setReady()
{
    m_ready = true;
    while (!m_queue.isEmpty()) {
        const auto request = m_queue.dequeue();
        emit openRequested(request.first, request.second);
    }
}
}
