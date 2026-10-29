import QtQuick 2.6
import Sailfish.Silica 1.0

// Short in-app notification shown on top of the current page
MouseArea {
    id: root

    function show(text) {
        label.text = text
        root.opacity = 1
        hideTimer.restart()
    }

    z: 1000
    width: parent.width
    height: label.height + 2 * Theme.paddingLarge
    opacity: 0
    visible: opacity > 0
    onClicked: opacity = 0

    Behavior on opacity { FadeAnimator {} }

    Rectangle {
        anchors.fill: parent
        color: Theme.highlightDimmerColor
        opacity: 0.95
    }

    Label {
        id: label
        x: Theme.horizontalPageMargin
        width: parent.width - 2 * x
        anchors.verticalCenter: parent.verticalCenter
        wrapMode: Text.Wrap
        color: Theme.highlightColor
        font.pixelSize: Theme.fontSizeSmall
    }

    Timer {
        id: hideTimer
        interval: 4000
        onTriggered: root.opacity = 0
    }
}
