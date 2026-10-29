#pragma once

#include <QSortFilterProxyModel>

#include "BaseInstance.h"

class PackProfile;

/*
 * QML view of the launcher's InstanceList.
 * Exposes named roles and keeps the most recently played instance first, like MojoLauncher's profile list.
 */
class InstanceModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    enum Roles {
        InstanceIdRole = Qt::UserRole + 100,
        NameRole,
        IconKeyRole,
        GameVersionRole,
        LoaderRole,
        RunningRole,
        LastLaunchRole,
        PlayTimeRole,
        NotesRole,
        BrokenRole
    };

    explicit InstanceModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    int count() const { return rowCount(); }

    Q_INVOKABLE int indexOfId(const QString &id) const;
    Q_INVOKABLE QString idAt(int row) const;

    static std::shared_ptr<PackProfile> loadedProfile(InstancePtr instance);
    static QString componentVersion(const std::shared_ptr<PackProfile> &profile, const QString &uid);
    static QString gameVersionOf(InstancePtr instance);
    static QString loaderOf(InstancePtr instance);
    static QString formatPlayTime(qint64 seconds);

signals:
    void countChanged();

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    InstancePtr instanceAt(const QModelIndex &index) const;
};
