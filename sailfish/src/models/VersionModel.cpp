#include "VersionModel.h"

#include "Application.h"
#include "meta/Index.h"
#include "meta/Version.h"
#include "meta/JsonFormat.h"

VersionModel::VersionModel(QObject *parent) : QAbstractListModel(parent)
{
}

void VersionModel::setUid(const QString &uid)
{
    if (uid == m_uid) {
        return;
    }
    m_uid = uid;
    emit uidChanged();
    reload();
}

void VersionModel::setMinecraftVersion(const QString &version)
{
    if (version == m_mcVersion) {
        return;
    }
    m_mcVersion = version;
    emit filterChanged();
    rebuild();
}

void VersionModel::setShowReleases(bool v)
{
    m_showReleases = v;
    emit filterChanged();
    rebuild();
}

void VersionModel::setShowSnapshots(bool v)
{
    m_showSnapshots = v;
    emit filterChanged();
    rebuild();
}

void VersionModel::setShowOld(bool v)
{
    m_showOld = v;
    emit filterChanged();
    rebuild();
}

void VersionModel::reload()
{
    if (m_uid.isEmpty()) {
        return;
    }
    m_list = APPLICATION->metadataIndex()->get(m_uid);
    m_loadTask = m_list->getLoadTask();
    m_error.clear();
    m_loading = true;
    emit loadingChanged();
    connect(m_loadTask.get(), &Task::finished, this, [this]() {
        m_loading = false;
        if (m_loadTask && !m_loadTask->wasSuccessful()) {
            m_error = m_loadTask->failReason();
        }
        emit loadingChanged();
        rebuild();
    });
    m_loadTask->start();
    // cached data can be shown right away while the refresh runs
    rebuild();
}

bool VersionModel::accepts(const Meta::VersionPtr &version) const
{
    if (m_uid == "net.minecraft") {
        const auto type = version->type();
        if (type == "release") {
            return m_showReleases;
        }
        if (type == "snapshot" || type == "experiment") {
            return m_showSnapshots;
        }
        if (type.startsWith("old_")) {
            return m_showOld;
        }
        return m_showSnapshots;
    }
    if (!m_mcVersion.isEmpty()) {
        for (const auto &req : version->depends()) {
            if (req.uid == "net.minecraft" && !req.equalsVersion.isEmpty()) {
                return req.equalsVersion == m_mcVersion;
            }
        }
    }
    return true;
}

void VersionModel::rebuild()
{
    beginResetModel();
    m_versions.clear();
    if (m_list) {
        for (int i = 0; i < m_list->count(); i++) {
            auto version = std::dynamic_pointer_cast<Meta::Version>(m_list->at(i));
            if (version && accepts(version)) {
                m_versions.append(version);
            }
        }
    }
    endResetModel();
    emit countChanged();
}

int VersionModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_versions.size();
}

QHash<int, QByteArray> VersionModel::roleNames() const
{
    return {
        {VersionIdRole, "versionId"},
        {TypeRole, "versionType"},
        {RecommendedRole, "recommended"},
        {LatestRole, "latest"},
        {ReleaseDateRole, "releaseDate"},
    };
}

QVariant VersionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_versions.size()) {
        return QVariant();
    }
    const auto &version = m_versions.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case VersionIdRole:
        return version->version();
    case TypeRole:
        return version->type();
    case RecommendedRole:
        return version->isRecommended();
    case LatestRole:
        return index.row() == 0;
    case ReleaseDateRole:
        return version->time();
    default:
        return QVariant();
    }
}

int VersionModel::recommendedIndex() const
{
    for (int i = 0; i < m_versions.size(); i++) {
        if (m_versions.at(i)->isRecommended()) {
            return i;
        }
    }
    return m_versions.isEmpty() ? -1 : 0;
}

QString VersionModel::versionAt(int row) const
{
    if (row < 0 || row >= m_versions.size()) {
        return QString();
    }
    return m_versions.at(row)->version();
}
