#include "RendererConfig.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include "Application.h"
#include "BuildConfig.h"
#include "FileSystem.h"
#include "settings/SettingsObject.h"

namespace {
struct Renderer {
    const char *id;
    const char *name;
    const char *description;
    const char *probe; // library that must exist for the renderer to be usable, or nullptr
    QList<QPair<QByteArray, QByteArray>> env;
};

const QList<Renderer> &allRenderers()
{
    static const QList<Renderer> list = {
        {"gl4es", QT_TRANSLATE_NOOP("RendererConfig", "GL4ES"),
         QT_TRANSLATE_NOOP("RendererConfig", "Translates OpenGL 2.1 to the GPU's OpenGL ES 2. Best compatibility up to 1.16."),
         "libGL.so.1",
         // LIBGL_NOTEST: GL4ES' extension probe fails on Wayland (no pbuffer config) and then nothing gets drawn
         {{"LIBGL_ES", "2"}, {"LIBGL_GL", "21"}, {"LIBGL_NOBANNER", "1"}, {"LIBGL_NOERROR", "1"}, {"LIBGL_MIPMAP", "3"},
          {"LIBGL_NOTEST", "1"}, {"GLFW_FORCE_GLES", "1"}}},
        {"ltw", QT_TRANSLATE_NOOP("RendererConfig", "LTW (GLES 3)"),
         QT_TRANSLATE_NOOP("RendererConfig", "OpenGL 3.2 core on top of OpenGL ES 3, needed by 1.17 and newer."),
         "libltw.so",
         {{"LIBGL_ES", "3"}, {"GLFW_FORCE_GLES", "1"}}},
        {"zink", QT_TRANSLATE_NOOP("RendererConfig", "Mesa Zink (Vulkan)"),
         QT_TRANSLATE_NOOP("RendererConfig", "Desktop OpenGL from Mesa on top of the Vulkan driver."),
         "libGL.so.1",
         {{"MESA_LOADER_DRIVER_OVERRIDE", "zink"}, {"GALLIUM_DRIVER", "zink"}, {"MESA_GL_VERSION_OVERRIDE", "4.6"}}},
        {"llvmpipe", QT_TRANSLATE_NOOP("RendererConfig", "Mesa llvmpipe (software)"),
         QT_TRANSLATE_NOOP("RendererConfig", "CPU rendering. Slow, but works everywhere - useful for testing in the emulator."),
         "libGL.so.1",
         {{"LIBGL_ALWAYS_SOFTWARE", "1"}, {"GALLIUM_DRIVER", "llvmpipe"}}},
        {"system", QT_TRANSLATE_NOOP("RendererConfig", "System libraries"),
         QT_TRANSLATE_NOOP("RendererConfig", "Use whatever libGL and libglfw the system provides, without changes."),
         nullptr,
         {}},
    };
    return list;
}
}

RendererConfig::RendererConfig(QObject *parent) : QObject(parent)
{
}

QStringList RendererConfig::libraryFolders(const QString &renderer) const
{
    QStringList folders;
    const QString custom = APPLICATION->settings()->get("SailRendererLibDir").toString();
    if (!custom.isEmpty()) {
        folders << custom;
    }
    folders << FS::PathCombine(APPLICATION->dataPath(), "renderers", renderer);
    folders << FS::PathCombine(FS::PathCombine(APPLICATION->root(), "share", BuildConfig.LAUNCHER_NAME), "renderers", renderer);
    return folders;
}

bool RendererConfig::isInstalled(const QString &renderer) const
{
    for (const auto &r : allRenderers()) {
        if (renderer != r.id) {
            continue;
        }
        if (!r.probe) {
            return true;
        }
        for (const auto &folder : libraryFolders(renderer)) {
            if (QFileInfo::exists(FS::PathCombine(folder, r.probe))) {
                return true;
            }
        }
    }
    return false;
}

QVariantList RendererConfig::renderers() const
{
    QVariantList out;
    for (const auto &r : allRenderers()) {
        QVariantMap entry;
        entry["id"] = r.id;
        entry["name"] = tr(r.name);
        entry["description"] = tr(r.description);
        entry["installed"] = isInstalled(r.id);
        // where the user can drop the libraries of this renderer
        entry["folder"] = r.probe ? FS::PathCombine(APPLICATION->dataPath(), "renderers", r.id) : QString();
        out << entry;
    }
    return out;
}

void RendererConfig::applyForLaunch()
{
    RendererConfig config;
    const QString selected = APPLICATION->settings()->get("SailRenderer").toString();

    // undo the variables of all renderers first, the user may have switched
    for (const auto &r : allRenderers()) {
        for (const auto &kv : r.env) {
            qunsetenv(kv.first.constData());
        }
    }

    QStringList libPaths;
    QString libGL;
    for (const auto &r : allRenderers()) {
        if (selected != r.id) {
            continue;
        }
        for (const auto &kv : r.env) {
            qputenv(kv.first.constData(), kv.second);
        }
        for (const auto &folder : config.libraryFolders(selected)) {
            if (QDir(folder).exists()) {
                libPaths << folder;
                const QString candidate = FS::PathCombine(folder, "libGL.so.1");
                if (libGL.isEmpty() && QFileInfo::exists(candidate)) {
                    libGL = candidate;
                }
            }
        }
    }
    // shared libraries every renderer may need (e.g. a Wayland enabled libglfw.so)
    for (const auto &common : config.libraryFolders("common")) {
        if (QDir(common).exists()) {
            libPaths << common;
        }
    }

    // Sailfish OS sessions are Wayland only
    qputenv("XDG_SESSION_TYPE", "wayland");
    qputenv("GAME_LIBRARY_PATH", libPaths.join(':').toLocal8Bit());

    // LWJGL looks in org.lwjgl.librarypath before java.library.path (the natives of the game
    // are only built for x86_64), and loads OpenGL from the renderer instead of the system.
    // jemalloc is not built for Sailfish OS, LWJGL falls back to the system allocator.
    QStringList jvmArgs;
    if (!libPaths.isEmpty()) {
        jvmArgs << "-Dorg.lwjgl.librarypath=" + libPaths.join(':');
    }
    if (!libGL.isEmpty()) {
        jvmArgs << "-Dorg.lwjgl.opengl.libname=" + libGL;
    }
    jvmArgs << "-Dorg.lwjgl.system.allocator=system";
    qputenv("SAILMC_JVM_ARGS", jvmArgs.join(' ').toLocal8Bit());

    // KEY=VALUE per line, for experiments
    const auto extra = APPLICATION->settings()->get("SailExtraEnv").toString().split('\n', QString::SkipEmptyParts);
    for (const auto &line : extra) {
        const int eq = line.indexOf('=');
        if (eq > 0) {
            qputenv(line.left(eq).trimmed().toLocal8Bit().constData(), line.mid(eq + 1).trimmed().toLocal8Bit());
        }
    }
    qDebug() << "Renderer" << selected << "library path" << libPaths << "JVM" << jvmArgs;
}
