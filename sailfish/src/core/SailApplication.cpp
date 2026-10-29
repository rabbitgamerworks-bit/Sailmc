#include "Application.h"
#include "BuildConfig.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QStandardPaths>
#include <QTextStream>

#include "InstanceList.h"
#include "FileSystem.h"

#include "minecraft/auth/AccountList.h"
#include "icons/IconList.h"
#include "net/HttpMetaCache.h"
#include "skins/CapeCache.h"
#include "skins/SkinsModel.h"
#include "java/JavaInstallList.h"
#include "settings/INISettingsObject.h"
#include "settings/Setting.h"
#include "translations/TranslationsModel.h"
#include "meta/Index.h"

namespace {
void appDebugOutput(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    const char *levels = "DWCFIS";
    const QString format("%1 %2 %3\n");

    qint64 msecstotal = APPLICATION->timeSinceStart();
    char buf[1025] = {0};
    ::snprintf(buf, 1024, "%5lld.%03lld", msecstotal / 1000, msecstotal % 1000);

    QString out = format.arg(buf).arg(levels[type]).arg(msg);

    if (APPLICATION->logFile) {
        APPLICATION->logFile->write(out.toUtf8());
        APPLICATION->logFile->flush();
    }
    QTextStream(stderr) << out.toLocal8Bit();
    fflush(stderr);
}
}

Application::Application(int &argc, char **argv) : QGuiApplication(argc, argv)
{
    // Must match the X-Sailjail section of the .desktop file, it defines the data location.
    setOrganizationName("io.github.sailmc");
    setApplicationName(BuildConfig.LAUNCHER_NAME);
    setApplicationDisplayName(BuildConfig.LAUNCHER_DISPLAYNAME);
    setApplicationVersion(BuildConfig.printableVersionString());

    startTime = QDateTime::currentDateTime();

    // Data lives in the per-app writable location, the binary is installed read-only in /usr.
    m_dataPath = QString::fromLocal8Bit(qgetenv("SAILMC_DATA_DIR"));
    if (m_dataPath.isEmpty()) {
        m_dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    }
    if (!FS::ensureFolderPathExists(m_dataPath) || !QDir::setCurrent(m_dataPath)) {
        showFatalErrorMessage(tr("Data folder is not usable"),
                              tr("The launcher data folder could not be created or opened:\n%1").arg(m_dataPath));
        return;
    }

    // init the logger
    {
        static const QString logBase = BuildConfig.LAUNCHER_NAME + "-%0.log";
        auto moveFile = [](const QString &oldName, const QString &newName) {
            QFile::remove(newName);
            QFile::copy(oldName, newName);
            QFile::remove(oldName);
        };
        moveFile(logBase.arg(1), logBase.arg(2));
        moveFile(logBase.arg(0), logBase.arg(1));

        logFile = std::unique_ptr<QFile>(new QFile(logBase.arg(0)));
        if (!logFile->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            showFatalErrorMessage(tr("Data folder is not writable"),
                                  tr("The launcher couldn't create a log file in %1").arg(m_dataPath));
            return;
        }
        qInstallMessageHandler(appDebugOutput);
        qDebug() << "<> Log initialized.";
    }

    // Root and jars paths: <prefix>/bin/sailmc -> <prefix>/share/sailmc/jars
    {
        QDir root(FS::PathCombine(applicationDirPath(), ".."));
        m_rootPath = root.absolutePath();
        m_jarsPath = QString::fromLocal8Bit(qgetenv("SAILMC_JARS_DIR"));
        if (m_jarsPath.isEmpty()) {
            m_jarsPath = FS::PathCombine(m_rootPath, "share", BuildConfig.LAUNCHER_NAME, "jars");
        }
        qDebug() << BuildConfig.LAUNCHER_DISPLAYNAME << BuildConfig.printableVersionString()
                 << "(" << BuildConfig.GIT_COMMIT << ")";
        qDebug() << "Data path                  : " << m_dataPath;
        qDebug() << "Application root path      : " << m_rootPath;
        qDebug() << "Jars path                  : " << m_jarsPath;
    }

    registerSettings();

    // initialize network access and proxy setup
    {
        m_network = new QNetworkAccessManager();
        QString proxyTypeStr = settings()->get("ProxyType").toString();
        QString addr = settings()->get("ProxyAddr").toString();
        int port = settings()->get("ProxyPort").value<qint16>();
        QString user = settings()->get("ProxyUser").toString();
        QString pass = settings()->get("ProxyPass").toString();
        updateProxySettings(proxyTypeStr, addr, port, user, pass);
        qDebug() << "<> Network done.";
    }

    // translations of the core strings (QML strings use their own .qm files)
    {
        m_translations.reset(new TranslationsModel("translations"));
        m_translations->selectLanguage(m_settings->get("Language").toString());
    }

    // Instance icons
    {
        auto setting = m_settings->getSetting("IconsDir");
        QStringList instFolders = {
            ":/icons/multimc/32x32/instances/",
            ":/icons/multimc/50x50/instances/",
            ":/icons/multimc/128x128/instances/",
            ":/icons/multimc/scalable/instances/"
        };
        m_icons.reset(new IconList(instFolders, setting->get().toString()));
        connect(setting.get(), &Setting::SettingChanged, [&](const Setting &, QVariant value) {
            m_icons->directoryChanged(value.toString());
        });
        qDebug() << "<> Instance icons intialized.";
    }

    // Skins
    {
        auto setting = m_settings->getSetting("SkinsDir");
        m_skinsModel.reset(new SkinsModel(setting->get().toString()));
        connect(setting.get(), &Setting::SettingChanged, [&](const Setting &, QVariant value) {
            m_skinsModel->directoryChanged(value.toString());
        });
    }

    // initialize and load all instances
    {
        auto InstDirSetting = m_settings->getSetting("InstanceDir");
        QString instDir = InstDirSetting->get().toString();
        qDebug() << "Instance path              : " << instDir;
        m_instances.reset(new InstanceList(m_settings, instDir, this));
        connect(InstDirSetting.get(), &Setting::SettingChanged, m_instances.get(), &InstanceList::on_InstFolderChanged);
        m_instances->loadList();
        qDebug() << "<> Instances loaded.";
    }

    // and accounts
    {
        m_accounts.reset(new AccountList(this));
        m_accounts->setListFilePath("accounts.json", true);
        m_accounts->loadList();
        m_accounts->fillQueue();
        qDebug() << "<> Accounts loaded.";
    }

    // init the http meta cache
    {
        m_metacache.reset(new HttpMetaCache("metacache"));
        m_metacache->addBase("asset_indexes", QDir("assets/indexes").absolutePath());
        m_metacache->addBase("asset_objects", QDir("assets/objects").absolutePath());
        m_metacache->addBase("versions", QDir("versions").absolutePath());
        m_metacache->addBase("libraries", QDir("libraries").absolutePath());
        m_metacache->addBase("minecraftforge", QDir("mods/minecraftforge").absolutePath());
        m_metacache->addBase("fmllibs", QDir("mods/minecraftforge/libs").absolutePath());
        m_metacache->addBase("liteloader", QDir("mods/liteloader").absolutePath());
        m_metacache->addBase("general", QDir("cache").absolutePath());
        m_metacache->addBase("ATLauncherPacks", QDir("cache/ATLauncherPacks").absolutePath());
        m_metacache->addBase("FTBPacks", QDir("cache/FTBPacks").absolutePath());
        m_metacache->addBase("TechnicPacks", QDir("cache/TechnicPacks").absolutePath());
        m_metacache->addBase("ModrinthPacks", QDir("cache/ModrinthPacks").absolutePath());
        m_metacache->addBase("root", QDir::currentPath());
        m_metacache->addBase("translations", QDir("translations").absolutePath());
        m_metacache->addBase("icons", QDir("cache/icons").absolutePath());
        m_metacache->addBase("meta", QDir("meta").absolutePath());
        m_metacache->Load();
        qDebug() << "<> Cache initialized.";
    }

    connect(this, &Application::aboutToQuit, [this]() {
        if (m_instances) {
            m_instances->saveNow();
        }
        if (logFile) {
            logFile->flush();
            logFile->close();
        }
    });

    m_status = Application::Initialized;
}

