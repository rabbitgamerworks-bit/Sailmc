#include "JavaRuntimeManager.h"

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QRegExp>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTextStream>

#include "Application.h"
#include "FileSystem.h"
#include "meta/Index.h"
#include "meta/Version.h"
#include "minecraft/VersionFilterData.h"

namespace {
// metadata of the latest release, with the download link and its SHA-256
const char *ADOPTIUM_URL = "https://api.adoptium.net/v3/assets/latest/%1/hotspot?architecture=%2&image_type=jre&os=linux&vendor=eclipse";
// Adoptium has no 32-bit x86 builds, Azul Zulu does (Java 8 and 17)
const char *ZULU_URL = "https://api.azul.com/metadata/v1/zulu/packages/?java_version=%1&os=linux&arch=i686"
                       "&archive_type=tar.gz&java_package_type=jre&javafx_bundled=false&latest=true&release_status=ga&page_size=1";
}

JavaRuntimeManager::JavaRuntimeManager(QObject *parent) : QObject(parent)
{
    refresh();
}

JavaRuntimeManager::~JavaRuntimeManager()
{
    cancel();
}

QString JavaRuntimeManager::runtimesFolder() const
{
    return FS::PathCombine(APPLICATION->dataPath(), "java");
}

QString JavaRuntimeManager::architecture() const
{
    // Adoptium architecture names
    const QString cpu = QSysInfo::currentCpuArchitecture();
    if (cpu == "arm64") {
        return "aarch64";
    }
    if (cpu == "arm") {
        return "arm";
    }
    if (cpu == "x86_64") {
        return "x64";
    }
    return cpu;
}

bool JavaRuntimeManager::useZulu() const
{
    const QString arch = architecture();
    return arch == "i386" || arch == "i686";
}

bool JavaRuntimeManager::downloadSupported() const
{
    const QString arch = architecture();
    return arch == "aarch64" || arch == "arm" || arch == "x64" || useZulu();
}

int JavaRuntimeManager::recommendedMajorFor(const QString &minecraftVersion)
{
    // Same thresholds as the VerifyJavaInstall launch step, based on the release date in the metadata
    auto version = APPLICATION->metadataIndex()->get("net.minecraft", minecraftVersion);
    const QDateTime released = version ? version->time() : QDateTime();
    if (released.isValid() && released.toMSecsSinceEpoch() > 0) {
        if (released >= g_VersionFilterData.java25BeginsDate) {
            return 25;
        }
        if (released >= g_VersionFilterData.java21BeginsDate) {
            return 21;
        }
        if (released >= g_VersionFilterData.java17BeginsDate) {
            return 17;
        }
        if (released >= g_VersionFilterData.java16BeginsDate) {
            // nothing needs exactly 16, 17 runs these versions fine
            return 17;
        }
        return 8;
    }

    // no metadata yet: guess from the version number
    const QStringList parts = minecraftVersion.split('.');
    bool ok = false;
    const int first = parts.value(0).toInt(&ok);
    if (!ok) {
        return 21; // snapshot names like 24w14a
    }
    if (first >= 26) {
        return 25; // year based versions (26.1+)
    }
    const int minor = parts.value(1).toInt();
    const int patch = parts.value(2).toInt();
    if (minor <= 16) {
        return 8;
    }
    if (minor < 20 || (minor == 20 && patch <= 4)) {
        return 17;
    }
    return 21;
}

int JavaRuntimeManager::majorOf(const QString &version)
{
    // "1.8.0_402" -> 8, "17.0.10" -> 17, "21" -> 21
    const QStringList parts = version.split(QRegExp("[._+-]"));
    int major = parts.value(0).toInt();
    if (major == 1) {
        major = parts.value(1).toInt();
    }
    return major;
}

QString JavaRuntimeManager::readReleaseVersion(const QString &javaHome)
{
    QFile release(FS::PathCombine(javaHome, "release"));
    if (!release.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    QTextStream in(&release);
    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (line.startsWith("JAVA_VERSION=")) {
            return line.mid(13).remove('"');
        }
    }
    return QString();
}

