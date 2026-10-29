import QtQuick 2.6
import Sailfish.Silica 1.0

ApplicationWindow {
    id: app

    initialPage: startupError !== "" ? Qt.resolvedUrl("pages/StartupErrorPage.qml")
                                     : Qt.resolvedUrl("pages/HomePage.qml")
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    allowedOrientations: defaultAllowedOrientations

    // native Silica toast
    function notify(text) {
        Notices.show(text, Notice.Long, Notice.Center)
    }

    Connections {
        target: startupError === "" ? Launcher : null
        ignoreUnknownSignals: true
        onErrorOccurred: notify(message)
        onTaskSucceeded: notify(qsTr("%1 finished").arg(title))
        onLoginSucceeded: notify(qsTr("Signed in as %1").arg(profileName))
        onAccountRequired: {
            notify(message)
            pageStack.push(Qt.resolvedUrl("pages/AccountsPage.qml"))
        }
        onJavaRequired: pageStack.push(Qt.resolvedUrl("pages/JavaPage.qml"), { suggestedMajor: majorVersion })
        onOfflineNameRequired: pageStack.push(Qt.resolvedUrl("pages/OfflineNameDialog.qml"),
                                              { instanceId: instanceId, playerName: suggestedName })
    }
}
