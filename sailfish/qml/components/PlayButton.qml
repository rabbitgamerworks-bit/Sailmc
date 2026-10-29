import QtQuick 2.6
import Sailfish.Silica 1.0

// The big launch button at the bottom of the home page, like the one of MojoLauncher,
// drawn with the ambience highlight colours so it fits every Sailfish theme.
BackgroundItem {
    id: root

    property bool running: Launcher.gameRunning
    property bool busy: Launcher.busy
    property string instanceName: Launcher.selectedName

    height: Theme.itemSizeHuge
    enabled: instanceName !== "" || running
    highlightedColor: "transparent"

    Rectangle {
        id: face
        anchors {
            fill: parent
            leftMargin: Theme.horizontalPageMargin
            rightMargin: Theme.horizontalPageMargin
            topMargin: Theme.paddingMedium
            bottomMargin: Theme.paddingMedium
        }
        radius: Theme.paddingMedium
        color: root.running ? Theme.rgba(Theme.errorColor, root.down ? 0.6 : 0.45)
                            : Theme.rgba(Theme.highlightBackgroundColor, root.down ? 0.75 : 0.5)
        border.color: Theme.rgba(Theme.highlightColor, 0.6)
        border.width: root.down ? 2 : 0
        opacity: root.enabled ? 1.0 : 0.4

        // progress of downloads/launch preparation fills the button from the left
        Rectangle {
            anchors {
                left: parent.left
                top: parent.top
                bottom: parent.bottom
            }
            radius: parent.radius
            visible: root.busy
            width: Launcher.taskProgress >= 0 ? parent.width * Launcher.taskProgress : parent.width
            color: Theme.rgba(Theme.highlightColor, Launcher.taskProgress >= 0 ? 0.35 : 0.15)
            Behavior on width { SmoothedAnimation { velocity: 400 } }
            SequentialAnimation on opacity {
                running: root.busy && Launcher.taskProgress < 0
                loops: Animation.Infinite
                NumberAnimation { from: 0.4; to: 1.0; duration: 700 }
                NumberAnimation { from: 1.0; to: 0.4; duration: 700 }
            }
        }

        Row {
            anchors.centerIn: parent
            spacing: Theme.paddingMedium

            Image {
                anchors.verticalCenter: parent.verticalCenter
                source: (root.running ? "image://theme/icon-m-clear" : "image://theme/icon-m-play")
                        + "?" + Theme.primaryColor
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                Label {
                    text: root.running ? qsTr("STOP") : qsTr("PLAY")
                    font.pixelSize: Theme.fontSizeExtraLarge
                    font.bold: true
                    font.letterSpacing: 2
                }
                Label {
                    width: Math.min(implicitWidth, face.width - 3 * Theme.itemSizeSmall)
                    truncationMode: TruncationMode.Fade
                    text: root.busy ? Launcher.taskStatus || Launcher.taskTitle
                                    : (root.running ? qsTr("%1 is running").arg(root.instanceName) : root.instanceName)
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                    visible: text !== ""
                }
            }
        }
    }
}
