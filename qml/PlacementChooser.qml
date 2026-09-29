// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QindaTK as Tk
import QindaWeb 1.0
Tk.Dialog {
    id: chooser
    required property BrowserView view
    objectName: "browserPlacementChooser"
    modal: true
    focus: true
    anchors.centerIn: parent
    padding: Tk.Theme.space.lg
    closePolicy: Tk.Dialog.CloseOnEscape
    property bool confirmed: false
    title: qsTr("Place the new split")
    primaryText: ""
    secondaryText: qsTr("Keep as separate window")
    function choose(direction) {
        confirmed = true
        view.confirmSplit(direction)
        close()
    }
    onOpened: { confirmed = false; choices.forceActiveFocus() }
    onClosed: if (!confirmed) view.cancelSplit()
    Tk.Flex {
        id: choices
        objectName: "browserSplitChoices"
        direction: Tk.Flex.Column
        gap: Tk.Theme.space.md
        focus: true
        Keys.onLeftPressed: chooser.choose("left")
        Keys.onRightPressed: chooser.choose("right")
        Keys.onUpPressed: chooser.choose("up")
        Keys.onDownPressed: chooser.choose("down")
        Tk.Label { text: qsTr("Choose a side, or use an arrow key.") }
        Tk.Flex {
            direction: Tk.Flex.Row
            gap: Tk.Theme.space.sm
            Tk.Button { text: qsTr("Left"); iconName: "arrow-left"; onClicked: chooser.choose("left") }
            Tk.Button { text: qsTr("Above"); iconName: "arrow-up"; onClicked: chooser.choose("up") }
            Tk.Button { text: qsTr("Below"); iconName: "arrow-down"; onClicked: chooser.choose("down") }
            Tk.Button { text: qsTr("Right"); iconName: "arrow-right"; onClicked: chooser.choose("right") }
        }
    }
}
