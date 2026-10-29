#pragma once

#include <QAbstractListModel>
#include <QPointer>

#include "meta/VersionList.h"
#include "tasks/Task.h"

/*
 * Versions of a metadata component (Minecraft, Fabric, Forge, ...) from the launcher meta server,
 * filtered for the "new instance" dialog.
 */
class VersionModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString uid READ uid WRITE setUid NOTIFY uidChanged)
    Q_PROPERTY(QString minecraftVersion READ minecraftVersion WRITE setMinecraftVersion NOTIFY filterChanged)
    Q_PROPERTY(bool showReleases MEMBER m_showReleases WRITE setShowReleases NOTIFY filterChanged)
    Q_PROPERTY(bool showSnapshots MEMBER m_showSnapshots WRITE setShowSnapshots NOTIFY filterChanged)
    Q_PROPERTY(bool showOld MEMBER m_showOld WRITE setShowOld NOTIFY filterChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString error READ error NOTIFY loadingChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int recommendedIndex READ recommendedIndex NOTIFY countChanged)
public:
    enum Roles {
        VersionIdRole = Qt::UserRole + 1,
        TypeRole,
        RecommendedRole,
        LatestRole,
        ReleaseDateRole
    };

    explicit VersionModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString uid() const { return m_uid; }
    void setUid(const QString &uid);
    QString minecraftVersion() const { return m_mcVersion; }
    void setMinecraftVersion(const QString &version);
    void setShowReleases(bool v);
    void setShowSnapshots(bool v);
    void setShowOld(bool v);
    bool loading() const { return m_loading; }
    QString error() const { return m_error; }
    int recommendedIndex() const;

    Q_INVOKABLE void reload();
    Q_INVOKABLE QString versionAt(int row) const;

signals:
    void uidChanged();
    void filterChanged();
    void loadingChanged();
    void countChanged();

private:
    void rebuild();
    bool accepts(const Meta::VersionPtr &version) const;

    QString m_uid;
    QString m_mcVersion;
    bool m_showReleases = true;
    bool m_showSnapshots = false;
    bool m_showOld = false;
    bool m_loading = false;
    QString m_error;
    Meta::VersionListPtr m_list;
    Task::Ptr m_loadTask;
    QList<Meta::VersionPtr> m_versions;
};
