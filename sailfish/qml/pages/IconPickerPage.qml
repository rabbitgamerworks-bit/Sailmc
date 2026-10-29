import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    property string instanceId

    allowedOrientations: Orientation.All

    SilicaGridView {
        id: grid
        anchors.fill: parent
        cellWidth: width / Math.floor(width / (Theme.iconSizeExtraLarge + Theme.paddingLarge))
        cellHeight: cellWidth
        model: Launcher.iconKeys()

        header: PageHeader {
            title: qsTr("Instance icon")
        }

        delegate: BackgroundItem {
            width: grid.cellWidth
            height: grid.cellHeight
            onClicked: {
                Launcher.setInstanceIcon(page.instanceId, modelData)
                pageStack.pop()
            }
            Image {
                anchors.centerIn: parent
                width: Theme.iconSizeLarge
                height: width
                sourceSize: Qt.size(width, height)
                smooth: false
                source: "image://sailmc/instance/" + encodeURIComponent(modelData)
            }
        }
        VerticalScrollDecorator {}
    }
}
