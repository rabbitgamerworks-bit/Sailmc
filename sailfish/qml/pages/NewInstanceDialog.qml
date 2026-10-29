import QtQuick 2.6
import Sailfish.Silica 1.0
import SailMC 1.0

Dialog {
    id: dialog

    property string minecraftVersion
    property string loaderVersion
    readonly property var loaders: [
        { name: qsTr("None (vanilla)"), uid: "" },
        { name: "Fabric", uid: "net.fabricmc.fabric-loader" },
        { name: "Quilt", uid: "org.quiltmc.quilt-loader" },
        { name: "Forge", uid: "net.minecraftforge" },
        { name: "NeoForge", uid: "net.neoforged" }
    ]
    readonly property string loaderUid: loaders[loaderBox.currentIndex].uid

    allowedOrientations: Orientation.All
    canAccept: minecraftVersion !== "" && !Launcher.busy && (loaderUid === "" || loaderVersion !== "")

    onAccepted: Launcher.createInstance(nameField.text, minecraftVersion, loaderUid, loaderVersion)

    VersionModel {
        id: gameVersions
        uid: "net.minecraft"
        onCountChanged: {
            if (dialog.minecraftVersion === "" && count > 0)
                dialog.minecraftVersion = versionAt(recommendedIndex)
        }
    }

    VersionModel {
        id: loaderVersions
        uid: dialog.loaderUid
        minecraftVersion: dialog.minecraftVersion
        onCountChanged: dialog.loaderVersion = count > 0 ? versionAt(recommendedIndex) : ""
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            DialogHeader {
                acceptText: qsTr("Create")
                title: qsTr("New instance")
            }

            TextField {
                id: nameField
                width: parent.width
                label: qsTr("Name")
                placeholderText: dialog.minecraftVersion !== "" ? qsTr("Minecraft %1").arg(dialog.minecraftVersion)
                                                                 : qsTr("Instance name")
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
            }

            ValueButton {
                label: qsTr("Minecraft version")
                value: gameVersions.loading && dialog.minecraftVersion === "" ? qsTr("Loading…")
                                                                              : dialog.minecraftVersion
                description: gameVersions.error !== "" ? gameVersions.error : ""
                onClicked: {
                    var picker = pageStack.push(Qt.resolvedUrl("VersionPickerPage.qml"),
                                                { model: gameVersions, title: qsTr("Minecraft version"), showFilters: true })
                    picker.picked.connect(function(version) { dialog.minecraftVersion = version })
                }
            }

            ComboBox {
                id: loaderBox
                label: qsTr("Mod loader")
                menu: ContextMenu {
                    Repeater {
                        model: dialog.loaders
                        MenuItem { text: modelData.name }
                    }
                }
            }

            ValueButton {
                visible: dialog.loaderUid !== ""
                label: qsTr("Loader version")
                value: loaderVersions.loading && dialog.loaderVersion === ""
                       ? qsTr("Loading…")
                       : (dialog.loaderVersion !== "" ? dialog.loaderVersion : qsTr("Not available for this version"))
                onClicked: {
                    var picker = pageStack.push(Qt.resolvedUrl("VersionPickerPage.qml"),
                                                { model: loaderVersions, title: loaderBox.value, showFilters: false })
                    picker.picked.connect(function(version) { dialog.loaderVersion = version })
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
                text: qsTr("The game files are downloaded when you play the instance for the first time. "
                           + "This version needs Java %1.").arg(JavaRuntimes.recommendedMajorFor(dialog.minecraftVersion))
            }
        }
    }
}
