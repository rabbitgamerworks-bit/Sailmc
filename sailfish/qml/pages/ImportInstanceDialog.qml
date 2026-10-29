import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0
import SailMC 1.0

// "Add instance locally": a Minecraft client .jar or an exported instance .zip stored on the device
Dialog {
    id: dialog

    property string filePath
    property string minecraftVersion
    readonly property bool isJar: /\.jar$/i.test(filePath)
    readonly property bool isZip: /\.zip$/i.test(filePath)

    allowedOrientations: Orientation.All
    canAccept: !Launcher.busy && (isZip || (isJar && minecraftVersion !== ""))

    onAccepted: Launcher.importInstance(filePath, nameField.text, minecraftVersion)

    VersionModel {
        id: gameVersions
        uid: "net.minecraft"
    }

    Component {
        id: filePicker
        FilePickerPage {
            nameFilters: ["*.jar", "*.zip"]
            onSelectedContentPropertiesChanged: {
                dialog.filePath = selectedContentProperties.filePath
                dialog.minecraftVersion = dialog.isJar ? Launcher.jarMinecraftVersion(dialog.filePath) : ""
            }
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            DialogHeader {
                acceptText: qsTr("Add")
                title: qsTr("Add instance locally")
            }

            ValueButton {
                label: qsTr("File")
                value: dialog.filePath !== "" ? dialog.filePath.split("/").pop() : qsTr("Choose")
                description: qsTr("A Minecraft .jar or an instance exported from MultiMC / Prism Launcher (.zip)")
                onClicked: pageStack.push(filePicker)
            }

            TextField {
                id: nameField
                width: parent.width
                label: qsTr("Name")
                placeholderText: dialog.minecraftVersion !== "" ? dialog.minecraftVersion : qsTr("Instance name")
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
            }

            ValueButton {
                visible: dialog.isJar
                label: qsTr("Minecraft version")
                value: dialog.minecraftVersion !== "" ? dialog.minecraftVersion : qsTr("Choose")
                description: qsTr("Read from the jar when it says (1.14 and newer), otherwise choose the version it is")
                onClicked: {
                    var picker = pageStack.push(Qt.resolvedUrl("VersionPickerPage.qml"),
                                                { model: gameVersions, title: qsTr("Minecraft version"), showFilters: true })
                    picker.picked.connect(function(version) { dialog.minecraftVersion = version })
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
                text: qsTr("The jar from the device is used instead of downloading it. Libraries and assets are "
                           + "still downloaded the first time, and playing needs your Microsoft account as usual.")
            }
        }
    }
}
