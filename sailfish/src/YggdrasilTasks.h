#pragma once

#include <QNetworkReply>
#include <QPointer>
#include <QString>
#include <functional>

#include "minecraft/auth/MinecraftAccount.h"
#include "tasks/Task.h"

/*
 * Accounts of a Yggdrasil server of your own (Drasl, Blessing Skin, ...) used through authlib-injector.
 * SailLauncher only offers them when an account that owns Minecraft was added, like offline accounts.
 */

// shared helpers: finds the API root of a server (authlib-injector API location indication)
class YggdrasilTask : public Task
{
    Q_OBJECT
protected:
    void post(const QUrl &url, const QJsonObject &body, std::function<void(QNetworkReply *)> done);
    void get(const QUrl &url, std::function<void(QNetworkReply *)> done);
    void resolveApiRoot(const QString &address, std::function<void(const QUrl &)> done);
    // error text of a failed reply: the server's errorMessage, or the network error
    static QString replyError(QNetworkReply *reply, const QByteArray &body);
};

// signs in with a user name (or e-mail) and password
class YggdrasilLoginTask : public YggdrasilTask
{
    Q_OBJECT
public:
    YggdrasilLoginTask(const QString &address, const QString &username, const QString &password);

    QString apiRoot() const { return m_apiRoot.toString(); }
    QString clientToken() const { return m_clientToken; }
    QString accessToken() const { return m_accessToken; }
    QString profileId() const { return m_profileId; }
    QString profileName() const { return m_profileName; }

protected:
    void executeTask() override;

private:
    QString m_address, m_username, m_password;
    QUrl m_apiRoot;
    QString m_clientToken, m_accessToken, m_profileId, m_profileName;
};

// before launching: refreshes the token, fetches the server metadata and makes sure authlib-injector is there
class YggdrasilLaunchTask : public YggdrasilTask
{
    Q_OBJECT
public:
    YggdrasilLaunchTask(MinecraftAccountPtr account, const QString &injectorFolder);

    // JVM arguments for the game: -javaagent:authlib-injector.jar=<server> and the prefetched metadata
    QStringList jvmArguments() const { return m_jvmArgs; }

protected:
    void executeTask() override;

private:
    void fetchMetadata();
    void ensureInjector();
    void finish();

    MinecraftAccountPtr m_account;
    QString m_injectorFolder;
    QByteArray m_metadata;
    QString m_injectorJar;
    QStringList m_jvmArgs;
};
