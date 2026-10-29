import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    allowedOrientations: Orientation.All

    Component.onCompleted: Launcher.startLogin()
    onStatusChanged: {
        if (status === PageStatus.Deactivating && Launcher.loginActive && !_done)
            Launcher.cancelLogin()
    }
    property bool _done

    Connections {
        target: Launcher
        onLoginSucceeded: {
            page._done = true
            pageStack.pop()
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        PullDownMenu {
            MenuItem {
                text: qsTr("Get a new code")
                onClicked: Launcher.startLogin()
            }
        }

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: qsTr("Microsoft sign in")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                color: Theme.highlightColor
                text: qsTr("Open the link on this phone or scan the QR code with another device, then sign in with the Microsoft account that owns Minecraft.")
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: Launcher.loginCode !== ""
                text: Launcher.loginCode
                font.pixelSize: Theme.fontSizeHuge
                font.family: "monospace"
                font.letterSpacing: 4
                color: Theme.highlightColor
            }

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: Launcher.loginUrl !== ""
                width: Math.min(page.width, page.height) * 0.6
                height: width
                color: "white"
                radius: Theme.paddingSmall

                Image {
                    anchors.fill: parent
                    anchors.margins: Theme.paddingMedium
                    smooth: false
                    sourceSize: Qt.size(width, height)
                    source: Launcher.loginUrl !== "" ? "image://sailmc/qr/" + encodeURIComponent(Launcher.loginUrl) : ""
                }
            }

            ButtonLayout {
                Button {
                    text: qsTr("Open in browser")
                    enabled: Launcher.loginUrl !== ""
                    onClicked: Qt.openUrlExternally(Launcher.loginUrl)
                }
                Button {
                    text: qsTr("Copy code")
                    enabled: Launcher.loginCode !== ""
                    onClicked: Clipboard.text = Launcher.loginCode
                }
            }

            ProgressBar {
                width: parent.width
                visible: Launcher.loginActive
                indeterminate: Launcher.loginCode === ""
                minimumValue: 0
                maximumValue: 900
                value: Launcher.loginSecondsLeft
                label: Launcher.loginCode !== ""
                       ? qsTr("Code valid for %1:%2").arg(Math.floor(Launcher.loginSecondsLeft / 60))
                                                    .arg(("0" + Launcher.loginSecondsLeft % 60).slice(-2))
                       : Launcher.loginStatus
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                text: Launcher.loginStatus
            }
        }
        VerticalScrollDecorator {}
    }
}
