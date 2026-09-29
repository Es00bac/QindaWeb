// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <memory>
class QWindow;
namespace QindaWeb {
class PlacementClient final : public QObject {
    Q_OBJECT
public:
    explicit PlacementClient(QObject *parent = nullptr);
    ~PlacementClient() override;
    bool available() const;
    quint32 place(QWindow *source, QWindow *created, const QString &placement, QString *error);
    void cancel(quint32 id);
signals:
    void availableChanged();
    void finished(quint32 id, bool accepted, const QString &containerId, const QString &message);
private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
}
