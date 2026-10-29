#pragma once

#include <QObject>
#include <QVariantList>

/*
 * Game renderer selection (MojoLauncher calls these "renderers").
 *
 * Minecraft needs desktop OpenGL, Sailfish OS devices only provide OpenGL ES through libhybris
 * or Mesa. The renderer decides which libGL/GLFW the game process loads. The launcher logic passes
 * GAME_LIBRARY_PATH as LD_LIBRARY_PATH to the game (see CleanEnviroment()), so a renderer is a
 * folder with libraries plus a set of environment variables.
 */
class RendererConfig : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList renderers READ renderers CONSTANT)
public:
    explicit RendererConfig(QObject *parent = nullptr);

    QVariantList renderers() const;

    // Folder searched for the libraries of a renderer (first match wins)
    Q_INVOKABLE QStringList libraryFolders(const QString &renderer) const;
    Q_INVOKABLE bool isInstalled(const QString &renderer) const;

    // Sets the environment of this process so that the next game launch inherits it.
    static void applyForLaunch();
};
