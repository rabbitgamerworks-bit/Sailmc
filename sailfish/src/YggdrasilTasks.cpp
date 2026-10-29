#include "YggdrasilTasks.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QUuid>

#include "Application.h"
#include "FileSystem.h"

namespace {
// official distribution of authlib-injector, see https://github.com/yushijinhun/authlib-injector
const char *INJECTOR_LATEST = "https://authlib-injector.yushi.moe/artifact/latest.json";

QUrl withSlash(QUrl url)
{
    if (!url.path().endsWith('/')) {
        url.setPath(url.path() + '/');
    }
    return url;
}
}

// ---------------------------------------------------------------- helpers

void YggdrasilTask::post(const QUrl &url, const QJsonObject &body, std::function<void(QNetworkReply *)> done)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    auto reply = APPLICATION->network()->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply, done]() {
        done(reply);
        reply->deleteLater();
    });
}

void YggdrasilTask::get(const QUrl &url, std::function<void(QNetworkReply *)> done)
{
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    auto reply = APPLICATION->network()->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, done]() {
        done(reply);
        reply->deleteLater();
    });
}

void YggdrasilTask::resolveApiRoot(const QString &address, std::function<void(const QUrl &)> done)
{
    QString text = address.trimmed();
    if (!text.contains("://")) {
        text = "https://" + text;
    }
    const QUrl url = withSlash(QUrl(text));
    // the address may point somewhere else (API location indication), otherwise it is the API root
    get(url, [url, done](QNetworkReply *reply) {
        const QByteArray location = reply->rawHeader("X-Authlib-Injector-API-Location");
        done(location.isEmpty() ? url : withSlash(url.resolved(QUrl(QString::fromUtf8(location)))));
    });
}

QString YggdrasilTask::replyError(QNetworkReply *reply, const QByteArray &body)
{
    const auto message = QJsonDocument::fromJson(body).object().value("errorMessage").toString();
    return message.isEmpty() ? reply->errorString() : message;
}

// ---------------------------------------------------------------- sign in

YggdrasilLoginTask::YggdrasilLoginTask(const QString &address, const QString &username, const QString &password)
    : m_address(address), m_username(username), m_password(password)
{
}

void YggdrasilLoginTask::executeTask()
{
    setStatus(tr("Contacting the server"));
    resolveApiRoot(m_address, [this](const QUrl &apiRoot) {
        m_apiRoot = apiRoot;
        m_clientToken = QUuid::createUuid().toString().remove('{').remove('}').remove('-');
        QJsonObject agent{{"name", "Minecraft"}, {"version", 1}};
        QJsonObject body{{"agent", agent},
                         {"username", m_username},
                         {"password", m_password},
                         {"clientToken", m_clientToken},
                         {"requestUser", false}};
        setStatus(tr("Signing in"));
        post(apiRoot.resolved(QUrl("authserver/authenticate")), body, [this](QNetworkReply *reply) {
            const QByteArray data = reply->readAll();
            if (reply->error() != QNetworkReply::NoError) {
                emitFailed(replyError(reply, data));
                return;
            }
            const auto response = QJsonDocument::fromJson(data).object();
            m_accessToken = response.value("accessToken").toString();
            auto profile = response.value("selectedProfile").toObject();
            if (profile.isEmpty()) {
                // no profile chosen yet: use the first one the account has
                profile = response.value("availableProfiles").toArray().first().toObject();
            }
            m_profileId = profile.value("id").toString();
            m_profileName = profile.value("name").toString();
            if (m_accessToken.isEmpty() || m_profileId.isEmpty()) {
                emitFailed(tr("The account has no player profile on this server"));
                return;
            }
            emitSucceeded();
        });
    });
}

// ---------------------------------------------------------------- launch

YggdrasilLaunchTask::YggdrasilLaunchTask(MinecraftAccountPtr account, const QString &injectorFolder)
    : m_account(account), m_injectorFolder(injectorFolder)
{
}

void YggdrasilLaunchTask::executeTask()
{
    const QUrl apiRoot(m_account->yggdrasilServer());
    QJsonObject body{{"accessToken", m_account->accessToken()},
                     {"clientToken", m_account->accountData()->yggdrasilClientToken}};
    setStatus(tr("Signing in"));
    post(apiRoot.resolved(QUrl("authserver/refresh")), body, [this](QNetworkReply *reply) {
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            emitFailed(status >= 400 && status < 500
                           ? tr("The server did not accept the account (%1), add it again").arg(replyError(reply, data))
                           : tr("Can't reach the server: %1").arg(reply->errorString()));
            return;
        }
        const QString token = QJsonDocument::fromJson(data).object().value("accessToken").toString();
        if (!token.isEmpty()) {
            m_account->setYggdrasilAccessToken(token);
        }
        fetchMetadata();
    });
}

void YggdrasilLaunchTask::fetchMetadata()
{
    // handed to authlib-injector so the game does not have to ask again
    get(QUrl(m_account->yggdrasilServer()), [this](QNetworkReply *reply) {
        if (reply->error() == QNetworkReply::NoError) {
            m_metadata = reply->readAll();
        }
        ensureInjector();
    });
}

void YggdrasilLaunchTask::ensureInjector()
{
    m_injectorJar = FS::PathCombine(m_injectorFolder, "authlib-injector.jar");
    if (QFileInfo::exists(m_injectorJar)) {
        finish();
        return;
    }
    setStatus(tr("Downloading authlib-injector"));
    get(QUrl(INJECTOR_LATEST), [this](QNetworkReply *reply) {
        const auto latest = QJsonDocument::fromJson(reply->readAll()).object();
        const QUrl download(latest.value("download_url").toString());
        const QByteArray sha256 = latest.value("checksums").toObject().value("sha256").toString().toLatin1();
        if (reply->error() != QNetworkReply::NoError || !download.isValid() || sha256.isEmpty()) {
            emitFailed(tr("Can't download authlib-injector: %1").arg(reply->errorString()));
            return;
        }
        get(download, [this, sha256](QNetworkReply *jarReply) {
            const QByteArray jar = jarReply->readAll();
            if (jarReply->error() != QNetworkReply::NoError) {
                emitFailed(tr("Can't download authlib-injector: %1").arg(jarReply->errorString()));
                return;
            }
            if (QCryptographicHash::hash(jar, QCryptographicHash::Sha256).toHex() != sha256.toLower()) {
                emitFailed(tr("The downloaded authlib-injector is damaged (checksum mismatch)"));
                return;
            }
            QDir().mkpath(m_injectorFolder);
            QFile file(m_injectorJar);
            if (!file.open(QIODevice::WriteOnly) || file.write(jar) != jar.size()) {
                emitFailed(tr("Can't save authlib-injector to %1").arg(m_injectorJar));
                return;
            }
            file.close();
            finish();
        });
    });
}

void YggdrasilLaunchTask::finish()
{
    m_jvmArgs = QStringList{"-javaagent:" + m_injectorJar + "=" + m_account->yggdrasilServer()};
    if (!m_metadata.isEmpty()) {
        m_jvmArgs << "-Dauthlibinjector.yggdrasil.prefetched=" + QString::fromLatin1(m_metadata.toBase64());
    }
    emitSucceeded();
}
