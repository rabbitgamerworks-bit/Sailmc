#pragma once

#include <functional>
#include <QJsonDocument>

#include <QObject>
#include <QPointer>
#include <QVariantList>
#include <memory>

class QFile;
class QUrl;
class QNetworkReply;
class QProcess;

/*
 * Finds Java runtimes on the device and installs Eclipse Temurin JREs into the app data folder,
 * similar to the runtime manager of MojoLauncher. Sailfish OS uses glibc, so the regular
 * Linux builds for the device architecture work.
 */
class JavaRuntimeManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList runtimes READ runtimes NOTIFY runtimesChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString architecture READ architecture CONSTANT)
    Q_PROPERTY(bool downloadSupported READ downloadSupported CONSTANT)
public:
    explicit JavaRuntimeManager(QObject *parent = nullptr);
    ~JavaRuntimeManager();

    QVariantList runtimes() const { return m_runtimes; }
    bool busy() const { return m_busy; }
    qreal progress() const { return m_progress; }
    QString status() const { return m_status; }
    QString architecture() const;
    bool downloadSupported() const;

    QString runtimesFolder() const;

    // Java major version recommended for a Minecraft version (8 up to 1.16, 17 up to 1.20.4, 21 after)
    Q_INVOKABLE static int recommendedMajorFor(const QString &minecraftVersion);

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void download(int majorVersion);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE bool remove(const QString &javaPath);
    // Picks the best installed runtime for the major version, empty if none
    Q_INVOKABLE QString bestFor(int majorVersion) const;

signals:
    void runtimesChanged();
    void busyChanged();
    void progressChanged();
    void statusChanged();
    void installed(const QString &javaPath, int majorVersion);
    void failed(const QString &reason);

private:
    void setStatus(const QString &status);
    void setBusy(bool busy);
    bool useZulu() const;
    // the archive is checked against the SHA-256 the provider's metadata gives for it
    void fetchArchive(const QUrl &url, const QByteArray &sha256);
    // GETs provider metadata, calls done with the parsed JSON (or fails)
    void fetchMetadata(const QUrl &url, std::function<void(const QJsonDocument &)> done);
    void finishDownload();
    void fail(const QString &reason);
    static int majorOf(const QString &version);
    static QString readReleaseVersion(const QString &javaHome);

    QVariantList m_runtimes;
    bool m_busy = false;
    qreal m_progress = 0;
    QString m_status;
    int m_downloadMajor = 0;
    QByteArray m_expectedSha256;
    QPointer<QNetworkReply> m_reply;
    std::unique_ptr<QFile> m_file;
    QPointer<QProcess> m_extract;
};
