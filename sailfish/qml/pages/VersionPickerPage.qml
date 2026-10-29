import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    property var model
    property string title
    property bool showFilters
    signal picked(string version)

    allowedOrientations: Orientation.All

    SilicaListView {
        id: list
        anchors.fill: parent
        model: page.model

        header: Column {
            width: list.width

            PageHeader {
                title: page.title
            }
            TextSwitch {
                visible: page.showFilters
                text: qsTr("Releases")
                checked: page.model.showReleases
                onClicked: page.model.showReleases = checked
            }
            TextSwitch {
                visible: page.showFilters
                text: qsTr("Snapshots")
                checked: page.model.showSnapshots
                onClicked: page.model.showSnapshots = checked
            }
            TextSwitch {
                visible: page.showFilters
                text: qsTr("Old alpha and beta")
                checked: page.model.showOld
                onClicked: page.model.showOld = checked
            }
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("Refresh")
                onClicked: page.model.reload()
            }
        }

        delegate: ListItem {
            contentHeight: Theme.itemSizeSmall
            onClicked: {
                page.picked(versionId)
                pageStack.pop()
            }

            Label {
                id: versionLabel
                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                text: versionId
                color: highlighted ? Theme.highlightColor : Theme.primaryColor
            }
            Label {
                anchors {
                    left: versionLabel.right
                    leftMargin: Theme.paddingMedium
                    baseline: versionLabel.baseline
                }
                text: recommended ? qsTr("recommended") : (versionType !== "release" && versionType !== "" ? versionType : "")
                font.pixelSize: Theme.fontSizeExtraSmall
                color: recommended ? Theme.highlightColor : Theme.secondaryColor
            }
            Label {
                anchors {
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                text: releaseDate ? Format.formatDate(releaseDate, Formatter.DateMedium) : ""
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
            }
        }

        ViewPlaceholder {
            enabled: list.count === 0 && !page.model.loading
            text: page.model.error !== "" ? qsTr("Could not load versions") : qsTr("No versions")
            hintText: page.model.error
        }

        BusyIndicator {
            anchors.centerIn: parent
            size: BusyIndicatorSize.Large
            running: page.model.loading && list.count === 0
        }

        VerticalScrollDecorator {}
    }
}
