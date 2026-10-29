#pragma once

/*
 * Sailfish OS replacement for launcher/Application.h
 *
 * The launcher logic only talks to the application object through the APPLICATION macro.
 * This class provides the same core services (settings, network, caches, instances, accounts, ...)
 * on top of QGuiApplication, so the Silica/QML frontend can run without QtWidgets.
 */

#include <QGuiApplication>
#include <QDateTime>
#include <QFile>
#include <QIcon>
#include <QMap>
#include <memory>

#include "QObjectPtr.h"

class SettingsObject;
class InstanceList;
class AccountList;
class IconList;
class QNetworkAccessManager;
class JavaInstallList;
class HttpMetaCache;
class TranslationsModel;
class CapeCache;
class SkinsModel;
class BaseProfilerFactory;
class MCEditTool;
class LocalPeer;

namespace Meta {
    class Index;
}

#if defined(APPLICATION)
#undef APPLICATION
#endif
#define APPLICATION (static_cast<Application *>(QCoreApplication::instance()))

class Application : public QGuiApplication
{
    Q_OBJECT
public:
    enum Status {
        StartingUp,
        Failed,
        Succeeded,
        Initialized
    };

public:
    Application(int &argc, char **argv);
    virtual ~Application();

    std::shared_ptr<SettingsObject> settings() const {
        return m_settings;
    }

    qint64 timeSinceStart() const {
        return startTime.msecsTo(QDateTime::currentDateTime());
    }

    // There is no icon theme engine on Sailfish OS - QML uses the Silica theme instead.
    QIcon getThemedIcon(const QString& name);

    std::shared_ptr<TranslationsModel> translations();

    std::shared_ptr<JavaInstallList> javalist();

    std::shared_ptr<InstanceList> instances() const {
        return m_instances;
    }

    std::shared_ptr<IconList> icons() const {
        return m_icons;
    }

    shared_qobject_ptr<SkinsModel> skinsModel() const {
        return m_skinsModel;
    }

    shared_qobject_ptr<AccountList> accounts() const {
        return m_accounts;
    }

    QString msaClientId() const;

    Status status() const {
        return m_status;
    }

    const QMap<QString, std::shared_ptr<BaseProfilerFactory>> &profilers() const {
        return m_profilers;
    }

    MCEditTool *mcedit() const {
        return nullptr;
    }

    void updateProxySettings(QString proxyTypeStr, QString addr, int port, QString user, QString password);

    shared_qobject_ptr<QNetworkAccessManager> network();

    shared_qobject_ptr<HttpMetaCache> metacache();

    shared_qobject_ptr<Meta::Index> metadataIndex();

    shared_qobject_ptr<CapeCache> capeCache();

    QString getJarsPath();

    /// this is the root of the installation (read-only on Sailfish OS)
    const QString &root() {
        return m_rootPath;
    }

    /// the writable data folder (instances, assets, libraries, ...)
    QString dataPath() const {
        return m_dataPath;
    }

    bool openJsonEditor(const QString &filename);

    // Updates are handled by the package manager on Sailfish OS
    bool updatesAreAllowed() {
        return false;
    }
    void updateIsRunning(bool) {}

signals:
    // another start of the app asked this one to show itself
    void activateRequested();

private:
    void showFatalErrorMessage(const QString & title, const QString & content);
    void registerSettings();

private:
    QDateTime startTime;

    shared_qobject_ptr<QNetworkAccessManager> m_network;
    shared_qobject_ptr<AccountList> m_accounts;
    shared_qobject_ptr<HttpMetaCache> m_metacache;
    shared_qobject_ptr<Meta::Index> m_metadataIndex;
    shared_qobject_ptr<CapeCache> m_capeCache;
    shared_qobject_ptr<SkinsModel> m_skinsModel;

    std::shared_ptr<SettingsObject> m_settings;
    std::shared_ptr<InstanceList> m_instances;
    std::shared_ptr<IconList> m_icons;
    std::shared_ptr<JavaInstallList> m_javalist;
    std::shared_ptr<TranslationsModel> m_translations;

    QMap<QString, std::shared_ptr<BaseProfilerFactory>> m_profilers;

    LocalPeer *m_peerInstance = nullptr;
    QString m_rootPath;
    QString m_dataPath;
    QString m_jarsPath;
    Status m_status = Application::StartingUp;

public:
    QString m_errorTitle;
    QString m_errorMessage;
    std::unique_ptr<QFile> logFile;
};
