import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    property int suggestedMajor: 0
    property string current: Launcher.setting("JavaPath")

    allowedOrientations: Orientation.All

    Component.onCompleted: JavaRuntimes.refresh()

    Connections {
        target: JavaRuntimes
        onInstalled: {
            if (page.current === "" || majorVersion === page.suggestedMajor) {
                Launcher.setSetting("JavaPath", javaPath)
                page.current = javaPath
            }
        }
        onFailed: app.notify(reason)
    }

    SilicaListView {
        id: list
        anchors.fill: parent
        model: JavaRuntimes.runtimes

        header: Column {
            width: list.width

            PageHeader {
                title: qsTr("Java runtimes")
                description: qsTr("Device architecture: %1").arg(JavaRuntimes.architecture)
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: page.suggestedMajor > 0
                wrapMode: Text.Wrap
                color: Theme.highlightColor
                text: qsTr("This Minecraft version needs Java %1. Pull down to download it.").arg(page.suggestedMajor)
            }

            ProgressBar {
                width: parent.width
                visible: JavaRuntimes.busy
                indeterminate: JavaRuntimes.progress <= 0
                value: JavaRuntimes.progress
                label: JavaRuntimes.status
            }

            TextSwitch {
                text: qsTr("Pick automatically")
                description: qsTr("Use the best installed runtime for each Minecraft version")
                checked: page.current === ""
                onClicked: {
                    Launcher.setSetting("JavaPath", "")
                    page.current = ""
                }
            }
        }

        PullDownMenu {
            busy: JavaRuntimes.busy
            MenuItem {
                text: qsTr("Download Java 8 (Minecraft 1.16 and older)")
                enabled: !JavaRuntimes.busy && JavaRuntimes.downloadSupported
                onClicked: JavaRuntimes.download(8)
            }
            MenuItem {
                text: qsTr("Download Java 17 (1.17 – 1.20.4)")
                enabled: !JavaRuntimes.busy && JavaRuntimes.downloadSupported
                onClicked: JavaRuntimes.download(17)
            }
            MenuItem {
                text: qsTr("Download Java 21 (1.20.5 – 1.21.x)")
                enabled: !JavaRuntimes.busy && JavaRuntimes.downloadSupported
                onClicked: JavaRuntimes.download(21)
            }
            MenuItem {
                text: qsTr("Download Java 25 (26.1 and newer)")
                enabled: !JavaRuntimes.busy && JavaRuntimes.downloadSupported
                onClicked: JavaRuntimes.download(25)
            }
            MenuItem {
                text: JavaRuntimes.busy ? qsTr("Cancel download") : qsTr("Rescan")
                onClicked: JavaRuntimes.busy ? JavaRuntimes.cancel() : JavaRuntimes.refresh()
            }
        }

        delegate: ListItem {
            id: item
            contentHeight: Theme.itemSizeMedium
            highlighted: down || modelData.path === page.current
            onClicked: {
                Launcher.setSetting("JavaPath", modelData.path)
                page.current = modelData.path
            }
            menu: modelData.managed ? removeMenu : null

            Component {
                id: removeMenu
                ContextMenu {
                    MenuItem {
                        text: qsTr("Remove")
                        onClicked: item.remorseDelete(function() {
                            if (page.current === modelData.path) {
                                Launcher.setSetting("JavaPath", "")
                                page.current = ""
                            }
                            JavaRuntimes.remove(modelData.path)
                        })
                    }
                }
            }

            Column {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                anchors.verticalCenter: parent.verticalCenter
                Label {
                    text: qsTr("Java %1").arg(modelData.major) + (modelData.version ? "  (" + modelData.version + ")" : "")
                    color: item.highlighted ? Theme.highlightColor : Theme.primaryColor
                }
                Label {
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    text: (modelData.managed ? qsTr("Installed by SailMC") + " · " : "") + modelData.path
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                }
            }
        }

        ViewPlaceholder {
            verticalOffset: list.headerItem ? list.headerItem.height / 2 : 0
            enabled: list.count === 0 && !JavaRuntimes.busy
            text: qsTr("No Java found")
            hintText: JavaRuntimes.downloadSupported
                      ? qsTr("Pull down to download a Java runtime for this device")
                      : qsTr("Automatic downloads are not available for %1. Install a JRE to /opt or /usr/lib/jvm.").arg(JavaRuntimes.architecture)
        }

        VerticalScrollDecorator {}
    }
}