void JavaRuntimeManager::refresh()
{
    QVariantList found;
    QStringList seen;
    auto add = [&](const QString &javaHome, bool managed) {
        const QString java = FS::PathCombine(javaHome, "bin", "java");
        const QString canonical = QFileInfo(java).canonicalFilePath();
        if (canonical.isEmpty() || seen.contains(canonical) || !QFileInfo(java).isExecutable()) {
            return;
        }
        seen << canonical;
        const QString version = readReleaseVersion(javaHome);
        QVariantMap entry;
        entry["path"] = java;
        entry["home"] = javaHome;
        entry["version"] = version;
        entry["major"] = majorOf(version);
        entry["managed"] = managed;
        found << entry;
    };

    // runtimes installed by SailMC
    QDir managedDir(runtimesFolder());
    for (const auto &dir : managedDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        add(dir.absoluteFilePath(), true);
    }
    // system wide ones (e.g. from OpenRepos/Chum or manually unpacked)
    for (const auto &base : {QString("/usr/lib/jvm"), QString("/opt")}) {
        for (const auto &dir : QDir(base).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            add(dir.absoluteFilePath(), false);
        }
    }
    const QString inPath = QStandardPaths::findExecutable("java");
    if (!inPath.isEmpty()) {
        add(QFileInfo(QFileInfo(inPath).canonicalFilePath()).dir().absoluteFilePath(".."), false);
    }

    m_runtimes = found;
    emit runtimesChanged();
}

QString JavaRuntimeManager::bestFor(int majorVersion) const
{
    QString fallback;
    int fallbackMajor = 0;
    for (const auto &item : m_runtimes) {
        const auto entry = item.toMap();
        const int major = entry["major"].toInt();
        if (major == majorVersion) {
            return entry["path"].toString();
        }
        // newer Java can usually run newer game versions, pick the closest newer one
        if (major > majorVersion && (fallbackMajor == 0 || major < fallbackMajor)) {
            fallback = entry["path"].toString();
            fallbackMajor = major;
        }
    }
    return fallback;
}

void JavaRuntimeManager::setStatus(const QString &status)
{
    m_status = status;
    emit statusChanged();
}

void JavaRuntimeManager::setBusy(bool busy)
{
    m_busy = busy;
    emit busyChanged();
}

void JavaRuntimeManager::fail(const QString &reason)
{
    m_file.reset();
    setStatus(reason);
    setBusy(false);
    emit failed(reason);
}

void JavaRuntimeManager::download(int majorVersion)
{
    if (m_busy) {
        return;
    }
    if (!downloadSupported()) {
        fail(tr("No Java builds are available for the %1 architecture").arg(QSysInfo::currentCpuArchitecture()));
        return;
    }
    if (!FS::ensureFolderPathExists(runtimesFolder())) {
        fail(tr("Cannot create %1").arg(runtimesFolder()));
        return;
    }
    m_downloadMajor = majorVersion;
    m_progress = 0;
    emit progressChanged();
    setBusy(true);
    setStatus(tr("Downloading Java %1…").arg(majorVersion));

    m_file.reset(new QFile(FS::PathCombine(runtimesFolder(), QString("download-%1.tar.gz").arg(majorVersion))));
    if (!m_file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        fail(tr("Cannot write %1").arg(m_file->fileName()));
        return;
    }

    const auto unavailable = [this, majorVersion]() {
        fail(tr("No Java %1 build is available for the %2 architecture").arg(majorVersion).arg(architecture()));
    };
    if (!useZulu()) {
        fetchMetadata(QUrl(QString(ADOPTIUM_URL).arg(majorVersion).arg(architecture())),
                      [this, unavailable](const QJsonDocument &doc) {
            const auto package = doc.array().first().toObject().value("binary").toObject().value("package").toObject();
            const QString url = package.value("link").toString();
            const QByteArray sha256 = package.value("checksum").toString().toLatin1();
            if (url.isEmpty() || sha256.isEmpty()) {
                unavailable();
                return;
            }
            fetchArchive(QUrl(url), sha256);
        });
        return;
    }
    // Azul: find the package, then ask for its details (they have the SHA-256)
    fetchMetadata(QUrl(QString(ZULU_URL).arg(majorVersion)), [this, unavailable](const QJsonDocument &doc) {
        const QString uuid = doc.array().first().toObject().value("package_uuid").toString();
        if (uuid.isEmpty()) {
            unavailable();
            return;
        }
        fetchMetadata(QUrl("https://api.azul.com/metadata/v1/zulu/packages/" + uuid),
                      [this, unavailable](const QJsonDocument &details) {
            const QString url = details.object().value("download_url").toString();
            const QByteArray sha256 = details.object().value("sha256_hash").toString().toLatin1();
            if (url.isEmpty() || sha256.isEmpty()) {
                unavailable();
                return;
            }
            fetchArchive(QUrl(url), sha256);
        });
    });
}

