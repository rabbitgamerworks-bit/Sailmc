#include "AccountModel.h"

#include "Application.h"
#include "minecraft/auth/AccountList.h"

AccountModel::AccountModel(QObject *parent) : QAbstractListModel(parent)
{
    auto accounts = APPLICATION->accounts().get();
    connect(accounts, &AccountList::listChanged, this, &AccountModel::reload);
    connect(accounts, &AccountList::accountChanged, this, &AccountModel::reload);
    connect(accounts, &AccountList::accountActivityChanged, this, &AccountModel::reload);
    connect(accounts, &AccountList::defaultAccountChanged, this, &AccountModel::reload);
    reload();
}

void AccountModel::reload()
{
    beginResetModel();
    // the core list also has a placeholder entry for "no account", skip it
    m_rows.clear();
    auto accounts = APPLICATION->accounts();
    for (int i = 0; i < accounts->count(); i++) {
        const auto &entry = accounts->at(i);
        if (entry.isAccount && entry.account) {
            m_rows << i;
        }
    }
    endResetModel();
    emit countChanged();
    emit defaultAccountChanged();
}

int AccountModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_rows.size();
}

QHash<int, QByteArray> AccountModel::roleNames() const
{
    return {
        {AccountIdRole, "accountId"},
        {ProfileNameRole, "profileName"},
        {GamerTagRole, "gamerTag"},
        {StateRole, "state"},
        {IsDefaultRole, "isDefault"},
        {OwnsMinecraftRole, "ownsMinecraft"},
        {HasProfileRole, "hasProfile"},
        {BusyRole, "busy"},
        {ErrorRole, "error"},
    };
}

QVariant AccountModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount()) {
        return QVariant();
    }
    const auto &entry = APPLICATION->accounts()->at(m_rows.at(index.row()));
    auto account = entry.account;
    if (!account) {
        return QVariant();
    }
    switch (role) {
    case Qt::DisplayRole:
    case ProfileNameRole:
        return account->hasProfile() ? account->profileName() : account->gamerTag();
    case AccountIdRole:
        return account->internalId();
    case GamerTagRole:
        return account->gamerTag();
    case StateRole:
        return account->accountStateText();
    case IsDefaultRole:
        return APPLICATION->accounts()->defaultAccount() == account;
    case OwnsMinecraftRole:
        return account->ownsMinecraft();
    case HasProfileRole:
        return account->hasProfile();
    case BusyRole:
        return account->isActive();
    case ErrorRole:
        return account->lastError();
    default:
        return QVariant();
    }
}

MinecraftAccountPtr AccountModel::byId(const QString &accountId)
{
    MinecraftAccountPtr account;
    int index = -1;
    if (APPLICATION->accounts()->getAccountById(accountId, account, index)) {
        return account;
    }
    return nullptr;
}

QString AccountModel::defaultAccountId() const
{
    auto account = APPLICATION->accounts()->defaultAccount();
    return account ? account->internalId() : QString();
}

QString AccountModel::defaultProfileName() const
{
    auto account = APPLICATION->accounts()->defaultAccount();
    if (!account) {
        return QString();
    }
    return account->hasProfile() ? account->profileName() : account->gamerTag();
}

void AccountModel::setDefault(const QString &accountId)
{
    APPLICATION->accounts()->setDefaultAccount(byId(accountId));
}

void AccountModel::remove(const QString &accountId)
{
    APPLICATION->accounts()->removeAccount(accountId);
}

void AccountModel::refresh(const QString &accountId)
{
    APPLICATION->accounts()->requestRefresh(accountId);
}
