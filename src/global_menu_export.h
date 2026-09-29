// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <memory>
class QWindow;
namespace QindaWeb {
class BrowserView;
// GUI-thread per-window dbusmenu endpoint. Native support must be detected and
// the menu published/announced before automatic local-menu suppression.
// stop before destroying the borrowed native window.
class GlobalMenuExport final : public QObject {
    Q_OBJECT
public:
    GlobalMenuExport(BrowserView *view, QWindow *window);
    ~GlobalMenuExport() override;
    bool available() const;
    QString objectPath() const;
    void start();
signals:
    void availableChanged();
private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
}
