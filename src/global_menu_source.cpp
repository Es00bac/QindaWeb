// SPDX-License-Identifier: GPL-3.0-or-later
#include "global_menu_source.h"
#include "browser_view.h"
namespace QindaWeb {
QindaQt::Shell::GlobalMenu::Exporter::MenuSnapshot GlobalMenuSource::snapshot() const
{
    using namespace QindaQt::Shell::GlobalMenu;
    Exporter::MenuSnapshot result;
    result.tree.ownerWindowId = QUuid(m_view->id());
    const QList<QPair<QString, QStringList>> sections{
        {QStringLiteral("File"), {QStringLiteral("browser.new-tab"), QStringLiteral("browser.new-split"), QStringLiteral("browser.new-window"), QStringLiteral("browser.close")}},
        {QStringLiteral("Navigate"), {QStringLiteral("browser.back"), QStringLiteral("browser.forward"), QStringLiteral("browser.reload"), QStringLiteral("browser.address")}},
        {QStringLiteral("View"), {QStringLiteral("browser.chrome")}}};
    for (const auto &section : sections) {
        Protocol::MenuItem menu;
        menu.id = QStringLiteral("menu.") + section.first.toLower();
        menu.kind = Protocol::MenuItemKind::Submenu;
        menu.text = section.first;
        menu.mnemonicIndex = 0;
        for (const auto &id : section.second) {
            const auto command = m_view->commands()->resolve(id);
            if (command.descriptor.id.isEmpty()) {
                result.complete = false;
                result.defectCode = QStringLiteral("unknown-command");
                result.tree.items.clear();
                return result;
            }
            Protocol::MenuItem item;
            item.id = id;
            item.text = command.descriptor.label;
            item.shortcutText = m_view->shortcuts()->displaySequence(id);
            item.enabled = command.enabled;
            item.visible = command.visible;
            item.checkable = command.descriptor.checkable;
            item.checked = command.checked;
            menu.children.append(item);
        }
        result.tree.items.append(menu);
    }
    return result;
}
}