Application::~Application()
{
    qInstallMessageHandler(nullptr);
}

void Application::registerSettings()
{
    m_settings.reset(new INISettingsObject(BuildConfig.LAUNCHER_CONFIGFILE, this));

    m_settings->registerSetting("ShownNotifications", QString());
    m_settings->registerSetting("LastUsedGroupForNewInstance", QString());

    m_settings->registerSetting("ConsoleMaxLines", 100000);
    m_settings->registerSetting("ConsoleOverflowStop", true);

    // Folders
    m_settings->registerSetting("InstanceDir", "instances");
    m_settings->registerSetting({"CentralModsDir", "ModsDir"}, "mods");
    m_settings->registerSetting("IconsDir", "icons");
    m_settings->registerSetting("SkinsDir", "skins");

    m_settings->registerSetting("JsonEditor", QString());
    m_settings->registerSetting("Language", QString());

    // Console
    m_settings->registerSetting("ShowConsole", false);
    m_settings->registerSetting("AutoCloseConsole", false);
    m_settings->registerSetting("ShowConsoleOnError", true);
    m_settings->registerSetting("LogPrePostOutput", true);

    // Window Size - the game always runs fullscreen on a phone, these are used as a fallback
    m_settings->registerSetting({"LaunchMaximized", "MCWindowMaximize"}, true);
    m_settings->registerSetting({"MinecraftWinWidth", "MCWindowWidth"}, 854);
    m_settings->registerSetting({"MinecraftWinHeight", "MCWindowHeight"}, 480);

    // Proxy Settings
    m_settings->registerSetting("ProxyType", "Default");
    m_settings->registerSetting({"ProxyAddr", "ProxyHostName"}, "127.0.0.1");
    m_settings->registerSetting("ProxyPort", 8080);
    m_settings->registerSetting({"ProxyUser", "ProxyUsername"}, "");
    m_settings->registerSetting({"ProxyPass", "ProxyPassword"}, "");

    // Memory - phones have a lot less RAM than desktops
    m_settings->registerSetting({"MinMemAlloc", "MinMemoryAlloc"}, 512);
    m_settings->registerSetting({"MaxMemAlloc", "MaxMemoryAlloc"}, 1536);
    m_settings->registerSetting("PermGen", 128);

    // Java Settings
    m_settings->registerSetting("JavaPath", "");
    m_settings->registerSetting("JavaTimestamp", 0);
    m_settings->registerSetting("JavaArchitecture", "");
    m_settings->registerSetting("JavaVersion", "");
    m_settings->registerSetting("JavaVendor", "");
    m_settings->registerSetting("LastHostname", "");
    m_settings->registerSetting("JvmArgs", "");

    // Native library workarounds - on Sailfish OS the system (or bundled) libraries are preferred
    m_settings->registerSetting("UseNativeOpenAL", true);
    m_settings->registerSetting("UseNativeGLFW", true);

    // Game time
    m_settings->registerSetting("ShowGameTime", true);
    m_settings->registerSetting("ShowGlobalGameTime", true);
    m_settings->registerSetting("RecordGameTime", true);
    m_settings->registerSetting("ShowGameTimeHours", false);

    m_settings->registerSetting("MCLaunchMethod", "LauncherPart");
    m_settings->registerSetting("LastOfflinePlayerName", "");

    // Wrapper command for launch
    m_settings->registerSetting("WrapperCommand", "");
    m_settings->registerSetting({"PreLaunchCommand", "PreLaunchCmd"}, "");
    m_settings->registerSetting({"PostExitCommand", "PostExitCmd"}, "");

    m_settings->registerSetting("InstSortMode", "Name");
    m_settings->registerSetting("SelectedInstance", QString());

    // Sailfish OS specific: how the game renders (see sailfish/src/RendererConfig.cpp)
    m_settings->registerSetting("SailRenderer", "gl4es");
    m_settings->registerSetting("SailRendererLibDir", "");
    m_settings->registerSetting("SailExtraEnv", "");

    qDebug() << "<> Settings loaded.";
}

