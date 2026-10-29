import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    property string instanceId
    property var info: Launcher.instanceInfo(instanceId)

    function reload() {
        info = Launcher.instanceInfo(instanceId)
    }

    allowedOrientations: Orientation.All

    Connections {
        target: Launcher
        onSelectedChanged: page.reload()
        onGameChanged: page.reload()
    }

    RemorsePopup {
        id: remorse
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        PullDownMenu {
            MenuItem {
                text: qsTr("Delete")
                enabled: !info.running
                onClicked: remorse.execute(qsTr("Deleting %1").arg(info.name), function() {
                    var id = page.instanceId
                    pageStack.pop()
                    Launcher.deleteInstance(id)
                })
            }
            MenuItem {
                text: qsTr("Rename")
                onClicked: pageStack.push(renameDialog)
            }
            MenuItem {
                text: qsTr("Change icon")
                onClicked: pageStack.push(Qt.resolvedUrl("IconPickerPage.qml"), { instanceId: page.instanceId })
            }
            MenuItem {
                text: qsTr("Download game files")
                enabled: !Launcher.busy
                onClicked: Launcher.updateInstance(page.instanceId)
            }
        }

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: info.name || ""
                description: info.loader ? qsTr("Minecraft %1 · %2").arg(info.gameVersion).arg(info.loader)
                                         : qsTr("Minecraft %1").arg(info.gameVersion)
            }

            Image {
                anchors.horizontalCenter: parent.horizontalCenter
                width: Theme.iconSizeExtraLarge
                height: width
                sourceSize: Qt.size(width, height)
                smooth: false
                source: "image://sailmc/instance/" + encodeURIComponent(info.iconKey || "")
            }

            Item { width: 1; height: Theme.paddingLarge }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                preferredWidth: Theme.buttonWidthLarge
                text: info.running ? qsTr("Stop") : qsTr("Play")
                enabled: !Launcher.busy || info.running
                onClicked: {
                    if (info.running) {
                        Launcher.kill()
                    } else {
                        Launcher.selectedInstanceId = page.instanceId
                        Launcher.launch(page.instanceId)
                    }
                }
            }

            ProgressBar {
                width: parent.width
                visible: Launcher.busy
                indeterminate: Launcher.taskProgress < 0
                value: Math.max(0, Launcher.taskProgress)
                label: Launcher.taskStatus || Launcher.taskTitle
            }

            SectionHeader { text: qsTr("Details") }

            DetailItem {
                label: qsTr("Minecraft")
                value: info.gameVersion || "-"
            }
            DetailItem {
                label: qsTr("Mod loader")
                value: info.loader || qsTr("None")
            }
            DetailItem {
                label: qsTr("Played")
                value: info.playTime || qsTr("Never")
            }
            DetailItem {
                label: qsTr("Last played")
                value: info.lastLaunch ? Format.formatDate(info.lastLaunch, Formatter.TimepointRelative) : qsTr("Never")
            }
            DetailItem {
                label: qsTr("Java")
                value: qsTr("Java %1 recommended").arg(info.recommendedJava)
            }

            SectionHeader { text: qsTr("Content") }

            BackgroundItem {
                Label {
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Mods")
                    color: parent.highlighted ? Theme.highlightColor : Theme.primaryColor
                }
                onClicked: pageStack.push(Qt.resolvedUrl("ModsPage.qml"), { instanceId: page.instanceId, kind: "mods", title: qsTr("Mods") })
            }
            BackgroundItem {
                Label {
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Resource packs")
                    color: parent.highlighted ? Theme.highlightColor : Theme.primaryColor
                }
                onClicked: pageStack.push(Qt.resolvedUrl("ModsPage.qml"), { instanceId: page.instanceId, kind: "resourcepacks", title: qsTr("Resource packs") })
            }
            BackgroundItem {
                Label {
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Instance settings")
                    color: parent.highlighted ? Theme.highlightColor : Theme.primaryColor
                }
                onClicked: pageStack.push(Qt.resolvedUrl("InstanceSettingsPage.qml"), { instanceId: page.instanceId })
            }
            BackgroundItem {
                Label {
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Game log")
                    color: parent.highlighted ? Theme.highlightColor : Theme.primaryColor
                }
                onClicked: pageStack.push(Qt.resolvedUrl("ConsolePage.qml"))
            }

            SectionHeader { text: qsTr("Notes") }

            TextArea {
                width: parent.width
                placeholderText: qsTr("Notes about this instance")
                text: info.notes || ""
                onActiveFocusChanged: if (!activeFocus) Launcher.setInstanceSetting(page.instanceId, "notes", text)
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.WrapAnywhere
                font.pixelSize: Theme.fontSizeTiny
                color: Theme.secondaryColor
                text: info.folder || ""
            }
        }
        VerticalScrollDecorator {}
    }

    Component {
        id: renameDialog
        Dialog {
            onAccepted: Launcher.renameInstance(page.instanceId, nameField.text)
            Column {
                width: parent.width
                DialogHeader { title: qsTr("Rename instance") }
                TextField {
                    id: nameField
                    width: parent.width
                    label: qsTr("Name")
                    text: info.name
                    focus: true
                }
            }
        }
    }
}
