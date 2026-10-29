import QtQuick 2.6
import Sailfish.Silica 1.0
import "../components"

Page {
    id: page

    allowedOrientations: Orientation.All

    function play() {
        if (Launcher.gameRunning) {
            Launcher.kill()
        } else if (Launcher.selectedInstanceId === "") {
            pageStack.push(Qt.resolvedUrl("NewInstanceDialog.qml"))
        } else {
            Launcher.launch(Launcher.selectedInstanceId)
        }
    }

    Component.onCompleted: News.reload()

    SilicaFlickable {
        id: flickable
        anchors {
            fill: parent
            bottomMargin: playPanel.height
        }
        clip: true
        contentHeight: column.height + Theme.paddingLarge

        PullDownMenu {
            MenuItem {
                text: qsTr("Settings")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }
            MenuItem {
                text: qsTr("Java runtimes")
                onClicked: pageStack.push(Qt.resolvedUrl("JavaPage.qml"))
            }
            MenuItem {
                text: qsTr("Accounts")
                onClicked: pageStack.push(Qt.resolvedUrl("AccountsPage.qml"))
            }
            MenuItem {
                text: qsTr("Add instance locally")
                onClicked: pageStack.push(Qt.resolvedUrl("ImportInstanceDialog.qml"))
            }
            MenuItem {
                text: qsTr("New instance")
                onClicked: pageStack.push(Qt.resolvedUrl("NewInstanceDialog.qml"))
            }
        }

        PushUpMenu {
            MenuItem {
                text: qsTr("Game log")
                onClicked: pageStack.push(Qt.resolvedUrl("ConsolePage.qml"))
            }
            MenuItem {
                text: qsTr("About SailMC")
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }
        }

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: "SailMC"
                description: qsTr("Minecraft launcher")
            }

            // ---- account, top left in MojoLauncher
            BackgroundItem {
                id: accountItem
                height: Theme.itemSizeLarge
                onClicked: pageStack.push(Qt.resolvedUrl("AccountsPage.qml"))

                Rectangle {
                    id: avatar
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    width: Theme.iconSizeLarge
                    height: width
                    radius: Theme.paddingSmall
                    color: Theme.rgba(Theme.highlightBackgroundColor, 0.3)

                    Image {
                        anchors.fill: parent
                        anchors.margins: Accounts.defaultAccountId !== "" ? 0 : Theme.paddingSmall
                        smooth: false
                        source: Accounts.defaultAccountId !== ""
                                ? "image://sailmc/face/" + Accounts.defaultAccountId
                                : "image://theme/icon-m-person?" + Theme.primaryColor
                    }
                }
                Column {
                    anchors {
                        left: avatar.right
                        leftMargin: Theme.paddingLarge
                        right: parent.right
                        rightMargin: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    Label {
                        width: parent.width
                        truncationMode: TruncationMode.Fade
                        text: Accounts.defaultProfileName !== "" ? Accounts.defaultProfileName : qsTr("Add an account")
                        color: accountItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                    }
                    Label {
                        width: parent.width
                        truncationMode: TruncationMode.Fade
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: Theme.secondaryColor
                        text: Accounts.count > 0 ? Accounts.defaultAccountKind : qsTr("Sign in with Microsoft to play")
                    }
                }
            }

            // ---- instances
            SectionHeader {
                text: qsTr("Instances")
            }

            SilicaListView {
                id: instancesView
                width: parent.width
                height: Theme.itemSizeHuge * 1.5
                orientation: ListView.Horizontal
                leftMargin: Theme.horizontalPageMargin - Theme.paddingSmall
                rightMargin: leftMargin
                model: Instances
                currentIndex: Instances.indexOfId(Launcher.selectedInstanceId)
                clip: true

                delegate: InstanceTile {
                    selected: instanceId === Launcher.selectedInstanceId
                    iconKey: model.iconKey
                    title: model.name
                    subtitle: model.loader !== "" ? model.gameVersion + " · " + model.loader.split(" ")[0] : model.gameVersion
                    onClicked: Launcher.selectedInstanceId = instanceId
                    onPressAndHold: pageStack.push(Qt.resolvedUrl("InstancePage.qml"), { instanceId: instanceId })
                }

                footer: InstanceTile {
                    iconKey: ""
                    title: qsTr("New")
                    subtitle: qsTr("instance")
                    onClicked: pageStack.push(Qt.resolvedUrl("NewInstanceDialog.qml"))

                    Image {
                        anchors.centerIn: parent
                        anchors.verticalCenterOffset: -Theme.paddingLarge
                        source: "image://theme/icon-l-add?" + Theme.highlightColor
                    }
                }
            }

            // ---- selected instance
            BackgroundItem {
                visible: Launcher.selectedInstanceId !== ""
                height: selectedColumn.height + 2 * Theme.paddingMedium
                onClicked: pageStack.push(Qt.resolvedUrl("InstancePage.qml"), { instanceId: Launcher.selectedInstanceId })

                Column {
                    id: selectedColumn
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * x - Theme.iconSizeMedium
                    anchors.verticalCenter: parent.verticalCenter

                    Label {
                        width: parent.width
                        truncationMode: TruncationMode.Fade
                        text: Launcher.selectedName
                        font.pixelSize: Theme.fontSizeLarge
                        color: Theme.highlightColor
                    }
                    Label {
                        width: parent.width
                        wrapMode: Text.Wrap
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.secondaryHighlightColor
                        text: {
                            var parts = [qsTr("Minecraft %1").arg(Launcher.selectedGameVersion)]
                            if (Launcher.selectedLoader !== "")
                                parts.push(Launcher.selectedLoader)
                            return parts.join(" · ")
                        }
                    }
                }
                Image {
                    anchors {
                        right: parent.right
                        rightMargin: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    source: "image://theme/icon-m-right?" + Theme.highlightColor
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: Instances.count === 0
                wrapMode: Text.Wrap
                text: qsTr("No instances yet. Tap New to install a Minecraft version.")
                color: Theme.secondaryHighlightColor
                font.pixelSize: Theme.fontSizeSmall
            }

            // ---- news
            SectionHeader {
                text: qsTr("News")
            }

            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                running: News.loading && News.count === 0
                visible: running
                size: BusyIndicatorSize.Medium
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: News.error !== "" && News.count === 0
                text: qsTr("News could not be loaded")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
            }

            Repeater {
                model: News
                delegate: BackgroundItem {
                    width: column.width
                    height: newsColumn.height + 2 * Theme.paddingMedium
                    // the feed comes from the internet: only open web links from it
                    onClicked: if (/^https?:\/\//i.test(link)) Qt.openUrlExternally(link)

                    Column {
                        id: newsColumn
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * x
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: Theme.paddingSmall / 2

                        Label {
                            width: parent.width
                            wrapMode: Text.Wrap
                            text: title
                            color: parent.parent.highlighted ? Theme.highlightColor : Theme.primaryColor
                        }
                        Label {
                            width: parent.width
                            text: Format.formatDate(date, Formatter.DateMedium)
                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: Theme.highlightColor
                        }
                        Label {
                            width: parent.width
                            wrapMode: Text.Wrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                            text: summary
                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: Theme.secondaryColor
                        }
                    }
                }
            }
        }

        VerticalScrollDecorator {}
    }

    // ---- launch area
    Item {
        id: playPanel
        anchors.bottom: parent.bottom
        width: parent.width
        height: playButton.height + Theme.paddingMedium

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 0.3; color: Theme.rgba(Theme.highlightDimmerColor, 0.6) }
            }
        }

        PlayButton {
            id: playButton
            anchors.bottom: parent.bottom
            anchors.bottomMargin: Theme.paddingSmall
            width: parent.width
            onClicked: page.play()
            onPressAndHold: pageStack.push(Qt.resolvedUrl("ConsolePage.qml"))
        }
    }
}
