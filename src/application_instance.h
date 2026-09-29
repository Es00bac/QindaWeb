// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <QLockFile>
#include <QQueue>
namespace QindaWeb {
// Per-owned-profile launch IPC, not an agent API. The profile lock elects
// one GUI process; same-session launches forward bounded address lists.
class ApplicationInstance final : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.qindaqt.QindaWeb.Launch1")
public:
    enum class Result { Primary, Forwarded, Failed };
    explicit ApplicationInstance(const QString &profile, QObject *parent = nullptr);
    ~ApplicationInstance() override;
    Result start(const QStringList &urls, bool hideControls, QString *error);
    void setReady();
public slots:
    Q_SCRIPTABLE bool OpenUrls(const QStringList &urls, bool hideControls);
signals:
    void openRequested(const QStringList &urls, bool hideControls);
private:
    QLockFile m_lock;
    QString m_service;
    QQueue<QPair<QStringList, bool>> m_queue;
    bool m_primary = false;
    bool m_ready = false;
};
}
