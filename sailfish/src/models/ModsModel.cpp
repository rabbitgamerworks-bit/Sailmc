#include "ModsModel.h"

#include "Application.h"
#include "InstanceList.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/mod/ModFolderModel.h"

ModsModel::ModsModel(QObject *parent) : QAbstractListModel(parent)
{
}

ModsModel::~ModsModel()
{
    if (m_source) {
        m_source->stopWatching();
    }
}

void ModsModel::setInstanceId(const QString &id)
{
    if (id == m_instanceId) {
        return;
    }
    m_instanceId = id;
    attach();
    emit instanceIdChanged();
}

void ModsModel::setKind(const QString &kind)
{
    if (kind == m_kind) {
        return;
    }
    m_kind = kind;
    attach();
    emit instanceIdChanged();
}

void ModsModel::attach()
{
    beginResetModel();
    if (m_source) {
        m_source->stopWatching();
        disconnect(m_source.get(), nullptr, this, nullptr);
        m_source.reset();
    }
    auto mc = std::dynamic_pointer_cast<MinecraftInstance>(APPLICATION->instances()->getInstanceById(m_instanceId));
    if (mc) {
        if (m_kind == "resourcepacks") {
            m_source = mc->resourcePackList();
        } else if (m_kind == "coremods") {
            m_source = mc->coreModList();
        } else {
            m_source = mc->loaderModList();
        }
    }
    if (m_source) {
        auto reset = [this]() {
            beginResetModel();
            endResetModel();
            emit countChanged();
        };
        connect(m_source.get(), &QAbstractItemModel::modelReset, this, reset);
        connect(m_source.get(), &QAbstractItemModel::rowsInserted, this, reset);
        connect(m_source.get(), &QAbstractItemModel::rowsRemoved, this, reset);
        connect(m_source.get(), &QAbstractItemModel::dataChanged, this, reset);
        m_source->startWatching();
        m_source->update();
    }
    endResetModel();
    emit countChanged();
}

QString ModsModel::folder() const
{
    return m_source ? m_source->dir().absolutePath() : QString();
}

int ModsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || !m_source) {
        return 0;
    }
    return int(m_source->size());
}

QHash<int, QByteArray> ModsModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {VersionRole, "version"},
        {EnabledRole, "enabled"},
        {FileNameRole, "fileName"},
        {DescriptionRole, "description"},
    };
}

QVariant ModsModel::data(const QModelIndex &index, int role) const
{
    if (!m_source || !index.isValid() || index.row() >= rowCount()) {
        return QVariant();
    }
    const auto &mod = m_source->at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return mod.name();
    case VersionRole:
        return mod.version();
    case EnabledRole:
        return mod.enabled();
    case FileNameRole:
        return mod.filename().fileName();
    case DescriptionRole:
        return mod.description();
    default:
        return QVariant();
    }
}

void ModsModel::setEnabled(int row, bool enabled)
{
    if (!m_source || row < 0 || row >= rowCount()) {
        return;
    }
    m_source->setModStatus({m_source->index(row, 0)}, enabled ? ModFolderModel::Enable : ModFolderModel::Disable);
}

void ModsModel::remove(int row)
{
    if (!m_source || row < 0 || row >= rowCount()) {
        return;
    }
    m_source->deleteMods({m_source->index(row, 0)});
}

void ModsModel::refresh()
{
    if (m_source) {
        m_source->update();
    }
}