void Application::showFatalErrorMessage(const QString &title, const QString &content)
{
    m_status = Application::Failed;
    m_errorTitle = title;
    m_errorMessage = content;
    qCritical() << title << content;
}

std::shared_ptr<TranslationsModel> Application::translations()
{
    return m_translations;
}

std::shared_ptr<JavaInstallList> Application::javalist()
{
    if (!m_javalist) {
        m_javalist.reset(new JavaInstallList());
    }
    return m_javalist;
}

QIcon Application::getThemedIcon(const QString &name)
{
    if (name == "logo") {
        return QIcon(":/logo.svg");
    }
    return QIcon::fromTheme(name);
}

bool Application::openJsonEditor(const QString &)
{
    return false;
}

QString Application::msaClientId() const
{
    return BuildConfig.MSA_CLIENT_ID;
}

void Application::updateProxySettings(QString proxyTypeStr, QString addr, int port, QString user, QString password)
{
    if (proxyTypeStr == "SOCKS5") {
        QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::Socks5Proxy, addr, port, user, password));
    } else if (proxyTypeStr == "HTTP") {
        QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::HttpProxy, addr, port, user, password));
    } else if (proxyTypeStr == "None") {
        QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::NoProxy));
    } else {
        // "Default": follow the system (connman) proxy configuration
        QNetworkProxyFactory::setUseSystemConfiguration(true);
    }
    QNetworkProxy proxy = QNetworkProxy::applicationProxy();
    m_network->setProxy(proxy);
    qDebug() << "Proxy type" << proxyTypeStr << proxy.hostName() << proxy.port();
}

shared_qobject_ptr<HttpMetaCache> Application::metacache()
{
    return m_metacache;
}

shared_qobject_ptr<QNetworkAccessManager> Application::network()
{
    return m_network;
}

shared_qobject_ptr<Meta::Index> Application::metadataIndex()
{
    if (!m_metadataIndex) {
        m_metadataIndex.reset(new Meta::Index());
    }
    return m_metadataIndex;
}

shared_qobject_ptr<CapeCache> Application::capeCache()
{
    if (!m_capeCache) {
        m_capeCache.reset(new CapeCache(this));
    }
    return m_capeCache;
}

QString Application::getJarsPath()
{
    return m_jarsPath;
}
