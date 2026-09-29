// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtWayland.Compositor
import QindaTK as Tk
import QindaTK.QindaQt as TkQt
import QindaWeb 1.0

Tk.AppWindow {
    id: window
    required property BrowserView view
    required property BrowserController browser
    objectName: "browserWindow-" + view.id
    title: view.title + " — QindaWeb"
    TkQt.QindaQtTheme { }
    menuBar: Tk.MenuBar {
        Tk.Menu {
            title: qsTr("&File")
            Tk.MenuItem { commandRegistry: view.commands; shortcutMap: view.shortcuts; commandId: "browser.new-tab" }
            Tk.MenuItem { commandRegistry: view.commands; shortcutMap: view.shortcuts; commandId: "browser.new-split" }
            Tk.MenuItem { commandRegistry: view.commands; shortcutMap: view.shortcuts; commandId: "browser.new-window" }
            Tk.MenuSeparator { }
            Tk.MenuItem { commandRegistry: view.commands; shortcutMap: view.shortcuts; commandId: "browser.close" }
        }
        Tk.Menu {
            title: qsTr("&Navigate")
            Tk.MenuItem { commandRegistry: view.commands; shortcutMap: view.shortcuts; commandId: "browser.back" }
            Tk.MenuItem { commandRegistry: view.commands; shortcutMap: view.shortcuts; commandId: "browser.forward" }
            Tk.MenuItem { commandRegistry: view.commands; shortcutMap: view.shortcuts; commandId: "browser.reload" }
            Tk.MenuItem { commandRegistry: view.commands; shortcutMap: view.shortcuts; commandId: "browser.address" }
        }
    }
    toolBars: Tk.ToolBar {
        Tk.CommandButton { registry: view.commands; shortcutMap: view.shortcuts; commandId: "browser.back"; iconOnly: true }
        Tk.CommandButton { registry: view.commands; shortcutMap: view.shortcuts; commandId: "browser.forward"; iconOnly: true }
        Tk.CommandButton { registry: view.commands; shortcutMap: view.shortcuts; commandId: "browser.reload"; iconOnly: true }
        Tk.TextField {
            id: address
            objectName: "browserAddress"
            placeholderText: qsTr("Search or enter address")
            tooltip: qsTr("Search or enter address")
            Accessible.name: qsTr("Address and search")
            Tk.Flex.grow: 1
            Tk.Flex.basis: 0
            text: view.url.toString() === "about:blank" ? "" : view.url.toString()
            onAccepted: if (view.navigate(text)) page.takeFocus()
            Keys.onEscapePressed: {
                text = view.url.toString() === "about:blank" ? "" : view.url.toString()
                page.takeFocus()
            }
        }
        Tk.CommandButton { registry: view.commands; shortcutMap: view.shortcuts; commandId: "browser.new-tab"; iconOnly: true }
        Tk.CommandButton { registry: view.commands; shortcutMap: view.shortcuts; commandId: "browser.new-split"; iconOnly: true }
    }
    Item {
        id: content
        objectName: "browserContent"
        anchors.fill: parent
        // ShellSurfaceItem forwards real input and owns Gecko popup items.
        // It paints wl_buffers; there is no screenshot or video transport.
        ShellSurfaceItem {
            id: page
            objectName: "geckoContent"
            shellSurface: view.shellSurface
            output: view.output
            autoCreatePopupItems: true
            touchEventsEnabled: true
            focusOnClick: true
            visible: view.shellSurface !== null
        }
        onWidthChanged: view.resizeContent(width, height)
        onHeightChanged: view.resizeContent(width, height)
        Component.onCompleted: view.resizeContent(width, height)
        Tk.Label {
            anchors.centerIn: parent
            visible: view.shellSurface === null
            text: view.message
            Accessible.role: Accessible.StaticText
        }
    }
    statusBar: Tk.ToolBar {
        compact: true
        edge: "top"
        Tk.Label {
            objectName: "browserStatus"
            text: view.message.length > 0 ? view.message : view.loading ? qsTr("Loading…") : ""
            elide: Text.ElideRight
            Tk.Flex.grow: 1
            Tk.Flex.basis: 0
        }
        Tk.Caption { text: browser.engineIdentity; Accessible.name: qsTr("Web engine: ") + browser.engineIdentity }
    }
    PlacementChooser { id: placement; view: window.view; parent: window.contentItem }
    Connections {
        target: view
        function onFocusAddressRequested() { address.forceActiveFocus(); address.selectAll() }
        function onPlacementRequested() { placement.open() }
        function onChanged() {
            if (!address.activeFocus)
                address.text = view.url.toString() === "about:blank" ? "" : view.url.toString()
        }
    }
    Component.onCompleted: address.forceActiveFocus()
}
