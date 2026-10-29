import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    allowedOrientations: Orientation.All

    SilicaListView {
        id: list
        anchors.fill: parent
        model: Accounts

        header: PageHeader {
            title: qsTr("Accounts")
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("Add offline account")
                onClicked: {
                    if (Launcher.canAddOfflineAccount())
                        pageStack.push(Qt.resolvedUrl("OfflineAccountDialog.qml"))
                    else
                        app.notify(qsTr("You must add a Microsoft account that owns Minecraft before you can add an offline account."))
                }
            }
            MenuItem {
                text: qsTr("Add Microsoft account")
                onClicked: pageStack.push(Qt.resolvedUrl("LoginPage.qml"))
            }
        }

        delegate: ListItem {
            id: item
            contentHeight: Theme.itemSizeLarge
            onClicked: Accounts.setDefault(accountId)

            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Use for playing")
                    visible: !isDefault
                    onClicked: Accounts.setDefault(accountId)
                }
                MenuItem {
                    text: qsTr("Refresh")
                    visible: !offline
                    onClicked: Accounts.refresh(accountId)
                }
                MenuItem {
                    text: qsTr("Remove")
                    onClicked: item.remorseDelete(function() { Accounts.remove(accountId) })
                }
            }

            Image {
                id: face
                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                width: Theme.iconSizeMedium
                height: width
                smooth: false
                source: "image://sailmc/face/" + accountId
            }
            Column {
                anchors {
                    left: face.right
                    leftMargin: Theme.paddingLarge
                    right: defaultIcon.left
                    rightMargin: Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }
                Label {
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    text: profileName
                    color: item.highlighted || isDefault ? Theme.highlightColor : Theme.primaryColor
                }
                Label {
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                    text: {
                        var parts = [state]
                        if (!ownsMinecraft && !offline)
                            parts.push(qsTr("demo only"))
                        if (gamerTag !== "" && gamerTag !== profileName)
                            parts.push(gamerTag)
                        return parts.join(" · ")
                    }
                }
            }
            BusyIndicator {
                id: defaultIcon
                anchors {
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                size: BusyIndicatorSize.ExtraSmall
                running: busy
                visible: busy

                Image {
                    anchors.centerIn: parent
                    visible: isDefault && !busy
                    source: "image://theme/icon-s-installed?" + Theme.highlightColor
                }
            }
        }

        ViewPlaceholder {
            enabled: list.count === 0
            text: qsTr("No accounts")
            hintText: qsTr("Pull down to sign in with the Microsoft account you bought Minecraft with")
        }

        VerticalScrollDecorator {}
    }
}
