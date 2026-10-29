import QtQuick 2.6
import Sailfish.Silica 1.0

// One instance ("profile" in MojoLauncher) in the horizontal selector on the home page
BackgroundItem {
    id: tile

    property bool selected
    property string iconKey
    property string title
    property string subtitle

    width: Theme.itemSizeHuge * 1.15
    height: Theme.itemSizeHuge * 1.5

    Rectangle {
        anchors {
            fill: parent
            margins: Theme.paddingSmall
        }
        radius: Theme.paddingSmall
        color: tile.selected ? Theme.rgba(Theme.highlightBackgroundColor, 0.35) : Theme.rgba(Theme.primaryColor, 0.06)
        border.color: Theme.highlightColor
        border.width: tile.selected ? 2 : 0
    }

    Column {
        anchors.centerIn: parent
        width: parent.width - 2 * Theme.paddingMedium
        spacing: Theme.paddingSmall

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Theme.iconSizeLarge
            height: width
            sourceSize: Qt.size(width, height)
            source: tile.iconKey !== "" ? "image://sailmc/instance/" + encodeURIComponent(tile.iconKey) : ""
            smooth: false
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            truncationMode: TruncationMode.Fade
            text: tile.title
            font.pixelSize: Theme.fontSizeSmall
            color: tile.selected || tile.highlighted ? Theme.highlightColor : Theme.primaryColor
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            truncationMode: TruncationMode.Fade
            text: tile.subtitle
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
        }
    }
}
