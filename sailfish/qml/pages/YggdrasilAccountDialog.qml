import QtQuick 2.6
import Sailfish.Silica 1.0

// Account of a Yggdrasil server of your own (e.g. Drasl), used in the game through authlib-injector.
// Like offline accounts, only offered next to a Microsoft account that owns Minecraft.
Dialog {
    id: dialog

    allowedOrientations: Orientation.All
    canAccept: serverField.text.trim() !== "" && userField.text.trim() !== "" && passwordField.text !== ""
               && !Launcher.busy

    onAccepted: Launcher.addYggdrasilAccount(serverField.text.trim(), userField.text.trim(), passwordField.text)

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            DialogHeader {
                acceptText: qsTr("Sign in")
                title: qsTr("authlib-injector account")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                color: Theme.highlightColor
                text: qsTr("Sign in to a Yggdrasil authentication server, for example a Drasl server of your "
                           + "community. The game uses it through authlib-injector, so you can play on the "
                           + "servers that use it with its skins.")
            }

            TextField {
                id: serverField
                width: parent.width
                label: qsTr("Server address")
                placeholderText: "https://drasl.example.org/authlib-injector"
                inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: userField.focus = true
            }

            TextField {
                id: userField
                width: parent.width
                label: qsTr("User name or e-mail")
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: passwordField.focus = true
            }

            PasswordField {
                id: passwordField
                width: parent.width
                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: if (dialog.canAccept) dialog.accept()
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
                text: qsTr("The password is only sent to this server and is not stored. authlib-injector "
                           + "(github.com/yushijinhun/authlib-injector) is downloaded the first time you play.")
            }
        }
    }
}
