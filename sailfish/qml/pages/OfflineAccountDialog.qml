import QtQuick 2.6
import Sailfish.Silica 1.0

// Offline account, ported from Prism Launcher (only reachable when an account owning Minecraft exists)
Dialog {
    id: dialog

    canAccept: /^[A-Za-z0-9_]{3,16}$/.test(nameField.text.trim())
    onAccepted: {
        var message = Launcher.addOfflineAccount(nameField.text)
        if (message !== "")
            app.notify(message)
    }

    Column {
        width: parent.width

        DialogHeader {
            acceptText: qsTr("Add")
            title: qsTr("Offline account")
        }
        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            wrapMode: Text.Wrap
            color: Theme.highlightColor
            text: qsTr("Offline accounts play without signing in, for example in single player or on LAN. Online servers will not accept them.")
        }
        TextField {
            id: nameField
            width: parent.width
            label: qsTr("Player name (3-16 letters, digits or _)")
            placeholderText: qsTr("Player name")
            focus: true
            inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
            EnterKey.enabled: dialog.canAccept
            EnterKey.onClicked: dialog.accept()
        }
    }
}
