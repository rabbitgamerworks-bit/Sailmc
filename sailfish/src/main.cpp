#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>
#include <QtQml>

#include <sailfishapp.h>

#include "Application.h"

#include "ImageProvider.h"
#include "JavaRuntimeManager.h"
#include "RendererConfig.h"
#include "SailLauncher.h"
#include "models/AccountModel.h"
#include "models/InstanceModel.h"
#include "models/ModsModel.h"
#include "models/NewsModel.h"
#include "models/VersionModel.h"

int main(int argc, char *argv[])
{
    // resources compiled into the static launcher core
    Q_INIT_RESOURCE(multimc);
    Q_INIT_RESOURCE(skins);
    Q_INIT_RESOURCE(logo);

    Application app(argc, argv);
    if (app.status() == Application::Succeeded) {
        // already running, the other process was asked to come to front
        return 0;
    }

    qmlRegisterType<VersionModel>("SailMC", 1, 0, "VersionModel");
    qmlRegisterType<ModsModel>("SailMC", 1, 0, "ModsModel");

    QScopedPointer<QQuickView> view(SailfishApp::createView());
    auto context = view->rootContext();
    view->engine()->addImageProvider("sailmc", new ImageProvider());

    // The QML shows an error page instead of the launcher if the core failed to start
    context->setContextProperty("startupError",
                                app.status() == Application::Failed ? app.m_errorTitle + "\n\n" + app.m_errorMessage : QString());

    JavaRuntimeManager *java = nullptr;
    SailLauncher *launcher = nullptr;
    if (app.status() != Application::Failed) {
        java = new JavaRuntimeManager(&app);
        launcher = new SailLauncher(java, &app);
        context->setContextProperty("Launcher", launcher);
        context->setContextProperty("Instances", new InstanceModel(&app));
        context->setContextProperty("Accounts", new AccountModel(&app));
        context->setContextProperty("News", new NewsModel(&app));
        context->setContextProperty("JavaRuntimes", java);
        context->setContextProperty("Renderers", new RendererConfig(&app));
    }

    QObject::connect(&app, &Application::activateRequested, view.data(), [&view]() {
        view->showFullScreen();
        view->raise();
        view->requestActivate();
    });

    view->setSource(SailfishApp::pathToMainQml());
    view->show();
    return app.exec();
}
