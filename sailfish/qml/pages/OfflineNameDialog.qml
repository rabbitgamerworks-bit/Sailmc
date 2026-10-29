import QtQuick 2.6
import Sailfish.Silica 1.0

Dialog {
    id: dialog

    property string instanceId
    property string playerName

    canAccept: nameField.text.trim().length >= 3
    onAccepted: Launcher.launch(instanceId, true, nameField.text.trim())

    Column {
        width: parent.width

        DialogHeader {
            acceptText: qsTr("Play")
            title: qsTr("Play offline")
        }
        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            wrapMode: Text.Wrap
            color: Theme.highlightColor
            text: qsTr("Your account can't be verified right now. Choose the player name to use in offline mode.")
        }
        TextField {
            id: nameField
            width: parent.width
            label: qsTr("Player name")
            text: dialog.playerName
            focus: true
            inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
        }
    }
}
