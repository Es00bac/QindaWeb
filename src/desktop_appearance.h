// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <memory>
class QQmlEngine;
namespace QindaWeb {
// GUI-thread, read-only Settings1 consumer. Exact owner/revision validation
// stays in the public desktop client; only complete themes reach TokenFacade.
class DesktopAppearance final : public QObject {
    Q_OBJECT
public:
    explicit DesktopAppearance(QQmlEngine *engine, QObject *parent = nullptr);
    ~DesktopAppearance() override;
    bool start(QString *error);
    bool ready() const;
    QString error() const;
signals:
    void changed();
private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
}
