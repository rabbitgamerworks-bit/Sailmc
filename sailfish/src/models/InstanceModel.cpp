#include "InstanceModel.h"

#include "Application.h"
#include "InstanceList.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "minecraft/Component.h"

InstanceModel::InstanceModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    setSourceModel(APPLICATION->instances().get());
    setDynamicSortFilter(true);
    sort(0);
    connect(this, &QAbstractItemModel::rowsInserted, this, &InstanceModel::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &InstanceModel::countChanged);
    connect(this, &QAbstractItemModel::modelReset, this, &InstanceModel::countChanged);
}

QHash<int, QByteArray> InstanceModel::roleNames() const
{
    return {
        {InstanceIdRole, "instanceId"},
        {NameRole, "name"},
        {IconKeyRole, "iconKey"},
        {GameVersionRole, "gameVersion"},
        {LoaderRole, "loader"},
        {RunningRole, "running"},
        {LastLaunchRole, "lastLaunch"},
        {PlayTimeRole, "playTime"},
        {NotesRole, "notes"},
        {BrokenRole, "broken"},
    };
}

InstancePtr InstanceModel::instanceAt(const QModelIndex &index) const
{
    auto id = QSortFilterProxyModel::data(index, InstanceList::InstanceIDRole).toString();
    return APPLICATION->instances()->getInstanceById(id);
}

std::shared_ptr<PackProfile> InstanceModel::loadedProfile(InstancePtr instance)
{
    auto mc = std::dynamic_pointer_cast<MinecraftInstance>(instance);
    if (!mc) {
        return nullptr;
    }
    auto profile = mc->getPackProfile();
    // the component list is loaded lazily by the launcher core
    if (!profile->getComponent("net.minecraft")) {
        profile->reload(Net::Mode::Offline);
    }
    return profile;
}

QString InstanceModel::componentVersion(const std::shared_ptr<PackProfile> &profile, const QString &uid)
{
    auto component = profile ? profile->getComponent(uid) : nullptr;
    if (!component) {
        return QString();
    }
    // the cached version is only known after the metadata was resolved once, fall back to the requested one
    const QString cached = component->getVersion();
    return cached.isEmpty() ? component->m_version : cached;
}

QString InstanceModel::gameVersionOf(InstancePtr instance)
{
    return componentVersion(loadedProfile(instance), "net.minecraft");
}

QString InstanceModel::loaderOf(InstancePtr instance)
{
    auto profile = loadedProfile(instance);
    if (!profile) {
        return QString();
    }
    static const QList<QPair<QString, QString>> loaders = {
        {"net.minecraftforge", "Forge"},
        {"net.neoforged", "NeoForge"},
        {"net.fabricmc.fabric-loader", "Fabric"},
        {"org.quiltmc.quilt-loader", "Quilt"},
        {"com.mumfrey.liteloader", "LiteLoader"},
    };
    for (const auto &loader : loaders) {
        auto version = componentVersion(profile, loader.first);
        if (!version.isEmpty()) {
            return loader.second + " " + version;
        }
    }
    return QString();
}

QString InstanceModel::formatPlayTime(qint64 seconds)
{
    if (seconds <= 0) {
        return QString();
    }
    if (seconds < 3600) {
        return tr("%n min", "", int(seconds / 60));
    }
    return tr("%1 h %2 min").arg(seconds / 3600).arg((seconds % 3600) / 60);
}

QVariant InstanceModel::data(const QModelIndex &index, int role) const
{
    if (role < InstanceIdRole) {
        return QSortFilterProxyModel::data(index, role);
    }
    auto instance = instanceAt(index);
    if (!instance) {
        return QVariant();
    }
    switch (role) {
    case InstanceIdRole:
        return instance->id();
    case NameRole:
        return instance->name();
    case IconKeyRole:
        return instance->iconKey();
    case GameVersionRole:
        return gameVersionOf(instance);
    case LoaderRole:
        return loaderOf(instance);
    case RunningRole:
        return instance->isRunning();
    case LastLaunchRole:
        return instance->lastLaunch() > 0 ? QDateTime::fromMSecsSinceEpoch(instance->lastLaunch()) : QVariant();
    case PlayTimeRole:
        return formatPlayTime(instance->totalTimePlayed());
    case NotesRole:
        return instance->notes();
    case BrokenRole:
        return instance->hasVersionBroken();
    default:
        return QVariant();
    }
}

bool InstanceModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    auto a = instanceAt(left);
    auto b = instanceAt(right);
    if (!a || !b) {
        return false;
    }
    if (a->lastLaunch() != b->lastLaunch()) {
        return a->lastLaunch() > b->lastLaunch();
    }
    return QString::localeAwareCompare(a->name(), b->name()) < 0;
}

int InstanceModel::indexOfId(const QString &id) const
{
    for (int i = 0; i < rowCount(); i++) {
        if (idAt(i) == id) {
            return i;
        }
    }
    return -1;
}

QString InstanceModel::idAt(int row) const
{
    if (row < 0 || row >= rowCount()) {
        return QString();
    }
    return QSortFilterProxyModel::data(index(row, 0), InstanceList::InstanceIDRole).toString();
}
