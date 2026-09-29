// SPDX-License-Identifier: GPL-3.0-or-later
#include "browser_view.h"
#include "browser_controller.h"
#include <QQuickWindow>
namespace QindaWeb {
void BrowserView::installCommands()
{
    using namespace QindaTK;
    const QString scope = m_commandContext.pushScope(QStringLiteral("browser.view"), this);
    m_commandContext.updateScope(scope, {{QStringLiteral("focusedViewId"), m_id}});
    m_commandContext.activate(scope);
    const QList<CommandDescriptor> descriptors{
        {QStringLiteral("browser.new-tab"), tr("New container tab"), QStringLiteral("plus"), tr("File"), {}, QStringLiteral("Ctrl+T")},
        {QStringLiteral("browser.new-split"), tr("New split…"), QStringLiteral("columns-2"), tr("File"), {}, QStringLiteral("Ctrl+Alt+T")},
        {QStringLiteral("browser.new-window"), tr("New window"), QStringLiteral("square-plus"), tr("File"), {}, QStringLiteral("Ctrl+N")},
        {QStringLiteral("browser.close"), tr("Close page"), QStringLiteral("x"), tr("File"), {}, QStringLiteral("Ctrl+W")},
        {QStringLiteral("browser.address"), tr("Focus address"), QStringLiteral("search"), tr("Navigate"), {}, QStringLiteral("Ctrl+L")},
        {QStringLiteral("browser.back"), tr("Back"), QStringLiteral("arrow-left"), tr("Navigate"), {}, QStringLiteral("Alt+Left")},
        {QStringLiteral("browser.forward"), tr("Forward"), QStringLiteral("arrow-right"), tr("Navigate"), {}, QStringLiteral("Alt+Right")},
        {QStringLiteral("browser.reload"), tr("Reload"), QStringLiteral("rotate-cw"), tr("Navigate"), {}, QStringLiteral("Ctrl+R")}};
    auto commands = descriptors;
    commands.append({QStringLiteral("browser.chrome"), tr("Show browser controls"), QStringLiteral("panel-top"), tr("View"), {}, QStringLiteral("Ctrl+Shift+H"), true});
    for (const auto &descriptor : commands) {
        m_commands.registerCommand(descriptor);
        const QString id = descriptor.id;
        m_commands.registerHandler(scope, id, this,
            [this, id](const CommandInvocationContext &, const QVariantMap &) -> OperationError {
                if (id == QStringLiteral("browser.new-tab")) m_controller->newView(this, QStringLiteral("tab"));
                else if (id == QStringLiteral("browser.new-split")) m_controller->beginSplit(this);
                else if (id == QStringLiteral("browser.new-window")) m_controller->newView();
                else if (id == QStringLiteral("browser.close")) m_window->close();
                else if (id == QStringLiteral("browser.address")) emit focusAddressRequested();
                else if (id == QStringLiteral("browser.back")) m_controller->traverse(this, -1);
                else if (id == QStringLiteral("browser.forward")) m_controller->traverse(this, 1);
                else if (id == QStringLiteral("browser.reload")) m_controller->reload(this);
                else if (id == QStringLiteral("browser.chrome")) setChromeVisible(!chromeVisible());
                return {};
            }, [this, id](const CommandInvocationContext &) {
                CommandState state;
                if (id == QStringLiteral("browser.chrome")) state.checked = chromeVisible();
                if (id == QStringLiteral("browser.back") || id == QStringLiteral("browser.forward") || id == QStringLiteral("browser.reload")) {
                    state.enabled = ready();
                    if (!state.enabled) state.disabledReason = tr("Gecko is starting");
                }
                return state;
            });
        // Browser commands must work while the address field or web form has
        // focus. ShortcutMap consumes them before forwarding input to Gecko.
        m_shortcuts.bindCommand(id, {}, true);
    }
}
}
