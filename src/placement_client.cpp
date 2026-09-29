// SPDX-License-Identifier: GPL-3.0-or-later
#include "placement_client.h"
#include <QHash>
#ifdef QINDAWEB_HAVE_PLACEMENT
#include <qindaqt/application_window_management/client.h>
#endif
namespace QindaWeb {
class PlacementClient::Impl {
public:
#ifdef QINDAWEB_HAVE_PLACEMENT
    QindaQt::ApplicationWindowManagement::WindowPlacementClient client;
#endif
};
PlacementClient::PlacementClient(QObject *parent) : QObject(parent), m_impl(std::make_unique<Impl>())
{
#ifdef QINDAWEB_HAVE_PLACEMENT
    using namespace QindaQt::ApplicationWindowManagement;
    connect(&m_impl->client, &WindowPlacementClient::finished, this,
        [this](quint32 id, Status status, const QString &container, const QString &message) {
            emit finished(id, status == Status::Accepted, container, message);
        });
#endif
}
PlacementClient::~PlacementClient() = default;
bool PlacementClient::available() const
{
#ifdef QINDAWEB_HAVE_PLACEMENT
    return m_impl->client.available();
#else
    return false;
#endif
}
quint32 PlacementClient::place(QWindow *source, QWindow *created, const QString &placement, QString *error)
{
#ifdef QINDAWEB_HAVE_PLACEMENT
    using namespace QindaQt::ApplicationWindowManagement;
    const QHash<QString, Placement> values{{QStringLiteral("tab"), Placement::Tab},
        {QStringLiteral("right"), Placement::TileRight}, {QStringLiteral("down"), Placement::TileDown},
        {QStringLiteral("left"), Placement::TileLeft}, {QStringLiteral("up"), Placement::TileUp}};
    if (!values.contains(placement)) {
        if (error) *error = QStringLiteral("Unknown placement direction.");
        return 0;
    }
    return m_impl->client.place(source, created, values.value(placement), error);
#else
    Q_UNUSED(source); Q_UNUSED(created); Q_UNUSED(placement);
    if (error) *error = QStringLiteral("Desktop placement is unavailable; the new page has its own window.");
    return 0;
#endif
}
void PlacementClient::cancel(quint32 id)
{
#ifdef QINDAWEB_HAVE_PLACEMENT
    m_impl->client.cancel(id);
#else
    Q_UNUSED(id);
#endif
}
}
