import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    // MessageLevel::Enum of the launcher core
    function levelColor(level) {
        switch (level) {
        case 3: return Theme.highlightColor          // Launcher
        case 7: return "#e6c34f"                     // Warning
        case 8:                                      // Error
        case 9: return Theme.errorColor              // Fatal
        case 4: return Theme.secondaryColor          // Debug
        default: return Theme.primaryColor
        }
    }

    allowedOrientations: Orientation.All

    SilicaListView {
        id: list
        anchors.fill: parent
        model: Launcher.log
        clip: true

        property bool follow: true
        onCountChanged: if (follow) positionViewAtEnd()
        onMovementEnded: follow = atYEnd

        header: PageHeader {
            title: qsTr("Game log")
            description: Launcher.gameRunning ? qsTr("Running") : ""
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("Stop the game")
                visible: Launcher.gameRunning
                onClicked: Launcher.kill()
            }
            MenuItem {
                text: qsTr("Copy all")
                onClicked: Clipboard.text = Launcher.logText()
            }
        }

        delegate: Label {
            x: Theme.paddingMedium
            width: list.width - 2 * x
            text: line
            wrapMode: Text.WrapAnywhere
            font.family: "monospace"
            font.pixelSize: Theme.fontSizeTiny
            color: page.levelColor(level)
        }

        ViewPlaceholder {
            enabled: list.count === 0
            text: qsTr("No game started yet")
            hintText: qsTr("The output of Minecraft appears here")
        }

        VerticalScrollDecorator {}
    }
}
