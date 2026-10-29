import QtQuick 2.6
import Sailfish.Silica 1.0
import SailMC 1.0

Page {
    id: page

    property alias instanceId: mods.instanceId
    property alias kind: mods.kind
    property string title

    allowedOrientations: Orientation.All

    ModsModel {
        id: mods
    }

    SilicaListView {
        id: list
        anchors.fill: parent
        model: mods

        header: PageHeader {
            title: page.title
            description: qsTr("%n item(s)", "", mods.count)
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("Open folder")
                onClicked: Qt.openUrlExternally("file://" + mods.folder)
            }
            MenuItem {
                text: qsTr("Refresh")
                onClicked: mods.refresh()
            }
        }

        delegate: ListItem {
            id: item
            contentHeight: modSwitch.height
            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Delete")
                    onClicked: item.remorseDelete(function() { mods.remove(index) })
                }
            }

            TextSwitch {
                id: modSwitch
                text: name
                description: version !== "" ? version + " · " + fileName : fileName
                checked: enabled
                automaticCheck: false
                onClicked: mods.setEnabled(index, !checked)
            }
        }

        ViewPlaceholder {
            enabled: list.count === 0
            text: qsTr("Nothing here yet")
            hintText: qsTr("Copy .jar or .zip files to the folder of this instance (pull down to open it)")
        }

        VerticalScrollDecorator {}
    }
}
