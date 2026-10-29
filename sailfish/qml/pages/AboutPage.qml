import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: qsTr("About")
            }

            Image {
                anchors.horizontalCenter: parent.horizontalCenter
                source: "/usr/share/icons/hicolor/172x172/apps/sailmc.png"
                width: Theme.iconSizeExtraLarge
                height: width
                sourceSize: Qt.size(width, height)
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "SailMC " + Launcher.version
                font.pixelSize: Theme.fontSizeLarge
                color: Theme.highlightColor
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                text: qsTr("A native Sailfish OS fork of the MultiMC launcher. The launcher core (instances, Microsoft accounts, "
                           + "downloads, mod loaders) is MultiMC, the interface is written with Silica and was inspired by MojoLauncher for Android.")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
                text: qsTr("MultiMC is © MultiMC Contributors and licensed under the Apache License 2.0. "
                           + "Minecraft is a trademark of Mojang AB. SailMC is not affiliated with Mojang, Microsoft or MultiMC.")
            }

            ButtonLayout {
                Button {
                    text: qsTr("MultiMC")
                    onClicked: Qt.openUrlExternally("https://multimc.org")
                }
            }
        }
    }
}
