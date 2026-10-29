import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    readonly property int systemMemory: Launcher.systemMemoryMiB()
    readonly property var proxyTypes: ["Default", "None", "HTTP", "SOCKS5"]

    function rendererName(id) {
        var list = Renderers.renderers
        for (var i = 0; i < list.length; i++)
            if (list[i].id === id)
                return list[i].name
        return id
    }

    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: qsTr("Settings")
            }

            SectionHeader { text: qsTr("Graphics") }

            ValueButton {
                id: rendererButton
                label: qsTr("Renderer")
                value: page.rendererName(Launcher.setting("SailRenderer"))
                onClicked: {
                    var picker = pageStack.push(Qt.resolvedUrl("RendererPage.qml"))
                    picker.selected.connect(function(id) { rendererButton.value = page.rendererName(id) })
                }
            }

            TextSwitch {
                text: qsTr("Use system GLFW")
                description: qsTr("Load libglfw from the renderer folder instead of the one shipped with the game. Needed on ARM, where Mojang provides no natives.")
                checked: Launcher.setting("UseNativeGLFW")
                onCheckedChanged: Launcher.setSetting("UseNativeGLFW", checked)
            }
            TextSwitch {
                text: qsTr("Use system OpenAL")
                checked: Launcher.setting("UseNativeOpenAL")
                onCheckedChanged: Launcher.setSetting("UseNativeOpenAL", checked)
            }

            SectionHeader { text: qsTr("Java") }

            ValueButton {
                label: qsTr("Java runtimes")
                value: Launcher.setting("JavaPath") || qsTr("Automatic")
                onClicked: pageStack.push(Qt.resolvedUrl("JavaPage.qml"))
            }

            Slider {
                width: parent.width
                label: qsTr("Maximum memory")
                minimumValue: 512
                maximumValue: Math.max(1024, page.systemMemory - 512)
                stepSize: 128
                value: Launcher.setting("MaxMemAlloc")
                valueText: qsTr("%1 MiB").arg(value)
                onReleased: Launcher.setSetting("MaxMemAlloc", value)
            }
            Slider {
                width: parent.width
                label: qsTr("Minimum memory")
                minimumValue: 256
                maximumValue: 2048
                stepSize: 128
                value: Launcher.setting("MinMemAlloc")
                valueText: qsTr("%1 MiB").arg(value)
                onReleased: Launcher.setSetting("MinMemAlloc", value)
            }
            TextField {
                width: parent.width
                label: qsTr("JVM arguments")
                placeholderText: qsTr("JVM arguments")
                text: Launcher.setting("JvmArgs")
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                onActiveFocusChanged: if (!activeFocus) Launcher.setSetting("JvmArgs", text)
            }

            SectionHeader { text: qsTr("Network") }

            ComboBox {
                id: proxyBox
                label: qsTr("Proxy")
                currentIndex: Math.max(0, page.proxyTypes.indexOf(Launcher.setting("ProxyType")))
                menu: ContextMenu {
                    MenuItem { text: qsTr("System") }
                    MenuItem { text: qsTr("None") }
                    MenuItem { text: "HTTP" }
                    MenuItem { text: "SOCKS5" }
                }
                onCurrentIndexChanged: {
                    Launcher.setSetting("ProxyType", page.proxyTypes[currentIndex])
                    Launcher.applyProxySettings()
                }
            }
            TextField {
                width: parent.width
                visible: proxyBox.currentIndex >= 2
                label: qsTr("Proxy address")
                text: Launcher.setting("ProxyAddr")
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhUrlCharactersOnly
                onActiveFocusChanged: if (!activeFocus) {
                    Launcher.setSetting("ProxyAddr", text)
                    Launcher.applyProxySettings()
                }
            }
            TextField {
                width: parent.width
                visible: proxyBox.currentIndex >= 2
                label: qsTr("Proxy port")
                text: Launcher.setting("ProxyPort")
                inputMethodHints: Qt.ImhDigitsOnly
                onActiveFocusChanged: if (!activeFocus) {
                    Launcher.setSetting("ProxyPort", parseInt(text))
                    Launcher.applyProxySettings()
                }
            }

            SectionHeader { text: qsTr("Advanced") }

            TextArea {
                width: parent.width
                label: qsTr("Extra environment for the game (KEY=VALUE per line)")
                placeholderText: label
                text: Launcher.setting("SailExtraEnv")
                font.family: "monospace"
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                onActiveFocusChanged: if (!activeFocus) Launcher.setSetting("SailExtraEnv", text)
            }

            DetailItem {
                label: qsTr("Data folder")
                value: Launcher.dataPath
            }
        }
        VerticalScrollDecorator {}
    }
}
