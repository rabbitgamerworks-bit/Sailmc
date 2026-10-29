import QtQuick 2.6
import Sailfish.Silica 1.0
import "components"

ApplicationWindow {
    id: app

    initialPage: startupError !== "" ? Qt.resolvedUrl("pages/StartupErrorPage.qml")
                                     : Qt.resolvedUrl("pages/HomePage.qml")
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    allowedOrientations: defaultAllowedOrientations

    function notify(text) {
        banner.show(text)
    }

    Banner {
        id: banner
    }

    Connections {
        target: startupError === "" ? Launcher : null
        ignoreUnknownSignals: true
        onErrorOccurred: banner.show(message)
        onTaskSucceeded: banner.show(qsTr("%1 finished").arg(title))
        onLoginSucceeded: banner.show(qsTr("Signed in as %1").arg(profileName))
        onAccountRequired: {
            banner.show(message)
            pageStack.push(Qt.resolvedUrl("pages/AccountsPage.qml"))
        }
        onJavaRequired: pageStack.push(Qt.resolvedUrl("pages/JavaPage.qml"), { suggestedMajor: majorVersion })
        onOfflineNameRequired: pageStack.push(Qt.resolvedUrl("pages/OfflineNameDialog.qml"),
                                              { instanceId: instanceId, playerName: suggestedName })
    }
}
