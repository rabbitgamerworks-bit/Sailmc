#pragma once

#include <QAbstractListModel>

#include "minecraft/auth/MinecraftAccount.h"

/*
 * QML view of the launcher's AccountList (Microsoft accounts).
 */
class AccountModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString defaultAccountId READ defaultAccountId NOTIFY defaultAccountChanged)
    Q_PROPERTY(QString defaultProfileName READ defaultProfileName NOTIFY defaultAccountChanged)
    // what kind of account plays: Microsoft, offline or the server of an authlib-injector account
    Q_PROPERTY(QString defaultAccountKind READ defaultAccountKind NOTIFY defaultAccountChanged)
public:
    enum Roles {
        AccountIdRole = Qt::UserRole + 1,
        ProfileNameRole,
        GamerTagRole,
        StateRole,
        IsDefaultRole,
        OwnsMinecraftRole,
        HasProfileRole,
        BusyRole,
        ErrorRole,
        OfflineRole,
        // account of a Yggdrasil server used through authlib-injector
        ServerRole
    };

    explicit AccountModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return rowCount(); }
    QString defaultAccountId() const;
    QString defaultProfileName() const;
    QString defaultAccountKind() const;

    Q_INVOKABLE void setDefault(const QString &accountId);
    Q_INVOKABLE void remove(const QString &accountId);
    Q_INVOKABLE void refresh(const QString &accountId);

    static MinecraftAccountPtr byId(const QString &accountId);

signals:
    void countChanged();
    void defaultAccountChanged();

private slots:
    void reload();

private:
    // rows of the core AccountList that hold a real account
    QList<int> m_rows;
};