void JavaRuntimeManager::fetchMetadata(const QUrl &url, std::function<void(const QJsonDocument &)> done)
{
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    request.setHeader(QNetworkRequest::UserAgentHeader, "SailMC");
    QNetworkReply *meta = APPLICATION->network()->get(request);
    m_reply = meta;
    connect(meta, &QNetworkReply::finished, this, [this, meta, done]() {
        meta->deleteLater();
        if (m_reply != meta) {
            return; // cancelled
        }
        m_reply.clear();
        if (meta->error() != QNetworkReply::NoError) {
            fail(tr("Download failed: %1").arg(meta->errorString()));
            return;
        }
        done(QJsonDocument::fromJson(meta->readAll()));
    });
}

void JavaRuntimeManager::fetchArchive(const QUrl &url, const QByteArray &sha256)
{
    m_expectedSha256 = sha256.toLower();
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    request.setHeader(QNetworkRequest::UserAgentHeader, "SailMC");
    m_reply = APPLICATION->network()->get(request);
    connect(m_reply.data(), &QNetworkReply::readyRead, this, [this]() {
        if (m_file && m_reply) {
            m_file->write(m_reply->readAll());
        }
    });
    connect(m_reply.data(), &QNetworkReply::downloadProgress, this, [this](qint64 got, qint64 total) {
        if (total > 0) {
            m_progress = qreal(got) / total;
            emit progressChanged();
        }
    });
    connect(m_reply.data(), &QNetworkReply::finished, this, &JavaRuntimeManager::finishDownload);
}

void JavaRuntimeManager::finishDownload()
{
    QNetworkReply *reply = m_reply;
    m_reply.clear();
    if (!reply || !m_file) {
        return;
    }
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        m_file->remove();
        fail(tr("Download failed: %1").arg(reply->errorString()));
        return;
    }
    m_file->write(reply->readAll());
    m_file->close();

    const QString archive = m_file->fileName();
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!m_file->open(QIODevice::ReadOnly) || !hash.addData(m_file.get()) || hash.result().toHex() != m_expectedSha256) {
        m_file->close();
        m_file->remove();
        fail(tr("The downloaded Java %1 is damaged (checksum mismatch)").arg(m_downloadMajor));
        return;
    }
    m_file->close();
    const QString target = FS::PathCombine(runtimesFolder(), QString(useZulu() ? "zulu-%1-jre" : "temurin-%1-jre").arg(m_downloadMajor));
    QDir(target).removeRecursively();
    FS::ensureFolderPathExists(target);

    setStatus(tr("Unpacking Java %1…").arg(m_downloadMajor));
    m_extract = new QProcess(this);
    connect(m_extract.data(), static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this,
            [this, archive, target](int code, QProcess::ExitStatus status) {
        m_extract->deleteLater();
        QFile::remove(archive);
        m_file.reset();
        if (status != QProcess::NormalExit || code != 0) {
            fail(tr("Unpacking the Java runtime failed"));
            return;
        }
        refresh();
        const QString java = FS::PathCombine(target, "bin", "java");
        setStatus(tr("Java %1 installed").arg(m_downloadMajor));
        setBusy(false);
        emit installed(java, m_downloadMajor);
    });
    // the archive contains a single top level folder (jdk-21.0.x+y-jre)
    m_extract->start("tar", {"-xzf", archive, "-C", target, "--strip-components=1"});
}

void JavaRuntimeManager::cancel()
{
    if (m_reply) {
        m_reply->abort();
    }
    if (m_extract) {
        m_extract->kill();
    }
}

bool JavaRuntimeManager::remove(const QString &javaPath)
{
    // only runtimes managed by SailMC may be removed
    const QString home = QFileInfo(QFileInfo(javaPath).absolutePath()).absolutePath();
    if (!home.startsWith(runtimesFolder())) {
        return false;
    }
    const bool ok = QDir(home).removeRecursively();
    refresh();
    return ok;
}
