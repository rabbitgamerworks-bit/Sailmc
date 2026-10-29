import QtQuick 2.6
import Sailfish.Silica 1.0

CoverBackground {
    readonly property bool ready: startupError === ""

    Column {
        anchors.centerIn: parent
        width: parent.width - 2 * Theme.paddingLarge
        spacing: Theme.paddingMedium
        visible: ready

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Theme.iconSizeLarge
            height: width
            sourceSize: Qt.size(width, height)
            smooth: false
            source: ready && Launcher.selectedIconKey !== ""
                    ? "image://sailmc/instance/" + encodeURIComponent(Launcher.selectedIconKey)
                    : "/usr/share/icons/hicolor/86x86/apps/sailmc.png"
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            truncationMode: TruncationMode.Fade
            text: ready ? (Launcher.selectedName || "SailMC") : ""
            color: Theme.highlightColor
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            maximumLineCount: 3
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
            text: !ready ? "" : Launcher.busy
                  ? (Launcher.taskProgress >= 0 ? Math.round(Launcher.taskProgress * 100) + " % · " : "") + Launcher.taskTitle
                  : (Launcher.gameRunning ? qsTr("Playing") : Launcher.selectedGameVersion)
        }
    }

    CoverActionList {
        enabled: ready && Launcher.selectedInstanceId !== "" && !Launcher.busy
        CoverAction {
            iconSource: Launcher.gameRunning ? "image://theme/icon-cover-cancel" : "image://theme/icon-cover-play"
            onTriggered: Launcher.gameRunning ? Launcher.kill() : Launcher.launch(Launcher.selectedInstanceId)
        }
    }
}
