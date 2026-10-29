import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    property string instanceId
    readonly property int systemMemory: Launcher.systemMemoryMiB()

    function get(key) { return Launcher.instanceSetting(instanceId, key) }
    function set(key, value) { Launcher.setInstanceSetting(instanceId, key, value) }

    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: qsTr("Instance settings")
            }

            SectionHeader { text: qsTr("Memory") }

            TextSwitch {
                id: memoryOverride
                text: qsTr("Custom memory for this instance")
                checked: page.get("OverrideMemory")
                onCheckedChanged: page.set("OverrideMemory", checked)
            }
            Slider {
                width: parent.width
                enabled: memoryOverride.checked
                label: qsTr("Maximum memory")
                minimumValue: 512
                maximumValue: Math.max(1024, page.systemMemory - 512)
                stepSize: 128
                value: page.get("MaxMemAlloc")
                valueText: qsTr("%1 MiB").arg(value)
                onReleased: page.set("MaxMemAlloc", value)
            }

            SectionHeader { text: qsTr("Java") }

            TextSwitch {
                id: argsOverride
                text: qsTr("Custom JVM arguments")
                checked: page.get("OverrideJavaArgs")
                onCheckedChanged: page.set("OverrideJavaArgs", checked)
            }
            TextField {
                width: parent.width
                enabled: argsOverride.checked
                label: qsTr("JVM arguments")
                placeholderText: "-XX:+UseG1GC"
                text: page.get("JvmArgs")
                onActiveFocusChanged: if (!activeFocus) page.set("JvmArgs", text)
            }

            SectionHeader { text: qsTr("Multiplayer") }

            TextSwitch {
                id: joinServer
                text: qsTr("Join a server on launch")
                checked: page.get("JoinWorldOnLaunch") && page.get("JoinServerOnLaunch")
                onCheckedChanged: {
                    // the core only joins when both are set
                    page.set("JoinWorldOnLaunch", checked)
                    page.set("JoinServerOnLaunch", checked)
                }
            }
            TextField {
                width: parent.width
                enabled: joinServer.checked
                label: qsTr("Server address")
                placeholderText: "mc.example.org:25565"
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhUrlCharactersOnly
                text: page.get("JoinServerOnLaunchAddress") || ""
                onActiveFocusChanged: if (!activeFocus) page.set("JoinServerOnLaunchAddress", text)
            }

            SectionHeader { text: qsTr("Game time") }

            TextSwitch {
                text: qsTr("Record play time")
                checked: page.get("RecordGameTime") !== false
                onCheckedChanged: {
                    page.set("OverrideGameTime", true)
                    page.set("RecordGameTime", checked)
                }
            }
        }
        VerticalScrollDecorator {}
    }
}
