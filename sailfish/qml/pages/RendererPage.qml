import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    property string current: Launcher.setting("SailRenderer")
    signal selected(string id)

    allowedOrientations: Orientation.All

    SilicaListView {
        id: list
        anchors.fill: parent
        model: Renderers.renderers

        header: Column {
            width: list.width
            PageHeader {
                title: qsTr("Renderer")
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
                text: qsTr("Minecraft needs desktop OpenGL. A renderer provides it on top of the phone's GPU driver. "
                           + "Put the libraries of a renderer (libGL.so.1, a Wayland libglfw.so…) in the folder shown below it.")
            }
            Item { width: 1; height: Theme.paddingLarge }
        }

        delegate: ListItem {
            contentHeight: col.height + 2 * Theme.paddingMedium
            highlighted: down || modelData.id === page.current
            onClicked: {
                Launcher.setSetting("SailRenderer", modelData.id)
                page.current = modelData.id
                page.selected(modelData.id)
            }

            Column {
                id: col
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                anchors.verticalCenter: parent.verticalCenter

                Label {
                    width: parent.width
                    text: modelData.name + (modelData.installed ? "" : "  ·  " + qsTr("not installed"))
                    color: highlighted ? Theme.highlightColor : Theme.primaryColor
                }
                Label {
                    width: parent.width
                    wrapMode: Text.Wrap
                    text: modelData.description
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                }
                Label {
                    width: parent.width
                    visible: modelData.folder !== ""
                    wrapMode: Text.WrapAnywhere
                    text: modelData.folder
                    font.pixelSize: Theme.fontSizeTiny
                    color: Theme.secondaryColor
                }
            }
        }
        VerticalScrollDecorator {}
    }
}
