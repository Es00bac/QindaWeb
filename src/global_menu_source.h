// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <qindaqt/shell/global_menu/exporter/menu_source.h>
namespace QindaWeb {
class BrowserView;
// Persistent semantic commands, independent of local menu item visibility.
class GlobalMenuSource final : public QindaQt::Shell::GlobalMenu::Exporter::MenuSource {
public:
    explicit GlobalMenuSource(BrowserView *view) : m_view(view) {}
    QindaQt::Shell::GlobalMenu::Exporter::MenuSnapshot snapshot() const override;
private:
    BrowserView *m_view;
};
}
